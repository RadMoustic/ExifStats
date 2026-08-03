#include "ESImageViewerQuickItem.h"

// ES
#include "ESImage.h"
#include "ESImageCache.h"
#include "ESNetClient.h"

// Qt
#include <QPainter>
#include <QtConcurrent>

/********************************************************************************/

ESImageViewerQuickItem::ESImageViewerQuickItem()
	: mValid(false)
	, mDataHasChanged(false)
	, mGeometryHasChanged(false)
	, mImageRatio(1.f)
{
	setTextureSize(textureSize()*3);
}

/********************************************************************************/

/*virtual*/ void ESImageViewerQuickItem::paint(QPainter* pPainter) /*override*/
{
	mGeometryHasChanged = mPreviousSize != size();
	mPreviousSize = size();

	updateInternal();

	if (mValid && mImage && mImage->isLoaded())
	{
		const QImage* lImage = mOriginalImage.isNull() ? mImage->getImage().get() : &mOriginalImage;
		float lW = width();
		float lH = height();
		pPainter->fillRect(pPainter->viewport(), Qt::black);
		float lImageRatio = float(lImage->width()) / float(lImage->height());
		float lViewportRatio = lW / lH;
		float lX, lY, lWidth, lHeight;
		if (lImageRatio >= lViewportRatio)
		{
			lWidth = lW;
			lHeight = lW / lImageRatio;
			lX = 0.f;
			lY = (lH - lHeight) / 2.f;
		}
		else
		{
			lWidth = lH * lImageRatio;
			lHeight = lH;
			lX = (lW - lWidth) / 2.f;
			lY = 0.f;
		}
		pPainter->setRenderHint(QPainter::SmoothPixmapTransform);
		pPainter->drawImage(QRectF(lX, lY, lWidth, lHeight), *lImage);
	}
}

/********************************************************************************/

void ESImageViewerQuickItem::updateInternal()
{
	mValid = true;

	if(!mValid)
		return;

	if (mDataHasChanged)
	{
		if(mImage)
			disconnect(mImageLoadedConnection);
		mImage = ESImageCache::getInstance().getImage(mImagePath);
		mOriginalImage = QImage();

		mOriginalImageDownloadRequest = ESNetClient::downloadOriginalImage(mImagePath, "192.168.1.15", 12345,
		[this](const QImage& pImage)
		{
			if (!pImage.isNull())
			{
				mOriginalImage = pImage;
				QMetaObject::invokeMethod(this, "update", Qt::QueuedConnection);
			}
		});

		const ESUsefullExif& lExif = mImage->getExif();
		setImageWidth(lExif.getOrientedWidth());
		setImageHeight(lExif.getOrientedHeight());
		setImageRatio(mImage->getRatio());
		setCameraModel(lExif.mCameraModel.getString());
		setLensModel(lExif.mLensModel.getString());
		setDateTime(QDateTime::fromSecsSinceEpoch(lExif.mDateTime).toString("yyyy/MM/dd hh:mm:ss"));
		setShutterSpeedValue(lExif.mShutterSpeedValue);
		setFNumber(lExif.mFNumber);
		if(lExif.mGeoLocationGuessed)
			setGeoLocation(QGeoCoordinate(0,0));
		else
			setGeoLocation(QGeoCoordinate(lExif.mGeoLocation.mLatitude, lExif.mGeoLocation.mLongitude));
		setFocalLengthIn35mm(lExif.mFocalLengthIn35mm);
		setFocalLength(lExif.mFocalLength);
		setOrientation(lExif.mOrientation);
		setISOSpeedRatings(lExif.mISOSpeedRatings);

		mImage->updateLastUsed();
		if (!mImage->isLoaded() && !mImage->isLoading())
			mImage->loadImage();
		update();
		mImageLoadedConnection = connect(mImage.get(), &ESImage::imageLoadedOrCanceled, this, [this]()
		{
			update();
		});
	}

	mDataHasChanged = false;
	mGeometryHasChanged = false;
}
