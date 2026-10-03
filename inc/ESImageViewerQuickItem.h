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

// Stl
#include <mutex>
#include <deque>

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

class ESImage;
class ESNetClientOriginalImageDownloadRequest;

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
	ES_QML_PROPERTY(IsUserInteracting, bool, onUserInteractingChanged();)

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

	ES_QML_READ_PROPERTY(HighResImageStep, int)
	ES_QML_READ_PROPERTY(HighResImageDownloadProgress, float)

	Q_INVOKABLE void downloadOriginalImage(QString pImagePath, bool pHighPriority);
	Q_INVOKABLE void cancelAllDownloadRequests();

	virtual void paint(QPainter* pPainter) override;

signals:
	/********************************** SIGNALS ***********************************/

private:
	/********************************** TYPES *************************************/

	enum HighResImageStep: int
	{
		StepFinished = -3,
		StepNone = -2,
		StepNoHash = -1,
		StepStarted = 0,
	};
	
	/******************************** ATTRIBUTES **********************************/

	std::mutex mImageMutex;
	std::shared_ptr<ESImage> mImage;
	QMetaObject::Connection mImageLoadedConnection;
	std::deque<QString> mOriginalDownloadRequests;
	std::shared_ptr<ESNetClientOriginalImageDownloadRequest> mCurrentOriginalImageDownloadRequest;

	QSizeF mPreviousSize;
	bool mValid;
	bool mDataHasChanged;
	bool mGeometryHasChanged;
	bool mHighResImageDisplayed;

	/********************************* METHODS ***********************************/

	void updateInternal();
	void onUserInteractingChanged();
	void onHighResImageDownloaded();
	void startNextOriginalImageDownloadRequest();
};

