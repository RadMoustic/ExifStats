#pragma once

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

// ES
#include "ESImage.h"

// Qt
#include <QObject>
#include <QFuture>

// Stl
#include <deque>
#include <mutex>
#include <shared_mutex>

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

class ESImageLoader : public QObject
{
	Q_OBJECT
public:
	/******************************** ATTRIBUTES **********************************/

	/********************************* METHODS ***********************************/

	bool isLoading() const;
	virtual void stopAndCancelAllLoadings();

	void setPaused(bool pPaused);

signals:
	/********************************** SIGNALS ***********************************/

	void imageLoadingProgress(int pCachedCount, int pCachingCount);

protected:
	/********************************** TYPES *************************************/

	class LoadingThreadTask
	{
	public:
		using ProcessFunction = std::function<void(std::shared_ptr<ESImage>, std::atomic_int32_t&)>;

		void init(ProcessFunction pProcessFct)
		{
			assert(!mProcessFct);
			mProcessFct = std::move(pProcessFct);
		}
		void processImage(const std::shared_ptr<ESImage>& pImage);
		void stop();
		void setPaused(bool pPaused);
		void start();

#ifdef QT_DEBUG
		void printImageDebugInfo(const QString& pTaskName, const std::shared_ptr<ESImage>& pImage);
#endif

		std::mutex mQueueMutex;
		QFuture<void> mLoadingThread;
		std::deque<std::shared_ptr<ESImage>> mLoadingQueue;
		ProcessFunction mProcessFct;
		std::atomic_bool mStop = false;
		std::atomic_bool mPaused = false;
		std::atomic_int32_t mNumAsyncTaskStarted = 0;
		int mMaxAsyncTask;
	};

	/******************************** ATTRIBUTES **********************************/

	std::map<QChar, std::shared_ptr<LoadingThreadTask>> mDriveLoadingTasks;
	std::shared_mutex mDriveLoadingTasksMutex;

	QFuture<void> mUnloadingUnusedThread;
	std::atomic_bool mIsLoading;
	std::atomic_int mImagesLoadingCount;
	std::atomic_int mImagesLoadedCount;
	std::atomic_bool mPaused;
	int mMaxAsyncTask = 0;

	/********************************* METHODS ***********************************/

	ESImageLoader();

	virtual void queueImageLoading(const std::shared_ptr<ESImage>& pImage);
	virtual void imageLoadingFinished();
	virtual void internalLoadImage(const std::shared_ptr<ESImage>& pImage, std::atomic_int32_t& pNumAsyncTaskStarted) = 0;
};

