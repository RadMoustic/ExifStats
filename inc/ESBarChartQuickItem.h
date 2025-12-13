#pragma once

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

// ExifStats
#include "ESUtils.h"

// Qt
#include <QQuickPaintedItem>
#include <QImage>

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

class ESBarChartQuickItem : public QQuickPaintedItem
{
	Q_OBJECT
	QML_ELEMENT
public:
	/******************************** ATTRIBUTES **********************************/

	
	/********************************* METHODS ***********************************/

	ESBarChartQuickItem();

	ES_QML_PROPERTY(Categories, QVector<QString>, mDataHasChanged = true; update(); )
	ES_QML_PROPERTY(Values, QVector<int>, mDataHasChanged = true; update();)
	
	ES_QML_PROPERTY(Margin, float, mDataHasChanged = true; update();)
	ES_QML_PROPERTY(XAxisHeightAuto, bool, mDataHasChanged = true; update();)
	ES_QML_PROPERTY(XAxisHeight, float, mDataHasChanged = true; update();)
	ES_QML_PROPERTY(YAxisWidthAuto, bool, mDataHasChanged = true; update();)
	ES_QML_PROPERTY(YAxisWidth, float, mDataHasChanged = true; update();)
	ES_QML_PROPERTY(BarSpacing, double, mDataHasChanged = true; update();)
	
	ES_QML_PROPERTY(XOffset, float, mGeometryHasChanged = true; update();)
	ES_QML_PROPERTY(XScale, float, mGeometryHasChanged = true; update();)
	ES_QML_PROPERTY(YScale, float, mGeometryHasChanged = true; update();)
	
	ES_QML_PROPERTY(AllCategoriesOnly, bool)
	ES_QML_PROPERTY(CategorySpacing, float)

	Q_INVOKABLE QPoint mapToValue(float pX);
	Q_INVOKABLE QPointF mapToPlotArea(float pX, float pY);
	Q_INVOKABLE float getChartFullWidth() const;

	virtual void paint(QPainter* pPainter) override;

signals:
	/********************************** SIGNALS ***********************************/

private:
	/********************************** TYPES *************************************/

	
	/******************************** ATTRIBUTES **********************************/

	QSizeF mPreviousSize;
	int mMaxValue ;
	float mBarWidth;
	float mBarHeightFactor;
	float mAvailableHeight;
	qreal mRealXAxisHeight;
	double mActualBarSpacing;

	bool mValid;
	bool mDataHasChanged;
	bool mGeometryHasChanged;

	/********************************* METHODS ***********************************/

	void setupPainterText(QPainter& pPainter);
	void updateInternal();
};

