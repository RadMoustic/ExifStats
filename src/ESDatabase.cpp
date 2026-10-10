#include "ESDatabase.h"

// ES
#include "ESSerializer.h"

// Qt
#include <QUrl>
#include <QDir>
#include <QDirIterator>
#include <QFuture>
#include <QSettings>
#include <QStandardPaths>
#include <QtConcurrent/QtConcurrentMap>
#include <QtConcurrent/qtconcurrentrun.h>
#include <QMessageBox>
#include <QGeoCoordinate>
#include <QTimer>

// Quazip
#ifdef Q_OS_ANDROID
#include <quazip/quazip.h>
#include <quazip/quazipfile.h>
#endif // Q_OS_ANDROID

// Stl
#include <set>

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

constexpr uint DATABASE_MAGIC_NUMBER = 0xEACDEACD;
constexpr uint DATABASE_VERSION = 13;
/*static*/ const char* ESDatabase::msReadOnlyDatabaseFolderSettingsKey = "ReadOnlyDataBaseFolderPath";

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

/*static*/ ESDatabase& ESDatabase::getInstance()
{
	static ESDatabase lsInstance;
	return lsInstance;
}

/********************************************************************************/

ESDatabase::ESDatabase()
	: mProcessing(false)
	, mProcessingProgress(0.f)
	, mEmbeddingsDimension(0)
	, mLastAssignedId(0)
	, mUsefullExifVersion(USEFULLEXIF_VERSION)
{
}

/********************************************************************************/

void ESDatabase::refresh(bool pFullRefresh)
{
	updateDatabase(mFolders, pFullRefresh, !pFullRefresh);
}

/********************************************************************************/

void ESDatabase::clear()
{
#ifndef EXIFSTATS_READONLY
	{
		mUnlockDatabaseRequested = true;
		std::scoped_lock lLock(mFilesMutex);
		mUnlockDatabaseRequested = false;

		mFiles.clear();
		mIdToIndex.clear();
		mFilesPathToIndex.clear();
		mFilesHashToIndex.clear();
		mFolders.clear();
		mAllLensModels.clear();
		mAllCameraModels.clear();
		mAllTags.clear();
		mProcessedFilesCounter = 0;
		mEmbeddingsDimension = 0;
		mLastAssignedId = 0;
		ESFocalLengthIn35mmStat::msCameraModelsTo35mmFocalFactors.clear();
	}

	emit dataChanged();
#endif // #ifdef EXIFSTATS_READONLY
}

/********************************************************************************/

void ESDatabase::addFolder(const QUrl& pFolderPath, bool pClearDB)
{
	updateDatabase(QStringList(pFolderPath.toLocalFile()), pClearDB, true);
}

/********************************************************************************/

void ESDatabase::updateDatabase(const QStringList& pFolders, bool pClearDB, bool pNewFilesOnly)
{
#ifdef EXIFSTATS_READONLY
	(void)pFolders;
	(void)pClearDB;
	(void)pNewFilesOnly;
#else
	setProcessing(true);
	setProcessingProgress(0.f);

	(void)QtConcurrent::run([this, pFolders, pClearDB, pNewFilesOnly]()
		{
			mUnlockDatabaseRequested = true;
			mFilesMutex.lock();
			mUnlockDatabaseRequested = false;

			if (pClearDB)
			{
				mFolders.clear();
				mFiles.clear();
				mIdToIndex.clear();
				mFilesPathToIndex.clear();
				mFilesHashToIndex.clear();
				mLastAssignedId = 0;
			}

			QVector<int> lAllImageFileIds;

			std::set<const QString*> lUniqueFolders;
			for (const QString& lFolderPath : pFolders)
				lUniqueFolders.insert(&lFolderPath);

			for(const QString* lFolderPath: lUniqueFolders)
			{
				QDir lDir(*lFolderPath);

				// Directories
				QDirIterator lDirIt(*lFolderPath, { "*.jpeg", "*.jpg", "*.heic"}, QDir::Files, QDirIterator::Subdirectories);
				while (lDirIt.hasNext())
				{
					ESStringId lFilePath = lDirIt.next();
					auto&& [lItFound, lIsNewFile] = mFilesPathToIndex.try_emplace(lFilePath, 0);

					int lFileInfoIndex = -1;
					if(lIsNewFile)
					{
						lFileInfoIndex = static_cast<int>(mFiles.size());
						mFiles.emplace_back();
					}
					else
					{
						lFileInfoIndex = lItFound->second;
					}
					ESFileInfo& lFileInfo = mFiles[lFileInfoIndex];
					if(lIsNewFile)
					{
						lFileInfo.mId = ++mLastAssignedId;
						lFileInfo.mFilePath = lFilePath;
						lItFound->second = lFileInfoIndex;

						mFilesPathToIndex[lFilePath] = lFileInfo.mId;
						mIdToIndex[lFileInfo.mId] = lFileInfoIndex;
					}
					if (!pNewFilesOnly || lIsNewFile || lFileInfo.mReadResult != eSuccess)
						lAllImageFileIds << lFileInfoIndex;
				}

				if (!mFolders.contains(*lFolderPath))
					mFolders.append(*lFolderPath);
			}

			setProcessingProgress(0.f);

			mProcessedFilesCounter = 0;

			constexpr uint cNbFilesPerThread = 256;

			QFuture<void> lRes = QtConcurrent::map(lAllImageFileIds,
				[&](const int pFileInfoIndex)
				{
					ESFileInfo& lFileInfo = mFiles[pFileInfoIndex];

					easyexif::EXIFInfo lExifData;
					lFileInfo.mReadResult = readFileExif(lFileInfo.mFilePath, lExifData);
					if (lFileInfo.mReadResult != eSuccess)
					{
						QImage lImage(lFileInfo.mFilePath.getString());
						if (!lImage.isNull())
						{
							lFileInfo.mExif.mWidth = lImage.width();
							lFileInfo.mExif.mHeight = lImage.height();
						}
						return;
					}

					lFileInfo.mExif = convertToUsefullExif(lExifData);

					// If the size is missing in the exif, read the values directly from the file, slow, but we need them and it is only done once
					if (lFileInfo.mExif.mWidth == 0 || lFileInfo.mExif.mHeight == 0)
					{
						QImage lImage(lFileInfo.mFilePath.getString());
						if (!lImage.isNull())
						{
							lFileInfo.mExif.mWidth = lImage.width();
							lFileInfo.mExif.mHeight = lImage.height();
						}
					}

					int lProcessedFiles = mProcessedFilesCounter.fetch_add(1);

					if (mProgressMutex.tryLock())
					{
						float lNewProgress = float(lProcessedFiles) / float(lAllImageFileIds.size());
						if (lNewProgress - mProcessingProgress > 0.001)
							setProcessingProgress(lNewProgress);
						mProgressMutex.unlock();
					}
				});
			lRes.waitForFinished();
			
			// Extract all camera models and counter
			std::unordered_set<ESStringId> lCameraModels;
			std::unordered_set<ESStringId> lLensModels;
			for (const ESFileInfo& lProcessedFile : mFiles)
			{
				if (lProcessedFile.mReadResult == eSuccess)
				{
					lCameraModels.insert(lProcessedFile.mExif.mCameraModel);
					lLensModels.insert(lProcessedFile.mExif.mLensModel);
				}
			}
			mAllCameraModels.assign(lCameraModels.begin(), lCameraModels.end());
			mAllLensModels.assign(lLensModels.begin(), lLensModels.end());

			// Set the camera model idx
			{
				int lCameraModelIdx = 0;
				for (auto&& lItCamera : lCameraModels)
				{
					for (ESFileInfo& lProcessedFile : mFiles)
						if (lProcessedFile.mExif.mCameraModel == lItCamera)
							lProcessedFile.mCameraModelIdx = lCameraModelIdx;

					++lCameraModelIdx;
				}
			}

			// Set the lens model idx
			{
				int lLensModelIdx = 0;
				for (auto&& lItLens : lLensModels)
				{
					for (ESFileInfo& lProcessedFile : mFiles)
						if (lProcessedFile.mExif.mLensModel == lItLens)
							lProcessedFile.mLensModelIdx = lLensModelIdx;

					++lLensModelIdx;
				}
			}

			// Sort files by date time, to guess the geolocation for files with missing GPS data
			std::vector<ESFileInfo*> lFilesSortedByDateTime;
			for (ESFileInfo& lProcessedFile : mFiles)
			{
				if(lProcessedFile.mExif.mGeoLocationGuessed)
				{
					lProcessedFile.mExif.mGeoLocationGuessed = false;
					lProcessedFile.mExif.mGeoLocation.mLatitude = 0.f;
					lProcessedFile.mExif.mGeoLocation.mLongitude = 0.f;
				}
				if (	lProcessedFile.mReadResult == eSuccess
					&&	lProcessedFile.mExif.mDateTime > 0)
				{
					lFilesSortedByDateTime.push_back(&lProcessedFile);
				}
			}
			std::sort(lFilesSortedByDateTime.begin(), lFilesSortedByDateTime.end(), [](const ESFileInfo* a, const ESFileInfo* b)
			{
				return a->mExif.mDateTime < b->mExif.mDateTime;
			});

			// Guess GeoLocation for files with missing GPS data
			constexpr int cMaxTimeDiffInSeconds = 3 * 60 * 60; // 3 hours
			constexpr int cMaxGeoDistanceTimeDiffInSeconds = 10 * 60; // 10 min
			constexpr float cMaxGeoDistance = 10000.f; // 10km
			
			std::vector<quint64> lFilesGeoLocDateTimeDiff;
			lFilesGeoLocDateTimeDiff.resize(lFilesSortedByDateTime.size());

			ESFileInfo* lLastFileWithGPSData = nullptr;
			for (int i = 0; i < lFilesSortedByDateTime.size(); ++i)
			{
				ESFileInfo* lFileInfo = lFilesSortedByDateTime[i];
				if (!lFileInfo->mExif.mGeoLocation.isValid())
				{
					if(lLastFileWithGPSData)
					{
						quint64 lTimeDiff = lFileInfo->mExif.mDateTime - lLastFileWithGPSData->mExif.mDateTime;
						if(lTimeDiff < cMaxTimeDiffInSeconds)
						{
							lFileInfo->mExif.mGeoLocation = lLastFileWithGPSData->mExif.mGeoLocation;
							lFileInfo->mExif.mGeoLocationGuessed = true;
							lFilesGeoLocDateTimeDiff[i] = lTimeDiff;
						}
						else
						{
							lLastFileWithGPSData = nullptr;
						}
					}
				}
				else
				{
					lLastFileWithGPSData = lFileInfo;
				}
			}

			lLastFileWithGPSData = nullptr;
			for (int i = int(lFilesSortedByDateTime.size()) - 1; i >= 0; --i)
			{
				ESFileInfo* lFileInfo = lFilesSortedByDateTime[i];
				if (	lFileInfo->mExif.mGeoLocationGuessed
					||	!lFileInfo->mExif.mGeoLocation.isValid())
				{
					if(lLastFileWithGPSData)
					{
						float lDist = QGeoCoordinate(lFileInfo->mExif.mGeoLocation.mLatitude, lFileInfo->mExif.mGeoLocation.mLongitude)
							.distanceTo(QGeoCoordinate(lLastFileWithGPSData->mExif.mGeoLocation.mLatitude, lLastFileWithGPSData->mExif.mGeoLocation.mLongitude));
						
						// The guessed geo location is too far from the next file with GPS data, we discard it if the time diff is also too big
						quint64 lTimeDiff = lLastFileWithGPSData->mExif.mDateTime - lFileInfo->mExif.mDateTime;
						if(lFileInfo->mExif.mGeoLocationGuessed && lDist > cMaxGeoDistance)
						{
							if (lTimeDiff <= cMaxGeoDistanceTimeDiffInSeconds && lTimeDiff < lFilesGeoLocDateTimeDiff[i])
							{
								lFileInfo->mExif.mGeoLocation = lLastFileWithGPSData->mExif.mGeoLocation;
								lFileInfo->mExif.mGeoLocationGuessed = true;
							}
							else if(lFilesGeoLocDateTimeDiff[i] > cMaxGeoDistanceTimeDiffInSeconds)
							{
								lFileInfo->mExif.mGeoLocationGuessed = false;
								lFileInfo->mExif.mGeoLocation.mLatitude = 0.f;
								lFileInfo->mExif.mGeoLocation.mLongitude = 0.f;
							}
						}
						else
						{
							if (lTimeDiff < cMaxTimeDiffInSeconds)
							{
								if(lTimeDiff < lFilesGeoLocDateTimeDiff[i])
								{
									lFileInfo->mExif.mGeoLocation = lLastFileWithGPSData->mExif.mGeoLocation;
									lFileInfo->mExif.mGeoLocationGuessed = true;
								}
							}
						}
					}
				}
				else if (lFileInfo->mExif.mGeoLocation.isValid())
				{
					lLastFileWithGPSData = lFileInfo;
				}
			}

			// Compute the hash for all files
			mFilesHashToIndex.clear();
			for (int i = 0 ; i < mFiles.size() ; ++i)
			{
				ESFileInfo& lProcessedFile = mFiles[i];
				lProcessedFile.computeHash();
				if(!lProcessedFile.mHash.isEmpty())
					mFilesHashToIndex.emplace(lProcessedFile.mHash, i);
			}

			mFilesMutex.unlock();

			mUsefullExifVersion = USEFULLEXIF_VERSION;

			setProcessing(false);
			emit dataChanged();

			qInfo() << "Database updated and now with " << mFiles.size() << " files";
		});
#endif // EXIFSTATS_READONLY
}

/********************************************************************************/

ESReadExifFileResult ESDatabase::readFileExif(const QString& pFilePath, easyexif::EXIFInfo& pOutExif)
{
	QFile lFile(pFilePath);
	if (!lFile.exists())
		return eFileNotFound;

	if (!lFile.open(QIODevice::ReadOnly))
		return eCantOpenFile;

	int lFileSize = lFile.size();

	thread_local int lTHeaderSize = 0;
	thread_local std::unique_ptr<char[]> lTHeaderBuffer;
	auto lAllocateHeaderBuffer = [&](int pSize)
	{
		if(pSize > lTHeaderSize)
		{
			lTHeaderSize = pSize;
			lTHeaderBuffer = std::make_unique<char[]>(lTHeaderSize);
		}
	};

	if (pFilePath.right(5).toLower() == ".heic")
	{
		// We don't really read the HEIC struct but just try to find the 'Exif\0\0' header, so read a big chunk
		lAllocateHeaderBuffer(64000);
		
		int lReadSize = std::min(lTHeaderSize, lFileSize);
		if (lFile.read(lTHeaderBuffer.get(), lReadSize) != lReadSize)
			return eFailedToRead;

		// Find Exif Header
		std::string_view lFullHeaderBufferView(lTHeaderBuffer.get(), lTHeaderSize);
		size_t lOffset = lFullHeaderBufferView.find("Exif\0\0MM", 0, 8);
		if(lOffset == std::string_view::npos)
			return eParseExifErrorNoExif;

		// Parse EXIF
		return static_cast<ESReadExifFileResult>(pOutExif.parseFromEXIFSegment(reinterpret_cast<unsigned char*>(&lTHeaderBuffer[lOffset]), static_cast<unsigned int>(lTHeaderSize - lOffset)));
	}
	else
	{
		constexpr int cFirstReadSize = 32;
		lAllocateHeaderBuffer(cFirstReadSize);

		int lReadSize = std::min(cFirstReadSize, lFileSize);
		if (lFile.read(lTHeaderBuffer.get(), lReadSize) != lReadSize)
			return eFailedToRead;
	
		// Find Exif Header
		int lOffset = 0;  // current offset into buffer
		for (lOffset = 0; lOffset < lReadSize - 1; lOffset++)
			if (uchar(lTHeaderBuffer[lOffset]) == 0xFF && uchar(lTHeaderBuffer[lOffset + 1]) == 0xE1)
				break;

		if (lOffset + 4 > lReadSize)
			return eBufferTooSmallToReadExifSize;

		// Read Exif Size
		lOffset += 2;
		unsigned short lExifSize = static_cast<uint16_t>(*(lTHeaderBuffer.get() + lOffset) << 8) | *(lTHeaderBuffer.get() + lOffset + 1);

		if (lExifSize < 16)
			return eExifSizeTooSmall;

		int lHeaderIncludingExifSize = lOffset + lExifSize;

		// Read the header including the full exif data
		lAllocateHeaderBuffer(lHeaderIncludingExifSize);
		lFile.seek(0);
		if (lFile.read(lTHeaderBuffer.get(), lHeaderIncludingExifSize) != lHeaderIncludingExifSize)
			return eFailedToRead;

		// Parse EXIF
		return static_cast<ESReadExifFileResult>(pOutExif.parseFrom(reinterpret_cast<unsigned char *>(lTHeaderBuffer.get()), lHeaderIncludingExifSize));
	}
}

/********************************************************************************/

ESUsefullExif ESDatabase::convertToUsefullExif(const easyexif::EXIFInfo& pFullExif)
{
	ESUsefullExif lResult;

	lResult.mCameraModel = QString(pFullExif.Model.c_str());
	lResult.mLensModel = QString(pFullExif.LensInfo.Model.c_str());
	lResult.mFNumber = pFullExif.FNumber;
	QDateTime lExifDateTime = QDateTime::fromString(QString(pFullExif.DateTimeOriginal.c_str()), "yyyy:MM:dd hh:mm:ss");
	if(!lExifDateTime.isValid())
		lExifDateTime = QDateTime::fromString(QString(pFullExif.DateTime.c_str()), "yyyy:MM:dd hh:mm:ss");
	lResult.mDateTime = lExifDateTime.toSecsSinceEpoch();
	lResult.mGeoLocation.mLatitude = static_cast<float>(pFullExif.GeoLocation.Latitude);
	lResult.mGeoLocation.mLongitude = static_cast<float>(pFullExif.GeoLocation.Longitude);
	lResult.mFocalLength = pFullExif.FocalLength;
	lResult.mFocalLengthIn35mm = pFullExif.FocalLengthIn35mm;
	lResult.mOrientation = ESExifOrientation(pFullExif.Orientation);
	lResult.mShutterSpeedValue = pFullExif.ExposureTime > 0 ? 1.0 / pFullExif.ExposureTime : 0;
	lResult.mISOSpeedRatings = pFullExif.ISOSpeedRatings;
	lResult.mWidth = pFullExif.ImageWidth > std::numeric_limits<decltype(lResult.mWidth)>::max() ? 0 : pFullExif.ImageWidth;
	lResult.mHeight = pFullExif.ImageHeight > std::numeric_limits<decltype(lResult.mHeight)>::max() ? 0 : pFullExif.ImageHeight;

	return lResult;
}

/********************************************************************************/

template<class SERIALIZER>
bool ESDatabase::Serialize(SERIALIZER& pSerializer, const QString& pFilePath)
{
	if (!pSerializer.SerializeCheck(DATABASE_MAGIC_NUMBER))
	{
		qWarning() << "Cannot load database: corrupted file: " << pFilePath;
		return false;
	}

	uint lDatabaseVersion = DATABASE_VERSION;
	if (!pSerializer.SerializeCheck(uint(4), std::greater_equal(), lDatabaseVersion, DATABASE_VERSION))
	{
		qWarning() << "Cannot load database: version '" << lDatabaseVersion << "' not supported: " << pFilePath;
		return false;
	}

	if(lDatabaseVersion >= 10)
	{
		pSerializer.Serialize(mUsefullExifVersion);
	}
	else
	{
		if constexpr (SERIALIZER::msIsReading)
		{
			mUsefullExifVersion = 1;
		}
	}

	pSerializer.Serialize(mFolders);
	pSerializer.Serialize(mAllCameraModels);
	pSerializer.Serialize(ESFocalLengthIn35mmStat::msCameraModelsTo35mmFocalFactors);
	pSerializer.Serialize(mAllLensModels);
	if (lDatabaseVersion >= 6)
		pSerializer.Serialize(mAllTags);
	if(lDatabaseVersion >= 9)
		pSerializer.Serialize(mLastAssignedId);

	auto lSerializeFileInfo = [&](ESFileInfo& pFileInfo)
		{
			if (lDatabaseVersion >= 12)
				pSerializer.Serialize(pFileInfo.mHash);
			if (lDatabaseVersion >= 9)
				pSerializer.Serialize(pFileInfo.mId);
			pSerializer.Serialize(pFileInfo.mFilePath);
			pSerializer.Serialize(pFileInfo.mCameraModelIdx);
			pSerializer.Serialize(pFileInfo.mLensModelIdx);
			pSerializer.Serialize(pFileInfo.mReadResult);

			pSerializer.Serialize(pFileInfo.mExif.mDateTime);
			pSerializer.Serialize(pFileInfo.mExif.mFNumber);
			pSerializer.Serialize(pFileInfo.mExif.mFocalLength);
			pSerializer.Serialize(pFileInfo.mExif.mFocalLengthIn35mm);
			pSerializer.Serialize(pFileInfo.mExif.mGeoLocation.mLatitude);
			pSerializer.Serialize(pFileInfo.mExif.mGeoLocation.mLongitude);
			if (lDatabaseVersion >= 11)
				pSerializer.Serialize(pFileInfo.mExif.mGeoLocationGuessed);
			pSerializer.Serialize(pFileInfo.mExif.mShutterSpeedValue);
			if (lDatabaseVersion >= 5)
				pSerializer.Serialize(pFileInfo.mExif.mOrientation);
			if (lDatabaseVersion >= 10)
			{
				pSerializer.Serialize(pFileInfo.mExif.mISOSpeedRatings);
				pSerializer.Serialize(pFileInfo.mExif.mWidth);
				pSerializer.Serialize(pFileInfo.mExif.mHeight);
			}
			if (lDatabaseVersion >= 6)
			{
				pSerializer.Serialize(pFileInfo.mTagsGenerated);
				pSerializer.Serialize(pFileInfo.mTagIndexes);
				if (lDatabaseVersion >= 7)
					pSerializer.Serialize(pFileInfo.mEmbeddings);
			}

			if constexpr (SERIALIZER::msIsReading)
			{
				if (lDatabaseVersion < 9)
				{
					pFileInfo.mId = ++mLastAssignedId;
				}

				if (pFileInfo.mCameraModelIdx != std::numeric_limits<decltype(pFileInfo.mCameraModelIdx)>::max())
					pFileInfo.mExif.mCameraModel = mAllCameraModels[pFileInfo.mCameraModelIdx];
				if (pFileInfo.mLensModelIdx != std::numeric_limits<decltype(pFileInfo.mLensModelIdx)>::max())
					pFileInfo.mExif.mLensModel = mAllLensModels[pFileInfo.mLensModelIdx];

				pFileInfo.mResolutionStr = ESStringId(QString::number(pFileInfo.mExif.mWidth) + " x " + QString::number(pFileInfo.mExif.mHeight));

				if (pFileInfo.mEmbeddings.size() > 0)
				{
					if (mEmbeddingsDimension == 0)
						mEmbeddingsDimension = int(pFileInfo.mEmbeddings.size());
					assert(mEmbeddingsDimension == pFileInfo.mEmbeddings.size());
				}

				if (lDatabaseVersion < 12)
					pFileInfo.computeHash();
			}
		};

	if (lDatabaseVersion < 13)
	{
		auto lSerializeFileInfoOld = [&](ESFileInfoId& pFileInfoId, ESFileInfo& pFileInfo)
		{
			lSerializeFileInfo(pFileInfo);
			if constexpr (SERIALIZER::msIsReading)
				pFileInfoId = pFileInfo.mId;
			else
				Q_UNUSED(pFileInfoId);
		};
		std::map<ESFileInfoId, ESFileInfo> lFilesMap;
		pSerializer.SerializeCustom(lFilesMap, lSerializeFileInfoOld);

		mFiles.reserve(lFilesMap.size());
		for (const auto& [lFileInfoId, lFileInfo] : lFilesMap)
			mFiles.emplace_back(lFileInfo);
	}
	else
	{
		pSerializer.SerializeCustom(mFiles, lSerializeFileInfo);
	}

	if constexpr (SERIALIZER::msIsReading)
	{
		for(int i = 0 ; i < mFiles.size() ; ++i)
		{
			const ESFileInfo& lFileInfo = mFiles[i];
			mIdToIndex[mFiles[i].mId] = i;
			mFilesPathToIndex[lFileInfo.mFilePath] = i;
			if (!lFileInfo.mHash.isEmpty())
				mFilesHashToIndex.emplace(lFileInfo.mHash, i);
		}
	}

	return true;
}

/********************************************************************************/

void ESDatabase::saveDatabase() const
{
#ifndef EXIFSTATS_READONLY
	std::shared_lock lLock(mFilesMutex);

	QString lDataBaseDir = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
	QString lDataBasePath = lDataBaseDir + QDir::separator() + "database.esdb";
	QString lDataBasePathTmp = lDataBasePath + ".tmp";

	QFile lDataBaseFile(lDataBasePathTmp);
	if (!lDataBaseFile.open(QIODevice::WriteOnly))
	{
		qWarning() << "Cannot save database: failed to open database file: " << lDataBasePath;
		return;
	}

	ESSerializer<false> lSerializer(&lDataBaseFile);
	if(!const_cast<ESDatabase*>(this)->Serialize(lSerializer, lDataBasePath))
		return;

	lDataBaseFile.close();

	if (QFile::exists(lDataBasePath) && !QFile::remove(lDataBasePath))
	{
		qWarning() << "Failed to delete the database file.";
		return;
	}
	if(!QFile::rename(lDataBasePathTmp, lDataBasePath))
	{
		qWarning() << "Failed to rename the temp database file.";
		return;
	}

	QSettings lSettings;
	lSettings.setValue("DataBasePath", lDataBasePath);
#endif // EXIFSTATS_READONLY
}

/********************************************************************************/

QString ESDatabase::getDatabaseFilePath() const
{
	QSettings lSettings;
	return lSettings.value("DataBasePath").toString();
}

/********************************************************************************/

void ESDatabase::loadDatabase()
{
	assert(mFiles.size() == 0 && "Database already loaded");

	// Settings
	QSettings lSettings;
#ifdef EXIFSTATS_READONLY
	#ifdef Q_OS_ANDROID
		QString lDataBasePath = lSettings.value(msReadOnlyDatabaseFolderSettingsKey).toString();
	#else
		QString lDataBasePath = lSettings.value(msReadOnlyDatabaseFolderSettingsKey).toString() + "database.esdb";
	#endif
#else
	QString lDataBasePath = lSettings.value("DataBasePath").toString();
#endif
	if(lDataBasePath.isEmpty())
		return;

	// Open database
#if defined(EXIFSTATS_READONLY) && defined(Q_OS_ANDROID)
	QuaZip lZip(lDataBasePath);
	if (!lZip.open(QuaZip::mdUnzip))
	{
		qWarning() << "Cannot open ExifStats archive file";
		return;
	}
	if (!lZip.setCurrentFile("database.esdb"))
	{
		qWarning() << "File 'database.esdb' not found in ExifStats archive file";
		return;
	}
	QuaZipFile lDataBaseFile(&lZip);
#else
	QFile lDataBaseFile(lDataBasePath);
#endif

	if (!lDataBaseFile.open(QIODevice::ReadOnly))
	{
		qWarning() << "Cannot load database: failed to open database file: " << lDataBasePath;
		return;
	}

	ESSerializer<true> lSerializer(&lDataBaseFile);
	Serialize(lSerializer, lDataBasePath);

	std::sort(mFolders.begin(), mFolders.end());
	QStringList::iterator lLast = std::unique(mFolders.begin(), mFolders.end());
	mFolders.erase(lLast, mFolders.end());

	setProperty("Processing", false);
	emit tagsChanged();
	emit dataChanged();

	qInfo() << "Database loaded with " << mFiles.size() << " files";

	if(mUsefullExifVersion != USEFULLEXIF_VERSION)
	{
		QTimer::singleShot(1000,[this]()
		{
			updateDatabase(mFolders, false, false);
		});
	}
}

/********************************************************************************/

const QVector<QString>& ESDatabase::getFolders() const
{
	return mFolders;
}

/********************************************************************************/

std::vector<const ESFileInfo*> ESDatabase::getFileInfoFromHash(QString pHash) const
{
	std::vector<const ESFileInfo*> lResult;
	if(pHash.size() > 10)
	{
		auto lRange = mFilesHashToIndex.equal_range(pHash);
		for (auto lItFiles = lRange.first; lItFiles != lRange.second; ++lItFiles)
			lResult.push_back(&mFiles[lItFiles->second]);
	}
	return lResult;
}

/********************************************************************************/

ESFileInfo* ESDatabase::getFileInfo(ESStringId pFile)
{
	ESFileInfo* lResult = nullptr;
	auto&& lIdItFound = mFilesPathToIndex.find(pFile);
	if (lIdItFound != mFilesPathToIndex.end())
	{
		return &mFiles[lIdItFound->second];
	}
	return lResult;
}

/********************************************************************************/

const ESFileInfo* ESDatabase::getFileInfo(ESStringId pFile) const
{
	return const_cast<ESDatabase*>(this)->getFileInfo(pFile);
}

/********************************************************************************/

ESFileInfo* ESDatabase::getFileInfo(ESFileInfoId pFile)
{
	ESFileInfo* lResult = nullptr;
	auto lItFound = mIdToIndex.find(pFile);
	if (lItFound != mIdToIndex.end())
		lResult = &mFiles[lItFound->second];
	return lResult;
}

/********************************************************************************/

const ESFileInfo* ESDatabase::getFileInfo(ESFileInfoId pFile) const
{
	return const_cast<ESDatabase*>(this)->getFileInfo(pFile);
}

/********************************************************************************/

std::shared_mutex& ESDatabase::getFilesMutex() const
{
	return mFilesMutex;
}

/********************************************************************************/

bool ESDatabase::isUnlockDatabaseRequested() const
{
	return mUnlockDatabaseRequested;
}

/********************************************************************************/

const std::vector<ESFileInfo>& ESDatabase::getFiles() const
{
	return mFiles;
}

/********************************************************************************/

const QVector<QString>& ESDatabase::getAllLensModels() const
{
	return mAllLensModels;
}

/********************************************************************************/

const QVector<QString>& ESDatabase::getAllCameraModels() const
{
	return mAllCameraModels;
}

/********************************************************************************/

void ESDatabase::getAllTags(std::vector<QString>& pOutput)
{
	std::shared_lock lLock(mFilesMutex);
	pOutput = mAllTags;
}

/********************************************************************************/

void ESDatabase::setAllTags(const std::vector<QString>& pAllTags)
{
	mAllTags = pAllTags;
	emit tagsChanged();
}

/********************************************************************************/

QStringList ESDatabase::getTagsLabels(const std::vector<uint16_t>& pTags)
{
	std::shared_lock lLock(mFilesMutex);
	QStringList lResult;
	for (uint16_t lTag : pTags)
	{
		if (lTag < mAllTags.size())
			lResult.push_back(mAllTags[lTag]);
		else
			lResult.push_back(QString("UnknownTag%1").arg(lTag));
	}
	return lResult;
}

/********************************************************************************/

QString ESDatabase::getTagLabel(uint16_t pTagIndex) const
{
	std::shared_lock lLock(mFilesMutex);
	return mAllTags[pTagIndex];
}

/********************************************************************************/

int ESDatabase::getEmbeddingsDimension() const
{
	return mEmbeddingsDimension;
}

/********************************************************************************/

void ESDatabase::setEmbeddingsDimension(int pEmbeddingsDimension)
{
	assert(mEmbeddingsDimension == 0);
	mEmbeddingsDimension = pEmbeddingsDimension;
}
