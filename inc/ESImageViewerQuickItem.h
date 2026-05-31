#pragma once

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

// ExifStats
#include "ESUtils.h"

// Qt
#include <QQuickPaintedItem>
#include <QImage>
#include <QGeoCoordinate>

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

class ESImage;

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

class ESImageViewerQuickItem : public QQuickPaintedItem
{
	Q_OBJECT
	QML_ELEMENT
public:
	/******************************** ATTRIBUTES **********************************/

	
	/********************************* METHODS ***********************************/

	ESImageViewerQuickItem();
	
	ES_QML_PROPERTY(ImagePath, QString, mDataHasChanged = true; update();)
	ES_QML_READ_PROPERTY(ImageRatio, float)
	ES_QML_READ_PROPERTY(ImageWidth, float)
	ES_QML_READ_PROPERTY(ImageHeight, float)

	ES_QML_READ_PROPERTY(CameraModel, QString)
	ES_QML_READ_PROPERTY(LensModel, QString)
	ES_QML_READ_PROPERTY(DateTime, QString)
	ES_QML_READ_PROPERTY(ShutterSpeedValue, float)
	ES_QML_READ_PROPERTY(FNumber, float)
	ES_QML_READ_PROPERTY(GeoLocation, QGeoCoordinate)
	ES_QML_READ_PROPERTY(FocalLengthIn35mm, int)
	ES_QML_READ_PROPERTY(FocalLength, int)
	ES_QML_READ_PROPERTY(Orientation, int)
	ES_QML_READ_PROPERTY(ISOSpeedRatings, int)

	virtual void paint(QPainter* pPainter) override;

signals:
	/********************************** SIGNALS ***********************************/

private:
	/********************************** TYPES *************************************/

	
	/******************************** ATTRIBUTES **********************************/

	std::shared_ptr<ESImage> mImage;
	QMetaObject::Connection mImageLoadedConnection;

	QSizeF mPreviousSize;
	bool mValid;
	bool mDataHasChanged;
	bool mGeometryHasChanged;

	/********************************* METHODS ***********************************/

	void updateInternal();
};

