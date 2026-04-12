#pragma once

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

// ExifStats
#include "ESUtils.h"
#include "ESImageCache.h"
#include "ESListFilesStat.h"

// Qt
#include <QQuickFramebufferObject>
#include <QOpenGLExtraFunctions>
#include <QImage>

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

class QOpenGLShaderProgram;
class QOpenGLVertexArrayObject;
class QOpenGLBuffer;
class QOpenGLTexture;

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

class ESImageGridQuickItemRenderer : public QQuickFramebufferObject::Renderer, protected QOpenGLExtraFunctions
{
public:
	/********************************* METHODS ***********************************/

	ESImageGridQuickItemRenderer();

	QOpenGLFramebufferObject* createFramebufferObject(const QSize& pSize) override;
	virtual void synchronize(QQuickFramebufferObject* pIem) override;
	virtual void render() override;
private:
	/********************************** TYPES *************************************/

	struct ImageInstanceData
	{
		float mX;
		float mY;
		float mWidthAndHeight;
		float mTextureIndex;
	};

	struct ImageTextureSlot
	{
		int mTextureSlot;
		int mRow;
	};

	/******************************** ATTRIBUTES **********************************/

	std::unique_ptr<QOpenGLShaderProgram> mShaderProgram;
	std::unique_ptr<QOpenGLVertexArrayObject> mVAO;
	std::unique_ptr<QOpenGLBuffer> mVBOGeometry;
	std::unique_ptr<QOpenGLBuffer> mVBOInstances;
	std::unique_ptr<QOpenGLTexture> mImageTextures;
	std::vector<int> mFreeImageTextureSlots;
	std::unordered_map<ESImage*, ImageTextureSlot> mImageToTextureSlot;
	std::vector<ImageInstanceData> mInstanceData;
	QSizeF mSize;

	/********************************* METHODS ***********************************/

	void initializeGL();
	void checkOpengGLErrors();
};

class ESImageGridQuickItem : public QQuickFramebufferObject
{
	Q_OBJECT
	QML_ELEMENT

	friend class ESImageGridQuickItemRenderer;
public:
	/********************************** TYPES *************************************/

	enum SortingMode
	{
		eSortByDatetime = 0,
		eSortBySimilarityScore,
	};

	

	/******************************** ATTRIBUTES **********************************/

	
	/********************************* METHODS ***********************************/

	ESImageGridQuickItem();
	virtual ~ESImageGridQuickItem();

	ES_QML_PROPERTY(FilteredFilesList, const ESListFilesStat*)
	ES_QML_PROPERTY(ImageFiles, QVector<QString>, mDataHasChanged = true; update();)
	ES_QML_PROPERTY(ImageSize, int, mGeometryHasChanged = true; update();)
	ES_QML_PROPERTY(YOffset, float, update();)
	ES_QML_PROPERTY(SortingMode, int, mDataHasChanged = true; update();)
	ES_QML_PROPERTY(Loading, bool)
	ES_QML_PROPERTY(LoadingProgress, float)

	ES_QML_READ_PROPERTY(ContentHeight, int)

	Q_INVOKABLE QString getImageFileAtPos(float pX, float pY) const;
	Q_INVOKABLE QString getPreviousImage(QString pImage, int pPreloadCountAround) const;
	Q_INVOKABLE QString getNextImage(QString pImage, int pPreloadCountAround) const;

	virtual QQuickFramebufferObject::Renderer* createRenderer() const override;

signals:
	/********************************** SIGNALS ***********************************/

private:
	/******************************** ATTRIBUTES **********************************/

	QSizeF mPreviousSize;
	int mNbColumns;
	int mNbRows;

	bool mValid;
	bool mDataHasChanged;
	bool mGeometryHasChanged;

	std::vector<std::shared_ptr<ESImage>> mImages;
	std::vector<float> mPackedImagesYOffsets;
	const ESListFilesStatComponent* mFilteredFilesListComponent;

	/********************************* METHODS ***********************************/

	void updateInternal();
	void onImageCachingProgress(int pLoadedCount, int pLoadingCount);
	void sort();
	void preloadImagesAround(int pImageIdx, int pPreloadCountAround) const;
};
