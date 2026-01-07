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

class ESImage : public QObject, public std::enable_shared_from_this<ESImage>
{
	friend class ESImageCache;

	Q_OBJECT
public:
	/********************************* METHODS ***********************************/

	bool isLoading() const;
	bool isLoaded() const;
	bool isNull() const;

	const QImage& getImage() const;
	StringId getImagePath() const;
	const UsefullExif& getExif() const;
	
	bool hasCacheFile() const;

	void updateLastUsed();
	void loadImage();

signals:
	/********************************** SIGNALS ***********************************/

	void imageLoadedOrCanceled(ESImage* pSender);

private:
	/******************************** ATTRIBUTES **********************************/

	mutable qint64 mLastUsed;
	mutable bool mCacheFileChecked;
	mutable bool mHasCacheFile;
	mutable QChar mDriveLetter;
	StringId mImagePath;
	QString mImageCachePath;
	std::atomic_bool mIsQueueForLoading;
	std::atomic_bool mIsLoading;
	std::atomic_bool mIsLoaded;
	std::atomic_bool mCancelLoading;
	QImage mImage;
	QByteArray mImageFileData;

	UsefullExif mExif;

	/********************************* METHODS ***********************************/

	explicit ESImage(const StringId pImagePath, const QString pImageCachePath, const UsefullExif* pImageExif);

	void loadImageInternal(const QSize pMaxSize, bool pAsync, std::atomic_int32_t* pNumAsyncTaskStarted);
	void readImage(const QString& pImagePath, QSize aMaxSize);
	void readImage(QByteArray& pImageData, QSize aMaxSize);
	void readImage(QImageReader& pImageReader, QSize aMaxSize);

	void cancelLoading();
	void unloadImage();

	QChar getDriveLetter() const;
};
