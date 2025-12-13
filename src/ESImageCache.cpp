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
	: mIsInitialized(false)
{
	QString lDataBaseDir = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
	mCacheFolderPath = lDataBaseDir + QDir::separator() + "ImageCache";
	QDir lDir;
	lDir.mkpath(mCacheFolderPath);

    mQueueLoadingTask.init([this](const std::shared_ptr<ESImage>& pImage)
        {
            queueDriveImageLoading(pImage);
        });
}

/********************************************************************************/

void ESImageCache::initializeFromDatabase()
{
	assert(mImages.empty()); // Initialize only once at startup

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
    mIsInitialized = true;
    emit initializationFinished();

	// Initialize cache file after emitting the signal to avoid delaying UI startup
	for (std::shared_ptr<ESImage>& lImage : lImagesToInitializeCacheFileCheck)
    {
        if(!lImage->hasCacheFile()) // Slow so initialize that too
            queueImageLoading(lImage);
    }
}

/********************************************************************************/

bool ESImageCache::isInitialized() const
{
	return mIsInitialized;
}

/********************************************************************************/

std::shared_ptr<ESImage> ESImageCache::getImage(StringId pImagePath)
{
    std::shared_ptr<ESImage> lResult;

    mImagesMutex.lock_shared();
	auto itFound = mImages.find(pImagePath);
	if (itFound == mImages.end())
	{
		mImagesMutex.unlock_shared();
		std::lock_guard<std::shared_mutex> lock(mImagesMutex);
		// Double check
		itFound = mImages.find(pImagePath);
        if (itFound == mImages.end())
		{
            const FileInfo* lFileInfo = ESDatabase::getInstance().getFileInfo(pImagePath);
            lResult.reset(new ESImage(pImagePath, getCacheFilePath(pImagePath), lFileInfo ? &lFileInfo->mExif : nullptr));
            mImages.emplace(std::make_pair(pImagePath, lResult));
        }
        else
        {
            lResult = itFound->second;
        }
    }
    else
    {
        lResult = itFound->second;
        mImagesMutex.unlock_shared();
	}

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
    pImage->mCancelLoading = false; // The ONLY ALLOWED place to reset the cancel loading state
    pImage->mIsQueueForLoading = true;

    if (pImage->isLoaded())
        return;

    mQueueLoadingTask.processImage(pImage);
}

/********************************************************************************/

void ESImageCache::queueDriveImageLoading(const std::shared_ptr<ESImage>& pImage)
{
    if (pImage->isLoaded())
        return;
	
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
        mLoadingQueue.push(pImage);
    }
    if (mLoadingThread.isRunning())
        return;
    mLoadingThread = QtConcurrent::run([this]()
        {
            while (!mLoadingQueue.empty())
            {
                std::shared_ptr<ESImage> currentImage;
                {
                    std::lock_guard<std::mutex> lock(mQueueMutex);
                    currentImage = mLoadingQueue.front();
                    mLoadingQueue.pop();
                }

                mProcessFct(currentImage);
            }
        });
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
                    if (lImage.second->isLoaded() || (lImage.second->mIsLoading && lImage.second->hasCacheFile())) // Don't unload or cancel image queued for loading with no cache file yet
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