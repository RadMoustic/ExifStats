#include "ESImage.h"

// ES
#include "ESImageCache.h"

// Qt
#include <QFile>
#include <QImageReader>
#include <QtConcurrent>

// TurboJPEG
#ifdef TURBOJPEG_PLUGIN_ENABLED
#include <turbojpeg.h>
#endif

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

ESImage::ESImage(StringId pImagePath, const QString pImageCachePath, const UsefullExif* pImageExif)
	: mLastUsed(0)
	, mImagePath(pImagePath)
	, mImageCachePath(pImageCachePath)
	, mIsLoaded(false)
	, mIsQueueForLoading(false)
	, mIsLoading(false)
	, mCancelLoading(false)
	, mCacheFileChecked(false)
	, mHasCacheFile(false)
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
	return mIsLoaded && mImage.isNull();
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
		QImage().swap(mImage);
		mIsLoaded = false;
		mIsQueueForLoading = false;
	}
	else
	{
		cancelLoading();
	}
}

/********************************************************************************/

const QImage& ESImage::getImage() const
{
	return mImage;
}

/********************************************************************************/

void ESImage::loadImage()
{
	ESImageCache::getInstance().queueImageLoading(std::const_pointer_cast<ESImage>(shared_from_this()));
}

/********************************************************************************/

void ESImage::updateLastUsed()
{
	mLastUsed = QDateTime::currentMSecsSinceEpoch();
}

/********************************************************************************/

StringId ESImage::getImagePath() const
{
	return mImagePath;
}

/********************************************************************************/

const UsefullExif& ESImage::getExif() const
{
	return mExif;
}

/********************************************************************************/

bool ESImage::hasCacheFile() const
{
	if(!mCacheFileChecked)
	{
		mCacheFileChecked = true;
		mHasCacheFile = QFile::exists(mImageCachePath);
	}
	
	return mHasCacheFile;
}

/********************************************************************************/

QChar ESImage::getDriveLetter() const
{
	if(mDriveLetter.isNull())
		mDriveLetter = hasCacheFile() ? mImageCachePath[0] : mImagePath.getString()[0];
	return mDriveLetter;
}

/********************************************************************************/

void ESImage::loadImageInternal(const QSize aMaxSize, bool pAsync, std::atomic_int32_t* pNumAsyncTaskStarted)
{
	assert(!aMaxSize.isEmpty());

	std::shared_ptr<void> lRAIIDecNumAsyncTaskStarted(nullptr, [pNumAsyncTaskStarted](void*) { if (pNumAsyncTaskStarted) { --(*pNumAsyncTaskStarted); } });

	if (mIsLoaded)
		return;

	if (mCancelLoading)
	{
		emit imageLoadedOrCanceled(this);
		return;
	}

	bool isLoading = mIsLoading;
	if(isLoading)
		return;
	if(!mIsLoading.compare_exchange_strong(isLoading, true))
		return;

	std::shared_ptr<void> lRAIIResetIsLoading(nullptr, [this](void*) { mIsLoading = false; });
	
	const bool lReadCache = hasCacheFile();
	const QString& lImagePath = lReadCache ? mImageCachePath : mImagePath.getString();

	if (mCancelLoading)
	{
		emit imageLoadedOrCanceled(this);
		return;
	}

	if(pAsync)
	{
		QFile lImageFile(lImagePath);
		if(!lImageFile.open(QIODevice::ReadOnly))
		{
			mIsLoaded = true;
			emit imageLoadedOrCanceled(this);
			return;
		}
		mImageFileData = lImageFile.readAll();
		if (mCancelLoading)
		{
			mImageFileData.clear();
			mImageFileData.squeeze();
			return;
		}

		QtConcurrent::run([this, aMaxSize, lRAIIResetIsLoading, lReadCache, lRAIIDecNumAsyncTaskStarted]()
		{
			if (!mCancelLoading)
			{
				readImage(mImageFileData, aMaxSize);
				mImageFileData.clear();
				mImageFileData.squeeze();
				if(mCancelLoading)
				{
					mImage = QImage();
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
		readImage(lImagePath, aMaxSize);
		mIsLoaded = true;
		ESImageCache::getInstance().unloadUnusedImages();
		assert(!mCancelLoading); // In synchronous mode, 1calling cancelLoading() in another thread is not supported
		if (!lReadCache)
			ESImageCache::getInstance().imageLoadingFinished();
		emit imageLoadedOrCanceled(this);
	}
}

/********************************************************************************/

void ESImage::readImage(const QString& pImagePath, QSize aMaxSize)
{
#ifdef TURBOJPEG_PLUGIN_ENABLED
	if (hasCacheFile())
	{
		QFile lImageFile(pImagePath);
		QByteArray lImageData = lImageFile.readAll();
		loadTurboJpeg(mImage, lImageData);
		return;
	}
#endif // TURBOJPEG_PLUGIN_ENABLED
	QImageReader lImageReader(pImagePath);
	readImage(lImageReader, aMaxSize);
}

/********************************************************************************/

void ESImage::readImage(QByteArray& pImageData, QSize aMaxSize)
{
#ifdef TURBOJPEG_PLUGIN_ENABLED
	if (hasCacheFile())
	{
		loadTurboJpeg(mImage, pImageData);
		return;
	}
#endif // TURBOJPEG_PLUGIN_ENABLED
	QBuffer lImageDataBuffer(&pImageData);
	QImageReader lImageReader(&lImageDataBuffer);
	readImage(lImageReader, aMaxSize);
}

/********************************************************************************/

void ESImage::readImage(QImageReader& pImageReader, QSize aMaxSize)
{
	QImage lFullImage = pImageReader.read();
	if (mCancelLoading)
		return;
	if (mHasCacheFile)
	{
		mImage = std::move(lFullImage);
	}
	else
	{
		mImage = lFullImage.scaled(aMaxSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);

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
			mImage = mImage.transformed(lTransform, Qt::SmoothTransformation);
		}

		mImage.save(mImageCachePath, "JPG", 90);
	}

	mDriveLetter = mImageCachePath[0];
	mHasCacheFile = true;
	mCacheFileChecked = true;
}