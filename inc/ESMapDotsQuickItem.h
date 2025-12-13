#pragma once

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

// ES
#include "ESQuadTree.h"

// Qt
#include <QQuickPaintedItem>
#include <QImage>

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

class ESMapDotsQuickItem : public QQuickPaintedItem
{
	Q_OBJECT
	QML_ELEMENT
public:
	/******************************** ATTRIBUTES **********************************/

	
	/********************************* METHODS ***********************************/

	ESMapDotsQuickItem();

	Q_INVOKABLE void setDots(const QVector<QPointF>& pDots);
	Q_INVOKABLE void setMap(QVariant pMap);
	Q_INVOKABLE void refresh();

	virtual void paint(QPainter* pPainter) override;

private:
	
	/******************************** ATTRIBUTES **********************************/

	QRectF mGeoRect;
	std::unique_ptr<ESQuadTree> mQuadTree;
	QMetaMethod mMapFromCoordinateFct;
	QMetaProperty mMapVisibleRegionProp;
	QMetaProperty mMapZoomLevelProp;
	QObject* mMapObj;
	QImage mDotQuantityLUT;
	float mMaxDotSizeMultiplier;
};

