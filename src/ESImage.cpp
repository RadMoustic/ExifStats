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
bool loadTurboJpeg(QImage& img, const QByteArray& data)
{
	tjhandle handle = tjInitDecompress();
	if (!handle)
		return false;

	int width, height, subsamp, cs;
	if (tjDecompressHeader3(handle,
		(unsigned char*)data.data(), data.size(),
		&width, &height, &subsamp, &cs) != 0)
	{
		return false;
	}

	img = QImage(width, height, QImage::Format_RGBA8888);
	if (img.isNull()) {
		return false;
	}

	int pitch = img.bytesPerLine();

	if (tjDecompress2(
		handle,
		(unsigned char*)data.data(), data.size(),
		img.bits(),
		width,
		pitch,
		height,
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
	, mExifDateTime(pImageExif ? pImageExif->mDateTime : 0)
	, mCacheFileChecked(false)
	, mHasCacheFile(false)
{
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
		mImage = QImage();
		mIsLoaded = false;
		mIsQueueForLoading = false;
	}
	else
	{
		cancelLoading();
	}
}

/********************************************************************************/

const QImage& ESImage::getImage(bool pUpdateLastUsed) const
{
	if (isLoaded())
		mLastUsed = pUpdateLastUsed ? QDateTime::currentMSecsSinceEpoch() : mLastUsed;
	return mImage;
}

/********************************************************************************/

void ESImage::loadImage()
{
	mLastUsed = QDateTime::currentMSecsSinceEpoch();
	ESImageCache::getInstance().queueImageLoading(std::const_pointer_cast<ESImage>(shared_from_this()));
}

/********************************************************************************/

StringId ESImage::getImagePath() const
{
	return mImagePath;
}

/********************************************************************************/

uint64_t ESImage::getExifDateTime() const
{
	return mExifDateTime;
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

void ESImage::loadImageInternal(const QSize aMaxSize, bool pAsync)
{
	assert(!aMaxSize.isEmpty());
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

	// RAII reset mIsLoading
	std::shared_ptr<void> lResetIsLoading(nullptr, [this](void*) { mIsLoading = false; });
	
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
			return;

		QtConcurrent::run([this, aMaxSize, lResetIsLoading]()
		{
			if (!mCancelLoading)
			{
				readImage(mImageFileData, aMaxSize);
				mImageFileData.clear();
				if(mCancelLoading)
				{
					mImage = QImage();
				}
				else
				{
					mIsLoaded = true;
					mLastUsed = QDateTime::currentMSecsSinceEpoch();
					ESImageCache::getInstance().unloadUnusedImages();
				}
			}
			emit imageLoadedOrCanceled(this);
		});
	}
	else
	{
		readImage(lImagePath, aMaxSize);
		mIsLoaded = true;
		mLastUsed = QDateTime::currentMSecsSinceEpoch();
		ESImageCache::getInstance().unloadUnusedImages();
		assert(!mCancelLoading); // In synchronous mode, calling cancelLoading() in another thread is not supported
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
	pImageReader.setAutoTransform(!mHasCacheFile);
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
		mImage.save(mImageCachePath, "JPG", 90);
	}

	mDriveLetter = mImageCachePath[0];
	mHasCacheFile = true;
	mCacheFileChecked = true;
}