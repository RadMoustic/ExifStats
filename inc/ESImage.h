#pragma once

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

// ES
#include "ESStringPool.h"
#include "ESFileInfo.h"

// Qt
#include <QObject>
#include <QImage>
#include <QImageReader>

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

class QuaZip;

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

class ESImage : public QObject, public std::enable_shared_from_this<ESImage>
{
	friend class ESImageCache;

	Q_OBJECT
public:
	/******************************** ATTRIBUTES **********************************/

	float mCurrentSearchSimilarity;

	/********************************* METHODS ***********************************/

	bool isLoading() const;
	bool isLoaded() const;
	bool isNull() const;

	QChar getDriveLetter() const;
	std::shared_ptr<const QImage> getImage() const;
	ESStringId getImagePath() const;
	QString getImageCachePath() const;
	const ESUsefullExif& getExif() const;
	
	bool hasCacheFile() const;

	void updateLastUsed();
	void loadImage();

#ifdef Q_OS_ANDROID
	static void closeExifStatsArchive();
#endif

signals:
	/********************************** SIGNALS ***********************************/

	void imageLoadedOrCanceled(ESImage* pSender);

private:
	/******************************** ATTRIBUTES **********************************/

	mutable qint64 mLastUsed;
	mutable std::atomic_bool mCacheFileChecked;
	mutable std::atomic_bool mHasCacheFile;
	mutable QChar mDriveLetter;
	ESStringId mImagePath;
	QString mImageCachePath;
	std::atomic_bool mIsQueueForLoading;
	std::atomic_bool mIsLoading;
	std::atomic_bool mIsLoaded;
	std::atomic_bool mCancelLoading;
	std::shared_ptr<QImage> mImage;
	QByteArray mImageFileData;

	ESUsefullExif mExif;

#ifdef Q_OS_ANDROID
	static int msCloseZipIdx;
	static thread_local int mstZipIdx;
	static thread_local QuaZip* mstZip;
#endif

	/********************************* METHODS ***********************************/

	explicit ESImage(const ESStringId pImagePath, const QString pImageCachePath, const ESUsefullExif* pImageExif);

	void loadImageInternal(const QSize pMaxSize, bool pAsync, std::atomic_int32_t* pNumAsyncTaskStarted);
	void readImage(const QString& pImagePath, QSize pMaxSize);
	void readImage(QByteArray& pImageData, QSize pMaxSize);
	void readImage(QImageReader& pImageReader, QSize pMaxSize);

	void cancelLoading();
	void unloadImage();

#ifdef Q_OS_ANDROID
	static QuaZip* getExifStatsArchive();
#endif
};
