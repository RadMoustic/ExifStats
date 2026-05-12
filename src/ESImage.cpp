#include "ESImage.h"

// ES
#include "ESImageCache.h"
#include "ESDatabase.h"

// Qt
#include <QFile>
#include <QImageReader>
#include <QtConcurrent>

// Quazip
#ifdef Q_OS_ANDROID
#include <quazip/quazip.h>
#include <quazip/quazipfile.h>
#endif // Q_OS_ANDROID

// TurboJPEG
#ifdef TURBOJPEG_PLUGIN_ENABLED
#include <turbojpeg.h>
#endif

/********************************************************************************/

#ifdef Q_OS_ANDROID
QuaZip* getExifStatsArchive()
{
	static thread_local QuaZip* lsZip = nullptr;
	if(lsZip == nullptr)
	{
		QSettings lSettings;
		lsZip = new QuaZip(lSettings.value(ESDatabase::msReadOnlyDatabaseFolderSettingsKey).toString());
		if (!lsZip->open(QuaZip::mdUnzip))
		{
			qWarning() << "Cannot open ExifStats archive file";
		}
	}
	return lsZip;
}
#endif // Q_OS_ANDROID

/********************************************************************************/

#ifdef TURBOJPEG_PLUGIN_ENABLED
bool loadTurboJpeg(QImage& pImageOut, const QByteArray& pImageData)
{
	tjhandle lHandle = tjInitDecompress();
	if (!lHandle)
		return false;
	
	std::shared_ptr<void> lRAIIDestroyHandle(nullptr, [lHandle](void*) { tjDestroy(lHandle); });

	int lWidth, lHeight, lSubsamp, lColorSpace;
	if (tjDecompressHeader3(lHandle,
		(unsigned char*)pImageData.data(), pImageData.size(),
		&lWidth, &lHeight, &lSubsamp, &lColorSpace) != 0)
	{
		return false;
	}

	pImageOut = QImage(lWidth, lHeight, QImage::Format_RGBA8888);
	if (pImageOut.isNull())
	{
		return false;
	}

	if (tjDecompress2(
		lHandle,
		(unsigned char*)pImageData.data(), pImageData.size(),
		pImageOut.bits(),
		lWidth,
		pImageOut.bytesPerLine(),
		lHeight,
		TJPF_RGBA,
		TJFLAG_FASTDCT) != 0)
	{
		return false;
	}

	return true;
}
#endif // TURBOJPEG_PLUGIN_ENABLED

/********************************************************************************/

ESImage::ESImage(ESStringId pImagePath, const QString pImageCachePath, const ESUsefullExif* pImageExif)
	: mLastUsed(0)
	, mImagePath(pImagePath)
	, mImageCachePath(pImageCachePath)
	, mIsLoaded(false)
	, mIsQueueForLoading(false)
	, mIsLoading(false)
	, mCancelLoading(false)
	, mCacheFileChecked(false)
	, mHasCacheFile(false)
	, mCurrentSearchSimilarity(0.f)
{
	if (pImageExif)
	{
		mExif = *pImageExif;
	}
}

/********************************************************************************/

bool ESImage::isLoading() const
{
	return !mIsLoaded && mIsQueueForLoading;
}

/********************************************************************************/

bool ESImage::isLoaded() const
{
	return mIsLoaded;
}

/********************************************************************************/

bool ESImage::isNull() const
{
	return mIsLoaded && !mImage;
}

/********************************************************************************/

void ESImage::cancelLoading()
{
	if(!mIsLoaded)
	{
		mCancelLoading = true;
		mIsQueueForLoading = false;
	}
}

/********************************************************************************/

void ESImage::unloadImage()
{
	if(mIsLoaded)
	{
		mImage.reset();
		mIsLoaded = false;
		mIsQueueForLoading = false;
	}
	else
	{
		cancelLoading();
	}
}

/********************************************************************************/

std::shared_ptr<const QImage> ESImage::getImage() const
{
	return mImage;
}

/********************************************************************************/

void ESImage::loadImage()
{
	ESImageCache::getInstance().queueImageLoading(std::const_pointer_cast<ESImage>(shared_from_this()), true);
}

/********************************************************************************/

void ESImage::updateLastUsed()
{
	mLastUsed = QDateTime::currentMSecsSinceEpoch();
}

/********************************************************************************/

ESStringId ESImage::getImagePath() const
{
	return mImagePath;
}

/********************************************************************************/

const ESUsefullExif& ESImage::getExif() const
{
	return mExif;
}

/********************************************************************************/

bool ESImage::hasCacheFile() const
{
	if(!mCacheFileChecked)
	{
#ifdef Q_OS_ANDROID
		if(QuaZip* lZip = getExifStatsArchive())
			mHasCacheFile = lZip->setCurrentFile(mImageCachePath);
#else
		mHasCacheFile = QFile::exists(mImageCachePath);
#endif // Q_OS_ANDROID
		mCacheFileChecked = true;
	}
	
	return mHasCacheFile;
}

/********************************************************************************/

QString ESImage::getImageCachePath() const
{
	return mImageCachePath;
}

/********************************************************************************/

QChar ESImage::getDriveLetter() const
{
	if(mDriveLetter.isNull())
		mDriveLetter = hasCacheFile() ? mImageCachePath[0] : mImagePath.getString()[0];
	return mDriveLetter;
}

/********************************************************************************/

void ESImage::loadImageInternal(const QSize pMaxSize, bool pAsync, std::atomic_int32_t* pNumAsyncTaskStarted)
{
	assert(!pMaxSize.isEmpty());

	std::shared_ptr<void> lRAIIDecNumAsyncTaskStarted(nullptr, [pNumAsyncTaskStarted](void*) { if (pNumAsyncTaskStarted) { --(*pNumAsyncTaskStarted); } });

	if (mIsLoaded)
		return;

	if (mCancelLoading)
	{
		emit imageLoadedOrCanceled(this);
		return;
	}

	bool lIsLoading = mIsLoading;
	if(lIsLoading)
		return;
	if(!mIsLoading.compare_exchange_strong(lIsLoading, true))
		return;

	std::shared_ptr<void> lRAIIResetIsLoading(nullptr, [this](void*) { mIsLoading = false; });
	
	const bool lReadCache = hasCacheFile();
	const QString& lImagePath = lReadCache ? mImageCachePath : mImagePath.getString();

	if (mCancelLoading)
	{
		emit imageLoadedOrCanceled(this);
		if (!lReadCache)
			ESImageCache::getInstance().imageLoadingFinished();
		return;
	}

	if(pAsync)
	{
#ifdef Q_OS_ANDROID
		QuaZip* lZip = getExifStatsArchive();
		if(!lZip)
			return;
		if (!lZip->setCurrentFile(lImagePath))
		{
			qWarning() << "File '" << lImagePath << "' not found in ExifStats archive file";
			return;
		}
		QuaZipFile lImageFile(lZip);
#else
		QFile lImageFile(lImagePath);
#endif //Q_OS_ANDROID
		if(!lImageFile.open(QIODevice::ReadOnly))
		{
			mIsLoaded = true;
			emit imageLoadedOrCanceled(this);
			if (!lReadCache)
				ESImageCache::getInstance().imageLoadingFinished();
			return;
		}
		mImageFileData = lImageFile.readAll();
		if (mCancelLoading)
		{
			mImageFileData.clear();
			mImageFileData.squeeze();
			if (!lReadCache)
				ESImageCache::getInstance().imageLoadingFinished();
			return;
		}

		(void)QtConcurrent::run([this, pMaxSize, lRAIIResetIsLoading, lReadCache, lRAIIDecNumAsyncTaskStarted]()
		{
			if (!mCancelLoading)
			{
				readImage(mImageFileData, pMaxSize);
				mImageFileData.clear();
				mImageFileData.squeeze();
				if(mCancelLoading)
				{
					mImage.reset();
				}
				else
				{
					mIsLoaded = true;
					ESImageCache::getInstance().unloadUnusedImages();
				}
			}
			if(!lReadCache)
				ESImageCache::getInstance().imageLoadingFinished();
			emit imageLoadedOrCanceled(this);
		});
	}
	else
	{
		readImage(lImagePath, pMaxSize);
		mIsLoaded = true;
		ESImageCache::getInstance().unloadUnusedImages();
		assert(!mCancelLoading); // In synchronous mode, 1calling cancelLoading() in another thread is not supported
		if (!lReadCache)
			ESImageCache::getInstance().imageLoadingFinished();
		emit imageLoadedOrCanceled(this);
	}
}

/********************************************************************************/

void ESImage::readImage(const QString& pImagePath, QSize pMaxSize)
{
#ifdef TURBOJPEG_PLUGIN_ENABLED
	if (hasCacheFile())
	{
		QFile lImageFile(pImagePath);
		QByteArray lImageData = lImageFile.readAll();
		mImage.reset(new QImage());
		loadTurboJpeg(*mImage.get(), lImageData);
		return;
	}
#endif // TURBOJPEG_PLUGIN_ENABLED

#ifdef Q_OS_ANDROID
	QuaZip* lZip = getExifStatsArchive();
	if(!lZip)
		return;
	if (!lZip->setCurrentFile(pImagePath))
	{
		qWarning() << "File '" << pImagePath << "' not found in ExifStats archive file";
		return;
	}
	QuaZipFile lImageFile(lZip);
	QByteArray lImageData = lImageFile.readAll();
	readImage(lImageData, pMaxSize);
#else
	QImageReader lImageReader(pImagePath);
	readImage(lImageReader, pMaxSize);
#endif
}

/********************************************************************************/

void ESImage::readImage(QByteArray& pImageData, QSize pMaxSize)
{
#ifdef TURBOJPEG_PLUGIN_ENABLED
	if (hasCacheFile())
	{
		mImage.reset(new QImage());
		loadTurboJpeg(*mImage.get(), pImageData);
		return;
	}
#endif // TURBOJPEG_PLUGIN_ENABLED
	QBuffer lImageDataBuffer(&pImageData);
	QImageReader lImageReader(&lImageDataBuffer);
	readImage(lImageReader, pMaxSize);
}

/********************************************************************************/

void ESImage::readImage(QImageReader& pImageReader, QSize pMaxSize)
{
	QImage lFullImage = pImageReader.read();
	if (mCancelLoading)
		return;

	mImage.reset(new QImage());
	if (mHasCacheFile)
	{
		*mImage = std::move(lFullImage);
	}
	else
	{
		*mImage = lFullImage.scaled(pMaxSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);

		// Manually rotate the image because the custom turbojpeg loader does not support auto rotate
		// TODO: Extend the turbojpeg loader to support auto rotate and add back the "pImageReader.setAutoTransform(!mHasCacheFile);"
		if (mExif.mOrientation != ESExifOrientation::Unspecified && mExif.mOrientation != ESExifOrientation::UpperLeft)
		{
			QTransform lTransform;
			switch (mExif.mOrientation)
			{
			case ESExifOrientation::UpperRight:
				lTransform.rotate(90);
				break;
			case ESExifOrientation::LowerRight:
				lTransform.rotate(180);
				break;
			case ESExifOrientation::LowerLeft:
				lTransform.rotate(270);
				break;
			default:
				break;
			}
			*mImage = mImage->transformed(lTransform, Qt::SmoothTransformation);
		}

		mImage->save(mImageCachePath, "JPG", CACHE_IMAGE_JPEG_COMPRESSION);
	}

	mDriveLetter = mImageCachePath[0];
	mHasCacheFile = true;
	mCacheFileChecked = true;
}
