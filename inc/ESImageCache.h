#pragma once

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

// ES
#include "ESStringPool.h"
#include "ESImage.h"

// Qt
#include <QObject>
#include <QImage>
#include <QFuture>

// Stl
#include <deque>
#include <mutex>
#include <shared_mutex>

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

#define CACHE_IMAGE_SIZE 250

class ESDatabase;

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

class ESImageCache : public QObject
{
	friend class ESImage;

	Q_OBJECT
public:
	/******************************** ATTRIBUTES **********************************/

	/********************************* METHODS ***********************************/

	static ESImageCache& getInstance();

	void initializeFromDatabase();
	bool isUpdating() const;
	std::shared_ptr<ESImage> getImage(StringId pImagePath);
	void stopAndCancelAllLoadings();

#ifdef QT_DEBUG
	void printImageDebugInfo(const std::shared_ptr<ESImage>& pImage);
#endif

signals:
	/********************************** SIGNALS ***********************************/

	void updateFinished();
	void imageCachingProgressUpdated(int pCachedCount, int pCachingCount);

private:
	/******************************** ATTRIBUTES **********************************/

	std::unordered_map<StringId, std::shared_ptr<ESImage>> mImages;
	std::shared_mutex mImagesMutex;
	QString mCacheFolderPath;

	class LoadingThreadTask
	{
#ifdef QT_DEBUG
		friend void ESImageCache::printImageDebugInfo(const std::shared_ptr<ESImage>& pImage);
#endif
		
	public:
		void init(std::function<void(std::shared_ptr<ESImage>)> pProcessFct)
		{
			assert(!mProcessFct);
			mProcessFct = std::move(pProcessFct);
		}
		void processImage(const std::shared_ptr<ESImage>& pImage);
		void stop();

#ifdef QT_DEBUG
		void printImageDebugInfo(const QString& pTaskName, const std::shared_ptr<ESImage>& pImage);
#endif
		
private:
		std::mutex mQueueMutex;
		QFuture<void> mLoadingThread;
		std::deque<std::shared_ptr<ESImage>> mLoadingQueue;
		std::function<void(std::shared_ptr<ESImage>)> mProcessFct;
		bool mStop = false;
	};

	std::map<QChar, std::shared_ptr<LoadingThreadTask>> mDriveLoadingTasks;
	std::shared_mutex mDriveLoadingTasksMutex;
	LoadingThreadTask mCacheLoadingTask; // Cache files are loaded in another thread to not be blocked by drive loading tasks

	QFuture<void> mUnloadingUnusedThread;
	std::atomic_bool mIsUpdating;
	std::atomic_int mImagesCachingCount;
	std::atomic_int mImagesCachedCount;

	/********************************* METHODS ***********************************/

	ESImageCache();
	QByteArray getImageHash(const QString& pImagePath);
	QString getCacheFilePath(const QString& pImagePath);
	void queueImageLoading(const std::shared_ptr<ESImage>& pImage);
	void unloadUnusedImages();
	void imageCachingFinished();
	void onDatabaseFoldersChanged();
	void queueImageCaching(std::vector<std::shared_ptr<ESImage>>& pImages);
};

