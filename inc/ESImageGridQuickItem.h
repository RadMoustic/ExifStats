#pragma once

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

// ExifStats
#include "ESUtils.h"
#include "ESImageCache.h"
#include "ESExifStatListFiles.h"

// Qt
#include <QQuickPaintedItem>
#include <QImage>

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

class ESImageGridQuickItem : public QQuickPaintedItem
{
	Q_OBJECT
	QML_ELEMENT
public:
	/******************************** ATTRIBUTES **********************************/

	
	/********************************* METHODS ***********************************/

	ESImageGridQuickItem();
	virtual ~ESImageGridQuickItem();

	ES_QML_PROPERTY(FilteredFilesList, const ESExifStatListFiles*)
	ES_QML_PROPERTY(ImageFiles, QVector<QString>, mDataHasChanged = true; update();)
	ES_QML_PROPERTY(ImageWidth, int, mDataHasChanged = true; update();)
	ES_QML_PROPERTY(ImageHeight, int, mDataHasChanged = true; update();)
	ES_QML_PROPERTY(YOffset, float, mGeometryHasChanged = true; update();)
	ES_QML_PROPERTY(Loading, bool)
	ES_QML_PROPERTY(LoadingProgress, float)

	ES_QML_READ_PROPERTY(ContentHeight, int)

	Q_INVOKABLE QString getImageFileAtPos(float pX, float pY) const;

	virtual void paint(QPainter* pPainter) override;

signals:
	/********************************** SIGNALS ***********************************/

private:
	/********************************** TYPES *************************************/

	
	/******************************** ATTRIBUTES **********************************/

	QSizeF mPreviousSize;
	int mNbColumns;
	int mNbRows;

	bool mValid;
	bool mDataHasChanged;
	bool mGeometryHasChanged;

	std::vector<std::shared_ptr<ESImage>> mImages;
	const ESExifStatListFilesComponent* mFilteredFilesListComponent;

	/********************************* METHODS ***********************************/

	void updateInternal();
	void onImageCachingProgress(int pLoadedCount, int pLoadingCount);
	void sort();
};
