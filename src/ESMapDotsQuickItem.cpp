#include "ESMapDotsQuickItem.h"

// Qt
#include <QPainter>
#include <QGeoCoordinate>
#include <QGeoShape>
#include <QGeoRectangle>

/********************************************************************************/

ESMapDotsQuickItem::ESMapDotsQuickItem()
: mMapObj(nullptr)
, mDotQuantityLUT(":/Images/DotQuantityLUT.png")
, mMaxDotSizeMultiplier(3.)
{
	assert(!mDotQuantityLUT.isNull());
}

/********************************************************************************/

/*virtual*/ void ESMapDotsQuickItem::paint(QPainter* pPainter) /*override*/
{
	if (	mMapObj
		&&	mMapFromCoordinateFct.isValid()
		&&	mMapVisibleRegionProp.isValid()
		&&	mMapZoomLevelProp.isValid()
		&&	mQuadTree)
	{
		pPainter->setPen(Qt::NoPen);

		float lDotQuantityLUTWidth = float(mDotQuantityLUT.width() - 1);
		QGeoShape lVisibleRegion = qvariant_cast<QGeoShape>(mMapVisibleRegionProp.read(mMapObj));
		qreal lZoomLevel = qvariant_cast<qreal>(mMapZoomLevelProp.read(mMapObj));
		if(lVisibleRegion.isValid())
		{
			QGeoRectangle lGeoRect = lVisibleRegion.boundingGeoRectangle();
			QRectF lVisibleRectF(lGeoRect.center().latitude() - lGeoRect.height() / 2.f, lGeoRect.center().longitude() - lGeoRect.width() / 2.f, lGeoRect.height(), lGeoRect.width());
			QVector<QVector3D> lVisiblePoints = mQuadTree->getPoints(static_cast<int>(lZoomLevel) + 6, lVisibleRectF);
			std::sort(lVisiblePoints.begin(), lVisiblePoints.end(), [&](const QVector3D& pP1, const QVector3D& pP2)
				{
					return pP1.z() < pP2.z();
				});
			constexpr float cMaxDotSize = 300.f; // The absolute dot size is clamped to this value (so any dot >= this will have the max color)
			for(const QVector3D& lPoint: lVisiblePoints)
			{
				QGeoCoordinate lGeoCoords(lPoint.x(), lPoint.y());
				QPointF lRet;
				mMapFromCoordinateFct.invoke(mMapObj, Qt::DirectConnection, qReturnArg(lRet), lGeoCoords, false);
				float lDotSize = sqrt(std::min(lPoint.z() / cMaxDotSize, 1.0f));
				QColor lColor = mDotQuantityLUT.pixelColor(QPoint(lDotSize * lDotQuantityLUTWidth, 0));
				const float lBaseSize = 7;
				const float lDotPixelSize = lBaseSize * std::max(1.0f, lDotSize * mMaxDotSizeMultiplier);
				lColor.setAlphaF(0.5f + (1.0f - lDotSize) * 0.5f);
				pPainter->setBrush(lColor);
				pPainter->setPen(lColor);
				pPainter->drawEllipse(lRet.x() - lDotPixelSize/2.f, lRet.y() - lDotPixelSize / 2.f, lDotPixelSize, lDotPixelSize);
			}
		}
	}
}

/********************************************************************************/

void ESMapDotsQuickItem::setMap(QVariant pMap)
{
	mMapObj = qvariant_cast<QObject*>(pMap);
	const QMetaObject* lMeta = mMapObj->metaObject();
	if (int lFctIdx = lMeta->indexOfMethod(QMetaObject::normalizedSignature("fromCoordinate(const QGeoCoordinate&, bool)")))
	{
		mMapFromCoordinateFct = lMeta->method(lFctIdx);
	}
	if (int lPropIdx = lMeta->indexOfProperty("visibleRegion"))
	{
		mMapVisibleRegionProp = lMeta->property(lPropIdx);
	}
	if (int lPropIdx = lMeta->indexOfProperty("zoomLevel"))
	{
		mMapZoomLevelProp = lMeta->property(lPropIdx);
	}
}

/********************************************************************************/

void ESMapDotsQuickItem::setDots(const QVector<QPointF>& pDots)
{
	mQuadTree.reset(new ESQuadTree(QRectF(-90.f, -180.f, 180.f, 360.f), pDots));
	refresh();
}

/********************************************************************************/

void ESMapDotsQuickItem::refresh()
{
	update();
}