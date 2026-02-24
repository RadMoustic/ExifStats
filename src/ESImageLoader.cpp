#include "ESImageLoader.h"

// Qt
#include <QtConcurrent>

/********************************************************************************/

ESImageLoader::ESImageLoader()
	: mPaused(false)
{
}

/********************************************************************************/

bool ESImageLoader::isLoading() const
{
	return mImagesLoadingCount == mImagesLoadedCount;
}

/********************************************************************************/

void ESImageLoader::setPaused(bool pPaused)
{
	if(mPaused != pPaused)
	{
		mPaused = pPaused;

		std::shared_lock<std::shared_mutex> lDriveLock(mDriveLoadingTasksMutex);
		for(auto lDriveLoadingTask: mDriveLoadingTasks)
			lDriveLoadingTask.second->setPaused(mPaused);
	}
}

/********************************************************************************/

/*virtual*/ void ESImageLoader::queueImageLoading(const std::shared_ptr<ESImage>& pImage)
{
	if(pImage->getImagePath().getString().isEmpty())
		return;

	++mImagesLoadingCount;

	emit imageLoadingProgress(mImagesLoadedCount, mImagesLoadingCount);
	
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
			driveLoadingTask->mMaxAsyncTask = mMaxAsyncTask;
			driveLoadingTask->init([this](const std::shared_ptr<ESImage>& pImage, std::atomic_int32_t& pNumAsyncTaskStarted)
				{
					internalLoadImage(pImage, pNumAsyncTaskStarted);
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

void ESImageLoader::LoadingThreadTask::processImage(const std::shared_ptr<ESImage>& pImage)
{
	mStop = false;
	{
		std::lock_guard<std::mutex> lock(mQueueMutex);
		mLoadingQueue.push_back(pImage);
	}
	if (mPaused || mLoadingThread.isRunning())
		return;
	start();
}

/********************************************************************************/

void ESImageLoader::LoadingThreadTask::start()
{
	mLoadingThread = QtConcurrent::run([this]()
		{
			while (!mStop && !mPaused)
			{
				assert(mMaxAsyncTask > 0 && "mMaxAsyncTask not set");
				if(mNumAsyncTaskStarted < mMaxAsyncTask)
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

					mProcessFct(currentImage, mNumAsyncTaskStarted);
				}
				else
				{
					QThread::sleep(std::chrono::nanoseconds(500000));
				}
			}
		});
}

/********************************************************************************/

void ESImageLoader::LoadingThreadTask::stop()
{
	mStop = true;
	mLoadingThread.cancel();
	std::lock_guard<std::mutex> lock(mQueueMutex);
	mLoadingQueue.clear();
}

/********************************************************************************/

void ESImageLoader::LoadingThreadTask::setPaused(bool pPaused)
{
	if(mPaused != pPaused)
	{
		mPaused = pPaused;
		if(mPaused)
			mLoadingThread.cancel();
		else
			start();
	}
}

/********************************************************************************/

/*virtual*/ void ESImageLoader::imageLoadingFinished()
{
	++mImagesLoadedCount;
	int currentCachingCount = mImagesLoadingCount.load();
	int currentCachedCount = mImagesLoadedCount.load();
	if(currentCachingCount == currentCachedCount)
	{
		mImagesLoadingCount -= currentCachingCount;
		mImagesLoadedCount -= currentCachedCount;
	}

	emit imageLoadingProgress(mImagesLoadedCount, mImagesLoadingCount);
}

/********************************************************************************/

void ESImageLoader::stopAndCancelAllLoadings()
{
	std::unique_lock<std::shared_mutex> lDriveLock(mDriveLoadingTasksMutex);
	for(auto lDriveLoadingTask: mDriveLoadingTasks)
		lDriveLoadingTask.second->stop();
}

/********************************************************************************/

#ifdef QT_DEBUG

void ESImageLoader::LoadingThreadTask::printImageDebugInfo(const QString& pTaskName, const std::shared_ptr<ESImage>& pImage)
{
	auto lItFound = std::find(mLoadingQueue.begin(), mLoadingQueue.end(), pImage);
	if (lItFound != mLoadingQueue.end())
	{
		qDebug() << "    - " << pTaskName << "[" << std::distance(mLoadingQueue.begin(), lItFound) << "]";
	}
}

#endif