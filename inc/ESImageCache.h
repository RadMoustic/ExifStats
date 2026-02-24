#pragma once

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

// ES
#include "ESImageLoader.h"

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

#define CACHE_IMAGE_SIZE 250

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
	std::shared_ptr<ESImage> getImage(StringId pImagePath);
	virtual void stopAndCancelAllLoadings() override;

#ifdef QT_DEBUG
	void printImageDebugInfo(const std::shared_ptr<ESImage>& pImage);
#endif

signals:
	/********************************** SIGNALS ***********************************/

	void updateFinished();

protected:
	/******************************** ATTRIBUTES **********************************/

	std::unordered_map<StringId, std::shared_ptr<ESImage>> mImages;
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
	void onDatabaseFoldersChanged();
	void queueImageCaching(std::vector<std::shared_ptr<ESImage>>& pImages);
	virtual void queueImageLoading(const std::shared_ptr<ESImage>& pImage) override;
	virtual void internalLoadImage(const std::shared_ptr<ESImage>& pImage, std::atomic_int32_t& pNumAsyncTaskStarted) override;
};

