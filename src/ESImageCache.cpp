#include "ESImageCache.h"

// ES
#include "ESUtils.h"
#include "ESDatabase.h"

// Qt
#include <QStandardPaths>
#include <QDir>
#include <QCryptographicHash>
#include <QtConcurrent>

// Stl
#include <set>

/********************************************************************************/

static const char* CACHE_VERSION = "3";
static const QString CACHE_IMAGE_FILE_EXTENSION = ".escache";
static const int MAX_NUM_IMAGE_LOADED = 512;

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
	QString lDataBaseDir = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
	mCacheFolderPath = lDataBaseDir + QDir::separator() + "ImageCache";
	QDir lDir;
	lDir.mkpath(mCacheFolderPath);


	mCacheLoadingTask.init([this](const std::shared_ptr<ESImage>& pImage)
		{
			pImage->loadImageInternal(QSize(CACHE_IMAGE_SIZE, CACHE_IMAGE_SIZE), true);
			unloadUnusedImages();
		});
}

/********************************************************************************/

void ESImageCache::initializeFromDatabase()
{
	assert(!mIsUpdating);
	assert(mImages.empty()); // Initialize only once at startup

	mIsUpdating = true;

	std::vector<std::shared_ptr<ESImage>> lImagesToInitializeCacheFileCheck;
	{
		std::lock_guard<std::shared_mutex> lock(mImagesMutex);
		const ESDatabase& lDatabase = ESDatabase::getInstance();
		mImages.reserve(lDatabase.getFiles().size());
		
		for(auto&& lFileInfo: lDatabase.getFiles())
		{
			std::shared_ptr<ESImage> lImage(new ESImage(lFileInfo.first, getCacheFilePath(lFileInfo.first), &lFileInfo.second.mExif));
			mImages.emplace(std::make_pair(lFileInfo.first, lImage));
			lImagesToInitializeCacheFileCheck.push_back(lImage);
		}
	}

	mIsUpdating = false;
	emit updateFinished();
		
	queueImageCaching(lImagesToInitializeCacheFileCheck);

	(void)connect(&ESDatabase::getInstance(), &ESDatabase::foldersChanged, this, &ESImageCache::onDatabaseFoldersChanged);
}

/********************************************************************************/

void ESImageCache::onDatabaseFoldersChanged()
{
	assert(!mIsUpdating);
	mIsUpdating = true;

	QtConcurrent::run([this]()
		{	
			std::vector<std::shared_ptr<ESImage>> lImagesToInitializeCacheFileCheck;
			{
				std::lock_guard<std::shared_mutex> lock(mImagesMutex);

				const ESDatabase& lDatabase = ESDatabase::getInstance();
				mImages.reserve(lDatabase.getFiles().size());

				for (auto&& lFileInfo : lDatabase.getFiles())
				{
					std::shared_ptr<ESImage>& lImage = mImages[lFileInfo.first];
					if (!lImage)
					{
						lImage.reset(new ESImage(lFileInfo.first, getCacheFilePath(lFileInfo.first), &lFileInfo.second.mExif));
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
			return a->getExifDateTime() < b->getExifDateTime();
		});

	// Initialize cache file after emitting the signal to avoid delaying UI startup
	for (std::shared_ptr<ESImage>& lImage : pImages)
	{
		lImage->updateLastUsed();
		if (!lImage->hasCacheFile()) // Slow so initialize that too
			queueImageLoading(lImage);
	}
}

/********************************************************************************/

bool ESImageCache::isUpdating() const
{
	return mIsUpdating;
}

/********************************************************************************/

std::shared_ptr<ESImage> ESImageCache::getImage(StringId pImagePath)
{
	std::shared_lock lock(mImagesMutex);
	auto itFound = mImages.find(pImagePath);
	std::shared_ptr<ESImage> lResult;
	if(itFound != mImages.end())
		lResult = itFound->second;
	return lResult;
}

/********************************************************************************/

QByteArray ESImageCache::getImageHash(const QString& pImagePath)
{
	QCryptographicHash lHasher(QCryptographicHash::Sha1);

	lHasher.addData(CACHE_VERSION);
	lHasher.addData(TOSTR(CACHE_IMAGE_SIZE));
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

void ESImageCache::queueImageLoading(const std::shared_ptr<ESImage>& pImage)
{
	if (pImage->isLoading() || pImage->isLoaded())
		return;

	pImage->mCancelLoading = false; // The ONLY ALLOWED place to reset the cancel loading state
	pImage->mIsQueueForLoading = true;

	if(pImage->hasCacheFile())
	{
		mCacheLoadingTask.processImage(pImage);
		return;
	}
	else
	{
		++mImagesCachingCount;
	}
	
	QChar lDriveLetter = pImage->getDriveLetter();

	std::shared_ptr<LoadingThreadTask> driveLoadingTask;
	mDriveLoadingTasksMutex.lock_shared();
	auto itFound = mDriveLoadingTasks.find(lDriveLetter);
	if(itFound == mDriveLoadingTasks.end())
	{
		mDriveLoadingTasksMutex.unlock_shared();
		std::unique_lock<std::shared_mutex> uniqueLock(mDriveLoadingTasksMutex);
		// Double check
		itFound = mDriveLoadingTasks.find(lDriveLetter);
		if(itFound == mDriveLoadingTasks.end())
		{
			driveLoadingTask = std::make_shared<LoadingThreadTask>();
			driveLoadingTask->init([this](const std::shared_ptr<ESImage>& pImage)
				{
					pImage->loadImageInternal(QSize(CACHE_IMAGE_SIZE, CACHE_IMAGE_SIZE), true);
					unloadUnusedImages();
				});
			mDriveLoadingTasks[lDriveLetter] = driveLoadingTask;
		}
		uniqueLock.unlock();
	}
	else
	{
		driveLoadingTask = itFound->second;
		mDriveLoadingTasksMutex.unlock_shared();
	}

	driveLoadingTask->processImage(pImage);
}

/********************************************************************************/

void ESImageCache::LoadingThreadTask::processImage(const std::shared_ptr<ESImage>& pImage)
{
	{
		std::lock_guard<std::mutex> lock(mQueueMutex);
		mLoadingQueue.push_back(pImage);
	}
	if (mLoadingThread.isRunning())
		return;
	mLoadingThread = QtConcurrent::run([this]()
		{
			while (!mStop)
			{
				std::shared_ptr<ESImage> currentImage;
				{
					std::lock_guard<std::mutex> lock(mQueueMutex);
					if (mLoadingQueue.empty())
					{
						break;
					}
					else
					{
						currentImage = mLoadingQueue.front();
						mLoadingQueue.pop_front();
					}
				}

				mProcessFct(currentImage);
			}
		});
}

/********************************************************************************/

void ESImageCache::LoadingThreadTask::stop()
{
	mStop = true;
	mLoadingThread.cancel();
	std::lock_guard<std::mutex> lock(mQueueMutex);
	mLoadingQueue.clear();
}

/********************************************************************************/

void ESImageCache::unloadUnusedImages()
{
	if (mImages.size() <= MAX_NUM_IMAGE_LOADED || mUnloadingUnusedThread.isRunning())
		return;
	mUnloadingUnusedThread = QtConcurrent::run([this]()
		{
			std::vector<std::shared_ptr<ESImage>> lLoadedImages;
			lLoadedImages.reserve(MAX_NUM_IMAGE_LOADED);
			{
				std::lock_guard<std::shared_mutex> lock(mImagesMutex);
				for (auto&& lImage : mImages)
					if (lImage.second->isLoaded() || (lImage.second->isLoading() && !lImage.second->hasCacheFile())) // Don't cancel files queued for caching
						lLoadedImages.push_back(lImage.second);
			}
			if (lLoadedImages.size() <= MAX_NUM_IMAGE_LOADED)
				return;
			std::sort(lLoadedImages.begin(), lLoadedImages.end(),
				[](const std::shared_ptr<ESImage>& a, const std::shared_ptr<ESImage>& b)
				{
					return a->mLastUsed > b->mLastUsed;
				});
			for (size_t i = MAX_NUM_IMAGE_LOADED; i < lLoadedImages.size(); ++i)
			{
				lLoadedImages[i]->unloadImage();
			}
		});
}

/********************************************************************************/

void ESImageCache::imageCachingFinished()
{
	++mImagesCachedCount;
	int currentCachingCount = mImagesCachingCount.load();
	int currentCachedCount = mImagesCachedCount.load();
	if(currentCachingCount == currentCachedCount)
	{
		mImagesCachingCount -= currentCachingCount;
		mImagesCachedCount -= currentCachedCount;
	}

	emit imageCachingProgressUpdated(mImagesCachedCount, mImagesCachingCount);
}

/********************************************************************************/

void ESImageCache::stopAndCancelAllLoadings()
{
	mCacheLoadingTask.stop();

	std::unique_lock<std::shared_mutex> lDriveLock(mDriveLoadingTasksMutex);
	for(auto lDriveLoadingTask: mDriveLoadingTasks)
		lDriveLoadingTask.second->stop();

	std::shared_lock<std::shared_mutex> lImagesLock(mImagesMutex);
	for (auto&& lImage : mImages)
		lImage.second->cancelLoading();
}

/********************************************************************************/

#ifdef QT_DEBUG

void ESImageCache::LoadingThreadTask::printImageDebugInfo(const QString& pTaskName, const std::shared_ptr<ESImage>& pImage)
{
	auto lItFound = std::find(mLoadingQueue.begin(), mLoadingQueue.end(), pImage);
	if (lItFound != mLoadingQueue.end())
	{
		qDebug() << "    - " << pTaskName << "[" << std::distance(mLoadingQueue.begin(), lItFound) << "]";
	}
}

void ESImageCache::printImageDebugInfo(const std::shared_ptr<ESImage>& pImage)
{
	// Lock Everything
	std::unique_lock<std::shared_mutex> lDriveLock(mDriveLoadingTasksMutex);
	std::shared_lock<std::shared_mutex> lImagesLock(mImagesMutex);
	for (auto lDriveLoadingTask : mDriveLoadingTasks)
		lDriveLoadingTask.second->mQueueMutex.lock();

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
	for (auto lDriveLoadingTask : mDriveLoadingTasks)
		lDriveLoadingTask.second->printImageDebugInfo(QString("Drive %1").arg(lDriveLoadingTask.first), pImage);

	// Unlock Everything
	for (auto lDriveLoadingTask : mDriveLoadingTasks)
		lDriveLoadingTask.second->mQueueMutex.unlock();

}
#endif