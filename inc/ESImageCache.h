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
#include <queue>
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
	bool isInitialized() const;
	std::shared_ptr<ESImage> getImage(StringId pImagePath);

signals:
	/********************************** SIGNALS ***********************************/

	void initializationFinished();

private:
	/******************************** ATTRIBUTES **********************************/

	std::unordered_map<StringId, std::shared_ptr<ESImage>> mImages;
	std::shared_mutex mImagesMutex;
	QString mCacheFolderPath;

	class LoadingThreadTask
	{
	public:
		void init(std::function<void(std::shared_ptr<ESImage>)> pProcessFct)
		{
			assert(!mProcessFct);
			mProcessFct = std::move(pProcessFct);
		}
		void processImage(const std::shared_ptr<ESImage>& pImage);

	private:
		QFuture<void> mLoadingThread;
		std::queue<std::shared_ptr<ESImage>> mLoadingQueue;
		std::mutex mQueueMutex;
		std::function<void(std::shared_ptr<ESImage>)> mProcessFct;
	};

	std::map<QChar, std::shared_ptr<LoadingThreadTask>> mDriveLoadingTasks;
	std::shared_mutex mDriveLoadingTasksMutex;
	LoadingThreadTask mQueueLoadingTask;

	QFuture<void> mUnloadingUnusedThread;
	bool mIsInitialized;

	/********************************* METHODS ***********************************/

	ESImageCache();
	QByteArray getImageHash(const QString& pImagePath);
	QString getCacheFilePath(const QString& pImagePath);
	void queueImageLoading(const std::shared_ptr<ESImage>& pImage);
	void queueDriveImageLoading(const std::shared_ptr<ESImage>& pImage);
	void unloadUnusedImages();

};

