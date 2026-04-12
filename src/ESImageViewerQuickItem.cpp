#include "ESImageViewerQuickItem.h"

// ES
#include "ESImage.h"
#include "ESImageCache.h"

// Qt
#include <QPainter>

/********************************************************************************/

ESImageViewerQuickItem::ESImageViewerQuickItem()
	: mValid(false)
	, mDataHasChanged(false)
	, mGeometryHasChanged(false)
{

}

/********************************************************************************/

/*virtual*/ void ESImageViewerQuickItem::paint(QPainter* pPainter) /*override*/
{
	mGeometryHasChanged = mPreviousSize != size();
	mPreviousSize = size();

	updateInternal();

	if (mValid && mImage)
	{
		float lW = width();
		float lH = height();
		const QImage& lImage = mImage->getImage();
		pPainter->fillRect(pPainter->viewport(), Qt::black);
		float lImageRatio = float(lImage.width()) / float(lImage.height());
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
		pPainter->drawImage(QRectF(lX, lY, lWidth, lHeight), lImage);
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
		mImage->updateLastUsed();
		if (!mImage->isLoaded() && !mImage->isLoading())
			mImage->loadImage();
		mImageLoadedConnection = connect(mImage.get(), &ESImage::imageLoadedOrCanceled, this, [this]() { update(); });
	}

	mDataHasChanged = false;
	mGeometryHasChanged = false;
}
