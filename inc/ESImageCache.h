#pragma once

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

// ES
#include "ESImageLoader.h"

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

#define CACHE_IMAGE_SIZE 640
#define CACHE_IMAGE_JPEG_COMPRESSION 50

class ESDatabase;

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

class ESImageCache : public ESImageLoader
{
	friend class ESImage;

	Q_OBJECT
public:
	/******************************** ATTRIBUTES **********************************/

	/********************************* METHODS ***********************************/

	static ESImageCache& getInstance();

	void initializeFromDatabase();
	bool isUpdating() const;
	void resetSearchSimilarityScores();
	std::shared_ptr<ESImage> getImage(ESStringId pImagePath);
	virtual void stopAndCancelAllLoadings() override;

#ifdef QT_DEBUG
	void printImageDebugInfo(const std::shared_ptr<ESImage>& pImage);
#endif

signals:
	/********************************** SIGNALS ***********************************/

	void updateFinished();

protected:
	/******************************** ATTRIBUTES **********************************/

	std::unordered_map<ESStringId, std::shared_ptr<ESImage>> mImages;
	std::shared_mutex mImagesMutex;
	QString mCacheFolderPath;
	bool mIsUpdating;

	LoadingThreadTask mCacheLoadingTask; // Cache files are loaded in another thread to not be blocked by drive loading tasks

	QFuture<void> mUnloadingUnusedThread;

	/********************************* METHODS ***********************************/

	ESImageCache();
	QByteArray getImageHash(const QString& pImagePath);
	QString getCacheFilePath(const QString& pImagePath);
	void unloadUnusedImages();
	void onDatabaseDataChanged();
	void queueImageCaching(std::vector<std::shared_ptr<ESImage>>& pImages);
	virtual void queueImageLoading(const std::shared_ptr<ESImage>& pImage, bool pUseCacheDriveQueueIfAvailable) override;
	virtual void internalLoadImage(const std::shared_ptr<ESImage>& pImage, std::atomic_int32_t& pNumAsyncTaskStarted) override;
};

