#include "ESImageCache.h"

// ES
#include "ESUtils.h"
#include "ESDatabase.h"

// Qt
#include <QStandardPaths>
#include <QDir>
#include <QCryptographicHash>
#include <QtConcurrent>

// Quazip
#ifdef Q_OS_ANDROID
#include <quazip/quazip.h>
#include <quazip/quazipfile.h>
#endif // Q_OS_ANDROID

// Stl
#include <set>

/********************************************************************************/

static const char* CACHE_VERSION = "3";
static const QString CACHE_IMAGE_FILE_EXTENSION = ".escache";
#ifdef Q_OS_ANDROID
static const int MAX_NUM_IMAGE_LOADED = 128;
#else
static const int MAX_NUM_IMAGE_LOADED = 700;
#endif

/********************************************************************************/

/*static*/ ESImageCache& ESImageCache::getInstance()
{
	static ESImageCache lsInstance;
	return lsInstance;
}

/********************************************************************************/

ESImageCache::ESImageCache()
	: mIsUpdating(false)
{
	mMaxAsyncTask = QThreadPool::globalInstance()->maxThreadCount();

#ifdef EXIFSTATS_READONLY
	#ifdef Q_OS_ANDROID
		QString lDataBaseDir = "";
	#else
		QSettings lSettings;
		QString lDataBaseDir = lSettings.value(ESDatabase::msReadOnlyDatabaseFolderSettingsKey).toString() + QDir::separator();
	#endif
#else
	QString lDataBaseDir = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + QDir::separator();
#endif
	mCacheFolderPath = lDataBaseDir + CACHE_IMAGE_FOLDER_NAME;
	QDir lDir;
	lDir.mkpath(mCacheFolderPath);

	mCacheLoadingTask.mMaxAsyncTask = mMaxAsyncTask;
	mCacheLoadingTask.init([this](const std::shared_ptr<ESImage>& pImage, std::atomic_int32_t& pNumAsyncTaskStarted)
		{
			++pNumAsyncTaskStarted;
			pImage->loadImageInternal(QSize(CACHE_IMAGE_SIZE, CACHE_IMAGE_SIZE), true, &pNumAsyncTaskStarted);
			unloadUnusedImages();
		});
}

/********************************************************************************/

void ESImageCache::initializeFromDatabase()
{
	assert(!mIsUpdating);

	mIsUpdating = true;

	std::vector<std::shared_ptr<ESImage>> lImagesToInitializeCacheFileCheck;
	{
		std::lock_guard<std::shared_mutex> lLock(mImagesMutex);
		const ESDatabase& lDatabase = ESDatabase::getInstance();
		mImages.clear();
		mImages.reserve(lDatabase.getFiles().size());
		
		for(const auto& [lFileInfoId, lFileInfo] : lDatabase.getFiles())
		{
			std::shared_ptr<ESImage> lImage(new ESImage(lFileInfo.mFilePath, getCacheFilePath(lFileInfo.mFilePath), &lFileInfo.mExif));
			mImages.emplace(std::make_pair(lFileInfo.mFilePath, lImage));
			lImagesToInitializeCacheFileCheck.push_back(lImage);
		}
	}

	mIsUpdating = false;
	emit updateFinished();
		
	queueImageCaching(lImagesToInitializeCacheFileCheck);

	(void)connect(&ESDatabase::getInstance(), &ESDatabase::dataChanged, this, &ESImageCache::onDatabaseDataChanged, Qt::QueuedConnection);
}

/********************************************************************************/

void ESImageCache::onDatabaseDataChanged()
{
	assert(!mIsUpdating);
	mIsUpdating = true;

	(void)QtConcurrent::run([this]()
		{	
			std::vector<std::shared_ptr<ESImage>> lImagesToInitializeCacheFileCheck;
			{
				std::lock_guard<std::shared_mutex> lLock(mImagesMutex);

				const ESDatabase& lDatabase = ESDatabase::getInstance();
				mImages.reserve(lDatabase.getFiles().size());

				for (const auto& [lFileInfoId, lFileInfo] : lDatabase.getFiles())
				{
					std::shared_ptr<ESImage>& lImage = mImages[lFileInfo.mFilePath];
					if (!lImage)
					{
						lImage.reset(new ESImage(lFileInfo.mFilePath, getCacheFilePath(lFileInfo.mFilePath), &lFileInfo.mExif));
						lImagesToInitializeCacheFileCheck.push_back(lImage);
					}
				}
			}

			mIsUpdating = false;
			emit updateFinished();

			queueImageCaching(lImagesToInitializeCacheFileCheck);
		});
}

/********************************************************************************/

void ESImageCache::queueImageCaching(std::vector<std::shared_ptr<ESImage>>& pImages)
{
	std::sort(pImages.begin(), pImages.end(),
		[](const std::shared_ptr<ESImage>& a, const std::shared_ptr<ESImage>& b)
		{
			return a->getExif().mDateTime < b->getExif().mDateTime;
		});

#ifndef EXIFSTATS_READONLY
	// Initialize cache file after emitting the signal to avoid delaying UI startup
	for (std::shared_ptr<ESImage>& lImage : pImages)
	{
		if (!lImage->hasCacheFile()) // Slow so initialize that too
			queueImageLoading(lImage, true);
	}
#endif // EXIFSTATS_READONLY
}

/********************************************************************************/

bool ESImageCache::isUpdating() const
{
	return mIsUpdating;
}

/********************************************************************************/

void ESImageCache::resetSearchSimilarityScores()
{
	std::shared_lock lLock(mImagesMutex);
	for(const auto& [lImagePath, lImage]: mImages)
		lImage->mCurrentSearchSimilarity = 0.f;
}

/********************************************************************************/

std::shared_ptr<ESImage> ESImageCache::getImage(ESStringId pImagePath)
{
	std::shared_lock lLock(mImagesMutex);
	auto lItFound = mImages.find(pImagePath);
	std::shared_ptr<ESImage> lResult;
	if(lItFound != mImages.end())
		lResult = lItFound->second;
	return lResult;
}

/********************************************************************************/

QByteArray ESImageCache::getImageHash(const QString& pImagePath)
{
	QCryptographicHash lHasher(QCryptographicHash::Sha1);

	lHasher.addData(CACHE_VERSION);
	lHasher.addData(TOSTR(CACHE_IMAGE_SIZE));
	lHasher.addData(TOSTR(CACHE_IMAGE_JPEG_COMPRESSION));
	lHasher.addData(pImagePath.toUtf8());

	return lHasher.result();
}

/********************************************************************************/

QString ESImageCache::getCacheFilePath(const QString& pImagePath)
{
	QByteArray lHash = getImageHash(pImagePath);
	QString lHexHash = QString::fromUtf8(lHash.toHex());
	return mCacheFolderPath + QDir::separator() + lHexHash + CACHE_IMAGE_FILE_EXTENSION;
}

/********************************************************************************/

/*virtual*/ void ESImageCache::queueImageLoading(const std::shared_ptr<ESImage>& pImage, bool pUseCacheDriveQueueIfAvailable) /*override*/
{
	if (pImage->isLoading() || pImage->isLoaded())
		return;

	pImage->mCancelLoading = false; // The ONLY ALLOWED place to reset the cancel loading state
	pImage->mIsQueueForLoading = true;

	if (pImage->hasCacheFile())
	{
		mCacheLoadingTask.processImage(pImage);
		return;
	}
#ifndef EXIFSTATS_READONLY
	else
	{
		ESImageLoader::queueImageLoading(pImage, pUseCacheDriveQueueIfAvailable);
	}
#else
	(void)pUseCacheDriveQueueIfAvailable;
#endif // EXIFSTATS_READONLY
}

/********************************************************************************/

void ESImageCache::unloadUnusedImages()
{
	if (mImages.size() <= MAX_NUM_IMAGE_LOADED || mUnloadingUnusedThread.isRunning())
		return;
	mUnloadingUnusedThread = QtConcurrent::run([this]()
		{
			std::vector<std::pair<qint64, std::shared_ptr<ESImage>>> lLoadedImages;
			lLoadedImages.reserve(MAX_NUM_IMAGE_LOADED);
			{
				std::shared_lock<std::shared_mutex> lLock(mImagesMutex);
				for (const auto& [lImagePath, lImage] : mImages)
					if (lImage->isLoaded() || (lImage->isLoading() && lImage->hasCacheFile())) // Don't cancel files queued for caching
						lLoadedImages.emplace_back(lImage->mLastUsed, lImage); // Store mLastUsed because if it is changed during the sort later by another thread it will crash
			}
			if (lLoadedImages.size() <= MAX_NUM_IMAGE_LOADED)
				return;
			std::sort(lLoadedImages.begin(), lLoadedImages.end(),
				[](const std::pair<qint64, std::shared_ptr<ESImage>>& a, const std::pair<qint64, std::shared_ptr<ESImage>>& b)
				{
					return a.first > b.first;
				});
			for (size_t i = MAX_NUM_IMAGE_LOADED; i < lLoadedImages.size(); ++i)
			{
				lLoadedImages[i].second->unloadImage();
			}
		});
}

/********************************************************************************/

/*virtual*/ void ESImageCache::stopAndCancelAllLoadings() /*override*/
{
	mCacheLoadingTask.stop();

	ESImageLoader::stopAndCancelAllLoadings();

	std::shared_lock<std::shared_mutex> lImagesLock(mImagesMutex);
	for (const auto& [lImagePath, lImage] : mImages)
		lImage->cancelLoading();
}

/********************************************************************************/

/*virtual*/ void ESImageCache::internalLoadImage(const std::shared_ptr<ESImage>& pImage, std::atomic_int32_t& pNumAsyncTaskStarted) /*override*/
{
	++pNumAsyncTaskStarted;
	pImage->loadImageInternal(QSize(CACHE_IMAGE_SIZE, CACHE_IMAGE_SIZE), true, &pNumAsyncTaskStarted);
	unloadUnusedImages();
}

/********************************************************************************/

#ifdef QT_DEBUG

void ESImageCache::printImageDebugInfo(const std::shared_ptr<ESImage>& pImage)
{
	// Lock Everything
	std::lock_guard<std::shared_mutex> lDriveLock(mDriveLoadingTasksMutex);
	std::shared_lock<std::shared_mutex> lImagesLock(mImagesMutex);
	for (const auto& [lDriveLetter, lDriveLoadingTask] : mDriveLoadingTasks)
		lDriveLoadingTask->mQueueMutex.lock();

	qDebug() << "Image path:" << pImage->getImagePath();
	if (pImage->isLoaded())
		qDebug() << " - Loaded in memory";
	else if (pImage->mIsLoading)
		qDebug() << " - Loading in progress";
	else if (pImage->mIsQueueForLoading)
		qDebug() << " - Loading queued";
	else
		qDebug() << " - Not loaded";
	if (pImage->hasCacheFile())
		qDebug() << " - Has cache file";
	else
		qDebug() << " - No cache file";

	qDebug() << " - Found in loading queues:";
	mCacheLoadingTask.printImageDebugInfo("Cache", pImage);
	for (const auto& [lDriveLetter, lDriveLoadingTask] : mDriveLoadingTasks)
		lDriveLoadingTask->printImageDebugInfo(QString("Drive %1").arg(lDriveLetter), pImage);

	// Unlock Everything
	for (const auto& [lDriveLetter, lDriveLoadingTask] : mDriveLoadingTasks)
		lDriveLoadingTask->mQueueMutex.unlock();

}
#endif
