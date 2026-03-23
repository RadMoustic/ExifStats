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
	return mImagesLoadingCount != mImagesLoadedCount;
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

/*virtual*/ void ESImageLoader::queueImageLoading(const std::shared_ptr<ESImage>& pImage, bool pUseCacheDriveQueueIfAvailable)
{
	if(pImage->getImagePath().getString().isEmpty())
		return;

	++mImagesLoadingCount;

	emit imageLoadingProgress(mImagesLoadedCount, mImagesLoadingCount);
	
	QChar lDriveLetter = pUseCacheDriveQueueIfAvailable ? pImage->getDriveLetter() : pImage->getImagePath().getString()[0];

	std::shared_ptr<LoadingThreadTask> lDriveLoadingTask;
	mDriveLoadingTasksMutex.lock_shared();
	auto lItFound = mDriveLoadingTasks.find(lDriveLetter);
	if(lItFound == mDriveLoadingTasks.end())
	{
		mDriveLoadingTasksMutex.unlock_shared();
		std::unique_lock<std::shared_mutex> lUniqueLock(mDriveLoadingTasksMutex);
		// Double check
		lItFound = mDriveLoadingTasks.find(lDriveLetter);
		if(lItFound == mDriveLoadingTasks.end())
		{
			lDriveLoadingTask = std::make_shared<LoadingThreadTask>();
			lDriveLoadingTask->mMaxAsyncTask = mMaxAsyncTask;
			lDriveLoadingTask->init([this](const std::shared_ptr<ESImage>& pImage, std::atomic_int32_t& pNumAsyncTaskStarted)
				{
					internalLoadImage(pImage, pNumAsyncTaskStarted);
				});
			mDriveLoadingTasks[lDriveLetter] = lDriveLoadingTask;
		}
		lUniqueLock.unlock();
	}
	else
	{
		lDriveLoadingTask = lItFound->second;
		mDriveLoadingTasksMutex.unlock_shared();
	}

	lDriveLoadingTask->processImage(pImage);
}

/********************************************************************************/

void ESImageLoader::LoadingThreadTask::processImage(const std::shared_ptr<ESImage>& pImage)
{
	mStop = false;
	{
		std::lock_guard<std::mutex> lLock(mQueueMutex);
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
					std::shared_ptr<ESImage> lCurrentImage;
					{
						std::lock_guard<std::mutex> lLock(mQueueMutex);
						if (mLoadingQueue.empty())
						{
							break;
						}
						else
						{
							lCurrentImage = mLoadingQueue.front();
							mLoadingQueue.pop_front();
						}
					}

					mProcessFct(lCurrentImage, mNumAsyncTaskStarted);
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
	std::lock_guard<std::mutex> lLock(mQueueMutex);
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
	int lCurrentLoadingCount = mImagesLoadingCount.load();
	int lCurrentLoadedCount = mImagesLoadedCount.load();
	if(lCurrentLoadingCount == lCurrentLoadedCount || lCurrentLoadingCount == 0)
	{
		mImagesLoadingCount -= lCurrentLoadingCount;
		mImagesLoadedCount -= lCurrentLoadedCount;
	}

	emit imageLoadingProgress(mImagesLoadedCount, mImagesLoadingCount);
}

/********************************************************************************/

void ESImageLoader::stopAndCancelAllLoadings()
{
	std::unique_lock<std::shared_mutex> lDriveLock(mDriveLoadingTasksMutex);
	for(auto lDriveLoadingTask: mDriveLoadingTasks)
		lDriveLoadingTask.second->stop();

	int lCurrentLoadingCount = mImagesLoadingCount.load();
	if (lCurrentLoadingCount > 0)
	{
		mImagesLoadingCount = 0;
		mImagesLoadedCount = 0;

		emit imageLoadingProgress(mImagesLoadedCount, mImagesLoadingCount);
	}
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