#include "ESImageViewerQuickItem.h"

// ES
#include "ESImage.h"
#include "ESImageCache.h"
#include "ESNetClient.h"
#include "ESDatabase.h"

// Qt
#include <QPainter>
#include <QtConcurrent>

/********************************************************************************/

ESImageViewerQuickItem::ESImageViewerQuickItem()
	: mValid(false)
	, mDataHasChanged(false)
	, mGeometryHasChanged(false)
	, mImageRatio(1.f)
	, mIsUserInteracting(false)
	, mHighResImageDisplayed(false)
{
}

/********************************************************************************/

void ESImageViewerQuickItem::downloadOriginalImage(QString pImagePath)
{
	std::shared_ptr<ESImage> lImage = ESImageCache::getInstance().getImage(pImagePath);
	if(lImage && (!lImage->mOriginalImageDownloadRequest || lImage->mOriginalImageDownloadRequest->isCancelled()) && !lImage->getImageHash().isEmpty())
	{
		lImage->mOriginalImageDownloadRequest = ESNetClientOriginalImageDownloadRequest::downloadOriginalImage(lImage, "192.168.1.15", 12345);
		connect(lImage->mOriginalImageDownloadRequest.get(), &ESNetClientOriginalImageDownloadRequest::finished, this,
			[this](const ESNetClientOriginalImageDownloadRequest& /*pRequest*/)
			{
				QMetaObject::invokeMethod(this, [this]() {onHighResImageDownloaded(); }, Qt::QueuedConnection);
			}, Qt::DirectConnection);
	}
}

/********************************************************************************/

/*virtual*/ void ESImageViewerQuickItem::paint(QPainter* pPainter) /*override*/
{
	mGeometryHasChanged = mPreviousSize != size();
	mPreviousSize = size();

	updateInternal();

	if (mValid && mImage && mImage->isLoaded())
	{
		// Keep refs to avoid race conditions with the image being unloaded while painting
		std::shared_ptr<ESImage> lESImage = mImage;
		std::shared_ptr<ESNetClientOriginalImageDownloadRequest> lRequest = lESImage->mOriginalImageDownloadRequest;

		bool hasHighResImage = lRequest && !lRequest->isCancelled() && lRequest->isFinished() && !lRequest->getDownloadedImage().isNull() && !mIsUserInteracting;
		mHighResImageDisplayed = hasHighResImage;
		const QImage* lImage = hasHighResImage ? &lRequest->getDownloadedImage() : lESImage->getImage().get();
		
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

void ESImageViewerQuickItem::onUserInteractingChanged()
{
	if(!mIsUserInteracting)
	{
		if (textureSize() != QSize(4096, 4096))
		{
			setTextureSize(QSize(4096, 4096));
			update();
		}
		else if (!mHighResImageDisplayed)
		{
			update();
		}
	}
}

/********************************************************************************/

void ESImageViewerQuickItem::onHighResImageDownloaded()
{
	if (!mIsUserInteracting)
	{
		setTextureSize(QSize(4096, 4096));
		update();
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
		std::lock_guard<std::mutex> lLock(mImageMutex);

		if(mImage)
			disconnect(mImageLoadedConnection);
		mImage = ESImageCache::getInstance().getImage(mImagePath);
		assert(mImage);

		mHighResImageDisplayed = true;

		const ESFileInfo* lImageFileInfo = ESDatabase::getInstance().getFileInfo(mImage->getImagePath());
		assert(lImageFileInfo);

		if(mImage && !mImage->getImageHash().isEmpty() && (!mImage->mOriginalImageDownloadRequest || mImage->mOriginalImageDownloadRequest->isCancelled()))
		{
			mHighResImageDisplayed = false;
			mImage->mOriginalImageDownloadRequest = ESNetClientOriginalImageDownloadRequest::downloadOriginalImage(mImage, "192.168.1.15", 12345);
			connect(mImage->mOriginalImageDownloadRequest.get(), &ESNetClientOriginalImageDownloadRequest::finished, this,
			[this](const ESNetClientOriginalImageDownloadRequest& pRequest)
			{
				std::lock_guard<std::mutex> lLock(mImageMutex);

				if (pRequest.getImage() == mImage && !pRequest.getDownloadedImage().isNull())
				{
					QMetaObject::invokeMethod(this,[this](){onHighResImageDownloaded();}, Qt::QueuedConnection);
				}
			}, Qt::DirectConnection);
		}

		if(mImage->mOriginalImageDownloadRequest)
			mImage->mOriginalImageDownloadRequest->mLastUsedImage = QDateTime::currentMSecsSinceEpoch();

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
