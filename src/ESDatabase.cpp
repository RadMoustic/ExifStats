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
constexpr uint DATABASE_VERSION = 10;
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
				mFiles.clear();
				mFolders.clear();
			}

			QVector<ESFileInfoId> lAllImageFileIds;

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
					auto&& [lItFound, lIsNewFile] = mFilesPathToId.try_emplace(lFilePath, 0);

					ESFileInfoId lFileInfoId = lIsNewFile ? ++mLastAssignedId : lItFound->second;
					ESFileInfo& lFileInfo = mFiles[lFileInfoId];
					if(lIsNewFile)
					{
						lFileInfo.mId = lFileInfoId;
						lFileInfo.mFilePath = lFilePath;
						lItFound->second = lFileInfoId;
					}
					if (!pNewFilesOnly || lIsNewFile || lFileInfo.mReadResult != eSuccess)
						lAllImageFileIds << lFileInfoId;
				}

				if (!mFolders.contains(*lFolderPath))
					mFolders.append(*lFolderPath);
			}

			setProcessingProgress(0.f);

			mProcessedFilesCounter = 0;

			constexpr uint cNbFilesPerThread = 256;

			QFuture<void> lRes = QtConcurrent::map(lAllImageFileIds,
				[&](const ESFileInfoId& pFileInfoId)
				{
					ESFileInfo& lFileInfo = mFiles[pFileInfoId];

					easyexif::EXIFInfo lExifData;
					lFileInfo.mReadResult = readFileExif(lFileInfo.mFilePath, lExifData);
					if (lFileInfo.mReadResult != eSuccess)
						return;

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
			mFilesMutex.unlock();

			// Extract all camera models and counter
			std::unordered_set<ESStringId> lCameraModels;
			std::unordered_set<ESStringId> lLensModels;
			for (std::pair<const ESFileInfoId, ESFileInfo>& lProcessedFile : mFiles)
			{
				if (lProcessedFile.second.mReadResult == eSuccess)
				{
					lCameraModels.insert(lProcessedFile.second.mExif.mCameraModel);
					lLensModels.insert(lProcessedFile.second.mExif.mLensModel);
				}
			}
			mAllCameraModels.assign(lCameraModels.begin(), lCameraModels.end());
			mAllLensModels.assign(lLensModels.begin(), lLensModels.end());

			// Set the camera model idx
			{
				int lCameraModelIdx = 0;
				for (auto&& lItCamera : lCameraModels)
				{
					for (auto&& lProcessedFile : mFiles)
						if (lProcessedFile.second.mExif.mCameraModel == lItCamera)
							lProcessedFile.second.mCameraModelIdx = lCameraModelIdx;

					++lCameraModelIdx;
				}
			}

			// Set the lens model idx
			{
				int lLensModelIdx = 0;
				for (auto&& lItLens : lLensModels)
				{
					for (auto&& lProcessedFile : mFiles)
						if (lProcessedFile.second.mExif.mLensModel == lItLens)
							lProcessedFile.second.mLensModelIdx = lLensModelIdx;

					++lLensModelIdx;
				}
			}

			mUsefullExifVersion = USEFULLEXIF_VERSION;

			setProcessing(false);
			emit dataChanged();
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

	pSerializer.SerializeCustom(mFiles,
		[&](ESFileInfoId& pFileInfoId, ESFileInfo& pFileInfo)
		{
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
			pSerializer.Serialize(pFileInfo.mExif.mShutterSpeedValue);
			if(lDatabaseVersion >= 5)
				pSerializer.Serialize(pFileInfo.mExif.mOrientation);
			if (lDatabaseVersion >= 10)
			{
				pSerializer.Serialize(pFileInfo.mExif.mISOSpeedRatings);
				pSerializer.Serialize(pFileInfo.mExif.mWidth);
				pSerializer.Serialize(pFileInfo.mExif.mHeight);
			}
			if(lDatabaseVersion >= 6)
			{
				pSerializer.Serialize(pFileInfo.mTagsGenerated);
				pSerializer.Serialize(pFileInfo.mTagIndexes);
				if (lDatabaseVersion >= 7)
				{
#if defined(EXIFSTATS_READONLY) && defined(HNSWLIB_ENABLED) && false // Disabled because we need them for similar image search feature
					if (mEmbeddingsDimension == 0)
						pSerializer.Serialize(pFileInfo.mEmbeddings);
					else
						pSerializer.Skip(pFileInfo.mEmbeddings);
#else
					pSerializer.Serialize(pFileInfo.mEmbeddings);
#endif // defined(EXIFSTATS_READONLY) && defined(HNSWLIB_ENABLED)
				}
			}

			if constexpr (SERIALIZER::msIsReading)
			{
				if (lDatabaseVersion < 9)
				{
					pFileInfo.mId = ++mLastAssignedId;
				}

				pFileInfoId = pFileInfo.mId;

				if(pFileInfo.mCameraModelIdx != std::numeric_limits<decltype(pFileInfo.mCameraModelIdx)>::max())
					pFileInfo.mExif.mCameraModel = mAllCameraModels[pFileInfo.mCameraModelIdx];
				if (pFileInfo.mLensModelIdx != std::numeric_limits<decltype(pFileInfo.mLensModelIdx)>::max())
					pFileInfo.mExif.mLensModel = mAllLensModels[pFileInfo.mLensModelIdx];

				pFileInfo.mResolutionStr = ESStringId(QString::number(pFileInfo.mExif.mWidth) + " x " + QString::number(pFileInfo.mExif.mHeight));

				if (pFileInfo.mEmbeddings.size() > 0)
				{
					if(mEmbeddingsDimension == 0)
						mEmbeddingsDimension = int(pFileInfo.mEmbeddings.size());
					assert(mEmbeddingsDimension == pFileInfo.mEmbeddings.size());
				}

				mFilesPathToId[pFileInfo.mFilePath] = pFileInfo.mId;
			}
			else
			{
				(void)pFileInfoId;
			}
		});

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
	// Clear
	mAllCameraModels.clear();
	mAllLensModels.clear();
	mFiles.clear();
	mFilesPathToId.clear();

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

ESFileInfo* ESDatabase::getFileInfo(ESStringId pFile)
{
	ESFileInfo* lResult = nullptr;
	auto&& lIdItFound = mFilesPathToId.find(pFile);
	if (lIdItFound != mFilesPathToId.end())
	{
		auto lItFound = mFiles.find(lIdItFound->second);
		if (lItFound != mFiles.end())
			lResult = &lItFound->second;
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
	auto lItFound = mFiles.find(pFile);
	if (lItFound != mFiles.end())
		lResult = &lItFound->second;
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

const std::map<ESFileInfoId, ESFileInfo>& ESDatabase::getFiles() const
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
