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

	ES_QML_PROPERTY(CategoryAxisMaxSizeAuto, float, mDataHasChanged = true; update();)
	ES_QML_PROPERTY(CategoryAxisSizeAuto, bool, mDataHasChanged = true; update();)
	ES_QML_PROPERTY(CategoryAxisSize, float, mDataHasChanged = true; update();)
	ES_QML_PROPERTY(CategoryAxisOffset, float, mGeometryHasChanged = true; update();)
	ES_QML_PROPERTY(CategoryAxisScale, float, mGeometryHasChanged = true; update();)

	ES_QML_PROPERTY(ValueAxisMaxSizeAuto, float, mDataHasChanged = true; update();)
	ES_QML_PROPERTY(ValueAxisSizeAuto, bool, mDataHasChanged = true; update();)
	ES_QML_PROPERTY(ValueAxisSize, float, mDataHasChanged = true; update();)
	ES_QML_PROPERTY(ValueAxisScale, float, mGeometryHasChanged = true; update();)

	ES_QML_PROPERTY(Margin, float, mDataHasChanged = true; update();)
	ES_QML_PROPERTY(BarSpacing, double, mDataHasChanged = true; update();)
	
	ES_QML_PROPERTY(AllCategoriesOnly, bool)
	ES_QML_PROPERTY(CategorySpacing, float)

	ES_QML_PROPERTY(InvertAxis, bool, mDataHasChanged = true; update();)

	Q_INVOKABLE QPoint mapToValue(float pX);
	Q_INVOKABLE QPointF mapToPlotArea(float pX, float pY);
	Q_INVOKABLE float getChartFullSize() const;

	virtual void paint(QPainter* pPainter) override;

signals:
	/********************************** SIGNALS ***********************************/

private:
	/********************************** TYPES *************************************/

	
	/******************************** ATTRIBUTES **********************************/

	QSizeF mPreviousSize;
	int mMaxValue ;
	float mBarThickness;
	float mBarLengthFactor;
	qreal mRealCategoryAxisSize;
	double mActualBarSpacing;

	bool mValid;
	bool mDataHasChanged;
	bool mGeometryHasChanged;

	/********************************* METHODS ***********************************/

	void updateInternal();
};

