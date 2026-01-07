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

// Stl
#include <set>

/********************************************************************************/

constexpr uint DATABASE_MAGIC_NUMBER = 0xEACDEACD;
constexpr uint DATABASE_VERSION = 5;

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
{
}

/********************************************************************************/

void ESDatabase::refresh(bool pFullRefresh)
{
	addFolders(mFolders, pFullRefresh, !pFullRefresh);
}

/********************************************************************************/

void ESDatabase::clear()
{
	mFiles.clear();
	mFolders.clear();
	mAllLensModels.clear();
	mAllCameraModels.clear();
	mProcessedFilesCounter = 0;
	ExifStatCountFocalLengthIn35mm::msCameraModelsTo35mmFocalFactors.clear();

	emit foldersChanged();
}

/********************************************************************************/

void ESDatabase::addFolder(const QUrl& pFolderPath, bool pClearDB)
{
	addFolders(QStringList(pFolderPath.toLocalFile()), pClearDB, false);
}

/********************************************************************************/

void ESDatabase::addFolders(const QStringList& pFolders, bool pClearDB, bool pNewFilesOnly)
{
	setProcessing(true);
	setProcessingProgress(0.f);
	
	QtConcurrent::run([this, pFolders, pClearDB, pNewFilesOnly]()
		{
			if (pClearDB)
			{
				mFiles.clear();
				mFolders.clear();
			}

			QVector<StringId> lAllImageFiles;

			std::set<const QString*> lUniqueFolders;
			for (const QString& lFolderPath : pFolders)
				lUniqueFolders.insert(&lFolderPath);

			for(const QString* lFolderPath: lUniqueFolders)
			{
				QDir lDir(*lFolderPath);

				// Directories
				QDirIterator lDirIt(*lFolderPath, { "*.jpg", "*.heic"}, QDir::Files, QDirIterator::Subdirectories);
				while (lDirIt.hasNext())
				{
					StringId lFilePath = lDirIt.next();
					FileInfo& lFileInfo = mFiles[lFilePath];
					if(!pNewFilesOnly || !lFileInfo.mFilePath.isValid() || lFileInfo.mReadResult != eSuccess)
						lAllImageFiles << lFilePath;
				}

				if (!mFolders.contains(*lFolderPath))
					mFolders.append(*lFolderPath);
			}

			setProcessingProgress(0.f);

			mProcessedFilesCounter = 0;

			constexpr uint cNbFilesPerThread = 256;

			QFuture<void> res = QtConcurrent::map(lAllImageFiles,
				[&](const StringId& pFilePath)
				{
					FileInfo& lFileInfo = mFiles[pFilePath];
					lFileInfo.mFilePath = pFilePath;

					easyexif::EXIFInfo lExifData;
					lFileInfo.mReadResult = readFileExif(pFilePath, lExifData);
					if (lFileInfo.mReadResult != eSuccess)
						return;

					lFileInfo.mExif = convertToUsefullExif(lExifData);

					int lProcessedFiles = mProcessedFilesCounter.fetch_add(1);

					if (mProgressMutex.tryLock())
					{
						float lNewProgress = float(lProcessedFiles) / float(lAllImageFiles.size());
						if (lNewProgress - mProcessingProgress > 0.001)
							setProcessingProgress(lNewProgress);
						mProgressMutex.unlock();
					}
				});
			res.waitForFinished();

			// Extract all camera models and counter
			std::unordered_set<StringId> lCameraModels;
			std::unordered_set<StringId> lLensModels;
			for (std::pair<const StringId, FileInfo>& lProcessedFile : mFiles)
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
				for (auto&& itCamera : lCameraModels)
				{
					for (auto&& lProcessedFile : mFiles)
						if (lProcessedFile.second.mExif.mCameraModel == itCamera)
							lProcessedFile.second.mCameraModelIdx = lCameraModelIdx;

					++lCameraModelIdx;
				}
			}

			// Set the lens model idx
			{
				int lLensModelIdx = 0;
				for (auto&& itLens : lLensModels)
				{
					for (auto&& lProcessedFile : mFiles)
						if (lProcessedFile.second.mExif.mLensModel == itLens)
							lProcessedFile.second.mLensModelIdx = lLensModelIdx;

					++lLensModelIdx;
				}
			}

			setProcessing(false);
			emit foldersChanged();
		});
}

/********************************************************************************/

ReadExifFileResult ESDatabase::readFileExif(const QString& pFilePath, easyexif::EXIFInfo& pOutExif)
{
	QFile lFile(pFilePath);
	if (!lFile.exists())
		return eFileNotFound;

	if (!lFile.open(QIODevice::ReadOnly))
		return eCantOpenFile;

	int lFileSize = lFile.size();

	thread_local int tHeaderSize = 0;
	thread_local std::unique_ptr<char[]> tHeaderBuffer;
	auto lAllocateHeaderBuffer = [&](int pSize)
	{
		if(pSize > tHeaderSize)
		{
			tHeaderSize = pSize;
			tHeaderBuffer = std::make_unique<char[]>(tHeaderSize);
		}
	};

	if (pFilePath.right(5).toLower() == ".heic")
	{
		// We don't really read the HEIC struct but just try to find the 'Exif\0\0' header, so read a big chunk
		lAllocateHeaderBuffer(64000);
		
		int lReadSize = std::min(tHeaderSize, lFileSize);
		if (lFile.read(tHeaderBuffer.get(), lReadSize) != lReadSize)
			return eFailedToRead;

		// Find Exif Header
		std::string_view lFullHeaderBufferView(tHeaderBuffer.get(), tHeaderSize);
		size_t lOffset = lFullHeaderBufferView.find("Exif\0\0MM", 0, 8);
		if(lOffset == std::string_view::npos)
			return eParseExifErrorNoExif;

		// Parse EXIF
		return static_cast<ReadExifFileResult>(pOutExif.parseFromEXIFSegment(reinterpret_cast<unsigned char*>(&tHeaderBuffer[lOffset]), static_cast<unsigned int>(tHeaderSize - lOffset)));
	}
	else
	{
		constexpr int cFirstReadSize = 32;
		lAllocateHeaderBuffer(cFirstReadSize);

		int lReadSize = std::min(cFirstReadSize, lFileSize);
		if (lFile.read(tHeaderBuffer.get(), lReadSize) != lReadSize)
			return eFailedToRead;
	
		// Find Exif Header
		int lOffset = 0;  // current offset into buffer
		for (lOffset = 0; lOffset < lReadSize - 1; lOffset++)
			if (uchar(tHeaderBuffer[lOffset]) == 0xFF && uchar(tHeaderBuffer[lOffset + 1]) == 0xE1)
				break;

		if (lOffset + 4 > lReadSize)
			return eBufferTooSmallToReadExifSize;

		// Read Exif Size
		lOffset += 2;
		unsigned short lExifSize = static_cast<uint16_t>(*(tHeaderBuffer.get() + lOffset) << 8) | *(tHeaderBuffer.get() + lOffset + 1);

		if (lExifSize < 16)
			return eExifSizeTooSmall;

		int lHeaderIncludingExifSize = lOffset + lExifSize;

		// Read the header including the full exif data
		lAllocateHeaderBuffer(lHeaderIncludingExifSize);
		lFile.seek(0);
		if (lFile.read(tHeaderBuffer.get(), lHeaderIncludingExifSize) != lHeaderIncludingExifSize)
			return eFailedToRead;

		// Parse EXIF
		return static_cast<ReadExifFileResult>(pOutExif.parseFrom(reinterpret_cast<unsigned char *>(tHeaderBuffer.get()), lHeaderIncludingExifSize));
	}
}

/********************************************************************************/

UsefullExif ESDatabase::convertToUsefullExif(const easyexif::EXIFInfo& aFullExif)
{
	UsefullExif result;

	result.mCameraModel = QString(aFullExif.Model.c_str());
	result.mLensModel = QString(aFullExif.LensInfo.Model.c_str());
	result.mFNumber = aFullExif.FNumber;
	QDateTime exifDateTime = QDateTime::fromString(QString(aFullExif.DateTimeOriginal.c_str()), "yyyy:MM:dd hh:mm:ss");
	if(!exifDateTime.isValid())
		exifDateTime = QDateTime::fromString(QString(aFullExif.DateTime.c_str()), "yyyy:MM:dd hh:mm:ss");
	result.mDateTime = exifDateTime.toSecsSinceEpoch();
	result.mGeoLococation.mLatitude = static_cast<float>(aFullExif.GeoLocation.Latitude);
	result.mGeoLococation.mLongitude = static_cast<float>(aFullExif.GeoLocation.Longitude);
	result.mFocalLength = aFullExif.FocalLength;
	result.mFocalLengthIn35mm = aFullExif.FocalLengthIn35mm;
	result.mOrientation = ESExifOrientation(aFullExif.Orientation);

	return result;
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

	pSerializer.Serialize(mFolders);
	pSerializer.Serialize(mAllCameraModels);
	pSerializer.Serialize(ExifStatCountFocalLengthIn35mm::msCameraModelsTo35mmFocalFactors);
	pSerializer.Serialize(mAllLensModels);

	pSerializer.SerializeCustom(mFiles,
		[&](StringId& pStringId, FileInfo& pFileInfo)
		{
			pSerializer.Serialize(pFileInfo.mFilePath);
			pSerializer.Serialize(pFileInfo.mCameraModelIdx);
			pSerializer.Serialize(pFileInfo.mLensModelIdx);
			pSerializer.Serialize(pFileInfo.mReadResult);
			
			pSerializer.Serialize(pFileInfo.mExif.mDateTime);
			pSerializer.Serialize(pFileInfo.mExif.mFNumber);
			pSerializer.Serialize(pFileInfo.mExif.mFocalLength);
			pSerializer.Serialize(pFileInfo.mExif.mFocalLengthIn35mm);
			pSerializer.Serialize(pFileInfo.mExif.mGeoLococation.mLatitude);
			pSerializer.Serialize(pFileInfo.mExif.mGeoLococation.mLongitude);
			pSerializer.Serialize(pFileInfo.mExif.mShutterSpeedValue);
			if(lDatabaseVersion >= 5)
				pSerializer.Serialize(pFileInfo.mExif.mOrientation);

			if constexpr (pSerializer.msIsReading)
			{
				pStringId = pFileInfo.mFilePath;

				if(pFileInfo.mCameraModelIdx != std::numeric_limits<decltype(pFileInfo.mCameraModelIdx)>::max())
					pFileInfo.mExif.mCameraModel = mAllCameraModels[pFileInfo.mCameraModelIdx];
				if (pFileInfo.mLensModelIdx != std::numeric_limits<decltype(pFileInfo.mLensModelIdx)>::max())
					pFileInfo.mExif.mLensModel = mAllLensModels[pFileInfo.mLensModelIdx];
			}
			else
			{
				(void)pStringId;
			}
		}
	);

	return true;
}

/********************************************************************************/

void ESDatabase::saveDatabase()
{
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
	if(!Serialize(lSerializer, lDataBasePath))
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
}

/********************************************************************************/

void ESDatabase::loadDatabase()
{
	// Clear
	mAllCameraModels.clear();
	mAllLensModels.clear();
	mFiles.clear();

	// Settings
	QSettings lSettings;
	QString lDataBasePath = lSettings.value("DataBasePath").toString();
	if(lDataBasePath.isEmpty())
		return;

	// Open database
	QFile lDataBaseFile(lDataBasePath);
	if (!lDataBaseFile.open(QIODevice::ReadOnly))
	{
		qWarning() << "Cannot load database: failed to open database file: " << lDataBasePath;
		return;
	}

	ESSerializer<true> lSerializer(&lDataBaseFile);
	Serialize(lSerializer, lDataBasePath);

	std::sort(mFolders.begin(), mFolders.end());
	QStringList::iterator last = std::unique(mFolders.begin(), mFolders.end());
	mFolders.erase(last, mFolders.end());

	setProperty("Processing", false);
	emit foldersChanged();
}

/********************************************************************************/

const QVector<QString>& ESDatabase::getFolders() const
{
	return mFolders;
}

/********************************************************************************/

const FileInfo* ESDatabase::getFileInfo(StringId pFile) const
{
	const FileInfo* lResult = nullptr;
	auto itFound = mFiles.find(pFile);
	if (itFound != mFiles.end())
		lResult = &itFound->second;
	return lResult;
}

/********************************************************************************/

const std::map<StringId, FileInfo>& ESDatabase::getFiles() const
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
