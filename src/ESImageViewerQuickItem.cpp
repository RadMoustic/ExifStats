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
	, mIsUserInteracting(false)
	, mHighResImageDisplayed(false)
	, mHighResImageStep(StepNone)
	, mHighResImageDownloadProgress(0.f)
{
}

/********************************************************************************/

void ESImageViewerQuickItem::downloadOriginalImage(QString pImagePath, bool pHighPriority)
{
	if(!pImagePath.isEmpty())
	{
		std::shared_ptr<ESImage> lImage = ESImageCache::getInstance().getImage(pImagePath);
		if(!lImage)
			return;
		if (mCurrentOriginalImageDownloadRequest && mCurrentOriginalImageDownloadRequest == lImage->mOriginalImageDownloadRequest)
			return;

		if (pHighPriority)
		{
			if(mCurrentOriginalImageDownloadRequest)
			{
				mCurrentOriginalImageDownloadRequest->cancelRequest();
				mCurrentOriginalImageDownloadRequest = nullptr;
			}

			mOriginalDownloadRequests.push_front(pImagePath);
		}
		else
		{
			mOriginalDownloadRequests.push_back(pImagePath);
		}
		startNextOriginalImageDownloadRequest();
	}
}

/********************************************************************************/

void ESImageViewerQuickItem::cancelAllDownloadRequests()
{
	mOriginalDownloadRequests.clear();
	if(mCurrentOriginalImageDownloadRequest)
	{
		mCurrentOriginalImageDownloadRequest->cancelRequest();
		mCurrentOriginalImageDownloadRequest = nullptr;
	}
}

/********************************************************************************/

void ESImageViewerQuickItem::startNextOriginalImageDownloadRequest()
{
	if(mCurrentOriginalImageDownloadRequest || mOriginalDownloadRequests.empty())
		return;

	QString lImagePath = mOriginalDownloadRequests.front();
	mOriginalDownloadRequests.pop_front();

	std::shared_ptr<ESImage> lImage = ESImageCache::getInstance().getImage(lImagePath);
	if(lImage && (!lImage->mOriginalImageDownloadRequest || lImage->mOriginalImageDownloadRequest->hasFailed()) && !lImage->getImageHash().isEmpty())
	{
		if (lImage == mImage)
		{
			setHighResImageStep(StepStarted);
			setHighResImageDownloadProgress(0.f);
		}
		lImage->mOriginalImageDownloadRequest = ESNetClientOriginalImageDownloadRequest::downloadOriginalImage(lImage);
		mCurrentOriginalImageDownloadRequest = lImage->mOriginalImageDownloadRequest;
		connect(lImage->mOriginalImageDownloadRequest.get(), &ESNetClientOriginalImageDownloadRequest::lastMessageChanged, this,
			[this, lImage](ESNetClientHandler::Message pLastMessage)
			{
				if(lImage == mImage)
				{
					setHighResImageStep(pLastMessage);
				}
			}, Qt::DirectConnection);
		connect(lImage->mOriginalImageDownloadRequest.get(), &ESNetClientOriginalImageDownloadRequest::downloadProgress, this,
			[this, lImage](float pProgress)
			{
				if(lImage == mImage)
				{
					setHighResImageDownloadProgress(pProgress);
				}
			}, Qt::DirectConnection);
		connect(lImage->mOriginalImageDownloadRequest.get(), &ESNetClientOriginalImageDownloadRequest::finished, this,
			[this](const ESNetClientOriginalImageDownloadRequest& /*pRequest*/)
			{
				onHighResImageDownloaded();
				mCurrentOriginalImageDownloadRequest = nullptr;
				startNextOriginalImageDownloadRequest();
			}, Qt::QueuedConnection);
	}
	else
	{
		if (lImage && lImage->mOriginalImageDownloadRequest)
		{
			if (lImage->mOriginalImageDownloadRequest->isFinished() && !lImage->mOriginalImageDownloadRequest->getDownloadedImage().isNull())
				setHighResImageStep(StepFinished);
			else
				setHighResImageStep(StepStarted);
		}
		else if(lImage->getImageHash().isEmpty())
			setHighResImageStep(StepNoHash);
		startNextOriginalImageDownloadRequest();
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

		bool lHasHighResImage = lRequest && !lRequest->hasFailed() && lRequest->isFinished() && !lRequest->getDownloadedImage().isNull() && !mIsUserInteracting;
		mHighResImageDisplayed = lHasHighResImage;
		const QImage* lImage = lHasHighResImage ? &lRequest->getDownloadedImage() : lESImage->getImage().get();
		
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
		if(mImage->mOriginalImageDownloadRequest && !mImage->mOriginalImageDownloadRequest->getDownloadedImage().isNull())
			setHighResImageStep(StepFinished);
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

		if(mImage)
		{
			mHighResImageDisplayed = false;
			if (mImage->mOriginalImageDownloadRequest && !mImage->mOriginalImageDownloadRequest->getDownloadedImage().isNull())
				setHighResImageStep(StepFinished);
			else if(mImage->getImageHash().isEmpty())
				setHighResImageStep(StepNoHash);
			else
			{	
				setHighResImageStep(StepStarted);
				downloadOriginalImage(mImagePath, true);
			}
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
