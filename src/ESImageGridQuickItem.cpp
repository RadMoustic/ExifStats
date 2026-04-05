#include "ESImageGridQuickItem.h"

// ES
#include "ESDatabase.h"
#include "ESImageTaggerManager.h"
#include "ESImageCache.h"

// Qt
#include <QPainter>
#include <QOpenGLTexture>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLBuffer>
#include <QOpenGLFramebufferObject>
#include <QOpenGLDebugLogger>

/********************************************************************************/

constexpr float cMarkerTextSpacing = 5.f;
constexpr int cNbYMarker = 5;
constexpr float cMarkerHalfWidth = 2.f;
constexpr int cMaxDisplayedImages = 256;

/********************************************************************************/

ESImageGridQuickItem::ESImageGridQuickItem()
	: mFilteredFilesList(nullptr)
	, mImageSize(CACHE_IMAGE_SIZE)
	, mContentHeight(100)
	, mYOffset(0.f)
	, mSortingMode(int(eSortByDatetime))
	, mLoading(false)
	, mLoadingProgress(0.f)
	, mNbRows(0)
	, mNbColumns(0)
	, mValid(false)
	, mDataHasChanged(false)
	, mGeometryHasChanged(false)
	, mFilteredFilesListComponent(nullptr)
{
	setFlag(ItemHasContents, true);
	setAcceptedMouseButtons(Qt::AllButtons);

	connect(&ESImageCache::getInstance(), &ESImageCache::imageLoadingProgress, this, &ESImageGridQuickItem::onImageCachingProgress, Qt::DirectConnection);

	connect(this, &ESImageGridQuickItem::propertyFilteredFilesListChanged, this, 
	[this]()
	{
		if (mFilteredFilesListComponent)
		{
			disconnect(mFilteredFilesListComponent, nullptr, this, nullptr);
		}
		mFilteredFilesListComponent = mFilteredFilesList ? &mFilteredFilesList->mListFilesComp : nullptr;
		if(mFilteredFilesListComponent)
		{
			connect(mFilteredFilesListComponent, &ESListFilesStatComponent::listFilesChanged, this, 
			[this]()
			{
				setYOffset(0.f);
				mDataHasChanged = true;
				update();
			});
		}

		mDataHasChanged = true;
		update();
	}, Qt::QueuedConnection);

	connect(&ESImageCache::getInstance(), &ESImageCache::updateFinished, this, 
	[this]()
	{
		mDataHasChanged = true; // All cache images have changed, don't keep the old images
		update();
	});
}

/********************************************************************************/

/*virtual*/ ESImageGridQuickItem::~ESImageGridQuickItem()
{
}

/********************************************************************************/

/*virtual*/ QQuickFramebufferObject::Renderer* ESImageGridQuickItem::createRenderer() const /*override*/
{
	return new ESImageGridQuickItemRenderer();
}

/********************************************************************************/

QString ESImageGridQuickItem::getImageFileAtPos(float pX, float pY) const
{
	int lCol = static_cast<int>(pX / mImageSize);
	int lRow = static_cast<int>((pY + mYOffset) / mImageSize);
	int lIndex = lRow * mNbColumns + lCol;
	if(lIndex >= 0 && lIndex < mImages.size())
	{
		const std::shared_ptr<ESImage>& lImage = mImages[lIndex];
#if defined(QT_DEBUG) && defined(IMAGETAGGER_ENABLE)
		if(!ESImageTaggerManager::getInstance().isLoading())
		{
			ESDatabase& db = ESDatabase::getInstance();
			QStringList lTagLabels = db.getTagsLabels(db.getFileInfo(lImage->getImagePath())->mTagIndexes);
			qDebug()  << lTagLabels.join(", ");
		}

		ESImageCache::getInstance().printImageDebugInfo(lImage);
#endif
		return lImage->getImagePath();
	}
	else
	{
		return QString();
	}
}

/********************************************************************************/

void ESImageGridQuickItem::updateInternal()
{
	mValid = !ESImageCache::getInstance().isUpdating();

	if(!mValid)
		return;

	if (mGeometryHasChanged || mDataHasChanged)
	{
		int lNbImages = int(mImageFiles.size() > 0 ? mImageFiles.size() : (mFilteredFilesList ? mFilteredFilesList->mListFilesComp.mFiles.size() : 0));

		mNbColumns = std::max<int>(1, width() / mImageSize);
		mNbRows = CeilIntDiv(lNbImages, mNbColumns);
		setContentHeight(std::max(1, mNbRows * mImageSize));

		mGeometryHasChanged = false;
	}

	if(mDataHasChanged)
	{
		for(std::shared_ptr<ESImage>& lImage: mImages)
		{
			disconnect(lImage.get(), nullptr, this, nullptr);
		}
		mImages.clear();

		auto lGetImage = [this](const QString& pImageFilePath)
			{
				std::shared_ptr<ESImage> lImage = ESImageCache::getInstance().getImage(pImageFilePath);
				if(lImage)
				{
					connect(lImage.get(), &ESImage::imageLoadedOrCanceled, this, [this]() { update(); });
					mImages.push_back(lImage);
				}
			};

		if(mImageFiles.size() > 0)
		{
			mImages.reserve(mImageFiles.size());
			for(const QString& lImageFilePath: mImageFiles)
				lGetImage(lImageFilePath);
		}
		else if (mFilteredFilesList)
		{
			mImages.reserve(mFilteredFilesList->mListFilesComp.mFiles.size());
			for (const ESStringId& lImageFilePath : mFilteredFilesList->mListFilesComp.mFiles)
				lGetImage(lImageFilePath);
		}

		sort();

		mDataHasChanged = false;
	}
}

/********************************************************************************/

void ESImageGridQuickItem::onImageCachingProgress(int pLoadedCount, int pLoadingCount)
{
	if (pLoadedCount == pLoadingCount)
	{
		setLoadingProgress(100.f);
		setLoading(false);
	}
	else
	{
		setLoading(true);
		float lLoadingProgress = static_cast<float>(pLoadedCount) / pLoadingCount;
		if (abs(lLoadingProgress - getLoadingProgress()) >= 0.001)
			setLoadingProgress(lLoadingProgress);
	}
}

/********************************************************************************/

void ESImageGridQuickItem::sort()
{
	std::sort(mImages.begin(), mImages.end(),
	[&](const std::shared_ptr<ESImage>& a, const std::shared_ptr<ESImage>& b)
	{
		if(mSortingMode == int(eSortBySimilarityScore) && (a->mCurrentSearchSimilarity > 0 || b->mCurrentSearchSimilarity > 0))
			return a->mCurrentSearchSimilarity > b->mCurrentSearchSimilarity;
		else
			return a->getExif().mDateTime < b->getExif().mDateTime;
	});
}

/********************************************************************************/

ESImageGridQuickItemRenderer::ESImageGridQuickItemRenderer()
{
    initializeGL(); checkOpengGLErrors();
}

/********************************************************************************/

void ESImageGridQuickItemRenderer::initializeGL()
{
	if (!mShaderProgram)
	{
		initializeOpenGLFunctions(); checkOpengGLErrors();

#ifdef QT_DEBUG
		QOpenGLDebugLogger* lLogger = new QOpenGLDebugLogger(nullptr);
		if (lLogger->initialize())
		{
			QObject::connect(lLogger, &QOpenGLDebugLogger::messageLogged,
				[](const QOpenGLDebugMessage& pMsg)
				{
                    qWarning() << "GL ERROR:" << pMsg.message();
				});
			lLogger->startLogging();
		}
#endif // QT_DEBUG

		const bool lIsOpenGLES = QOpenGLContext::currentContext()->isOpenGLES();
		QString lGLSLVersion = lIsOpenGLES ? "#version 300 es\n" : "#version 330 core\n";
		QString lGLSLPrecision = lIsOpenGLES ? "precision mediump float;\nprecision mediump sampler2DArray;\n" : "";

		mShaderProgram.reset(new QOpenGLShaderProgram());

		bool lVertexShaderCompilationResult = mShaderProgram->addShaderFromSourceCode(QOpenGLShader::Vertex,
			lGLSLVersion +
			"in vec2 pos;\n"
			"in vec2 instancePos;\n"
			"in float instanceSize;\n"
			"in float textureIndex;\n"
			"out vec3 vTexCoord;\n"
			"uniform mat4 matrix;\n"
			"void main()\n"
			"{\n"
			"   vec2 worldPos = instancePos + pos * instanceSize;\n"
			"   gl_Position = matrix * vec4(worldPos, 0.0, 1.0);\n"
			"   vTexCoord = vec3(pos.x, pos.y, textureIndex);\n"
			"}\n"); checkOpengGLErrors();

		bool lFragShaderCompilationResult = mShaderProgram->addShaderFromSourceCode(QOpenGLShader::Fragment,
			lGLSLVersion +
			lGLSLPrecision +
			"in vec3 vTexCoord;\n"
			"out vec4 fragColor;\n"
			"uniform sampler2DArray texArray;\n"
			"void main()\n"
			"{\n"
            "   fragColor = vTexCoord.z >= 0.f ? texture(texArray, vTexCoord) : vec4(0.75f,0.75f,0.75f,1);\n"
			"}\n"); checkOpengGLErrors();

		Q_UNUSED(lVertexShaderCompilationResult);
		Q_UNUSED(lFragShaderCompilationResult);
		assert(lVertexShaderCompilationResult && lFragShaderCompilationResult);

		mShaderProgram->link(); checkOpengGLErrors();

		float lQuad[] = { 0,0, 1,0, 1,1, 0,0, 1,1, 0,1 };
		mVBOGeometry.reset(new QOpenGLBuffer(QOpenGLBuffer::VertexBuffer));
		mVBOGeometry->create();
		mVBOGeometry->bind(); checkOpengGLErrors();
        mVBOGeometry->allocate(lQuad, sizeof(lQuad)); checkOpengGLErrors();

		mVBOInstances.reset(new QOpenGLBuffer(QOpenGLBuffer::VertexBuffer));
		mVBOInstances->create();
		mVBOInstances->bind(); checkOpengGLErrors();
        mVBOInstances->allocate(cMaxDisplayedImages * sizeof(ImageInstanceData)); checkOpengGLErrors();

        mImageTextures.reset(new QOpenGLTexture(QOpenGLTexture::Target2DArray));checkOpengGLErrors();
        mImageTextures->create();checkOpengGLErrors();
        mImageTextures->setSize(CACHE_IMAGE_SIZE, CACHE_IMAGE_SIZE);checkOpengGLErrors();
        mImageTextures->setLayers(cMaxDisplayedImages);checkOpengGLErrors();
        mImageTextures->setMipLevels(1);checkOpengGLErrors();
        mImageTextures->setFormat(QOpenGLTexture::RGBA8_UNorm);checkOpengGLErrors();
        //mImageTextures->allocateStorage(); checkOpengGLErrors();
        mImageTextures->bind();
        glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, 0x8058, CACHE_IMAGE_SIZE, CACHE_IMAGE_SIZE, cMaxDisplayedImages);checkOpengGLErrors();
        mImageTextures->release();

		mVAO.reset(new QOpenGLVertexArrayObject());
        mVAO->create(); checkOpengGLErrors();
	}
}

/********************************************************************************/

/*virtual*/ QOpenGLFramebufferObject* ESImageGridQuickItemRenderer::createFramebufferObject(const QSize& pSize) /*override*/
{
	QOpenGLFramebufferObjectFormat lFormat;
	lFormat.setAttachment(QOpenGLFramebufferObject::NoAttachment);
	return new QOpenGLFramebufferObject(pSize, lFormat);
}

/********************************************************************************/

void ESImageGridQuickItemRenderer::checkOpengGLErrors()
{
#ifdef QT_DEBUG
	GLenum lError = glGetError();
	while (lError != GL_NO_ERROR)
	{
		qWarning() << QString("OpenGL error: %1").arg(lError);
        ES_BREAKPOINT();
		lError = glGetError();
	}
#endif
}

/********************************************************************************/

/*virtual*/ void ESImageGridQuickItemRenderer::synchronize(QQuickFramebufferObject* pIem) /*override*/
{
	ESImageGridQuickItem* lItem = static_cast<ESImageGridQuickItem*>(pIem);

	lItem->mGeometryHasChanged |= lItem->mPreviousSize != lItem->size();
	lItem->mPreviousSize = lItem->size();
	mSize = lItem->size();

	if(lItem->mDataHasChanged || lItem->mGeometryHasChanged)
	{
		// Rows are invalidated so clear all texture slots
		mImageToTextureSlot.clear();
		mFreeImageTextureSlots.clear();
		for (int i = 0; i < cMaxDisplayedImages; ++i)
			mFreeImageTextureSlots.push_back(i);
	}

	lItem->updateInternal();

    if (lItem->mValid)
	{
		assert(mShaderProgram->isLinked() && "Fix Shader Errors");

		mInstanceData.clear();
		mInstanceData.reserve(cMaxDisplayedImages);

		QRectF lBoudingRect = lItem->boundingRect();
		QRectF lPreloadRect = lBoudingRect;
		lPreloadRect.setY(lPreloadRect.y() - 3.f * lItem->mImageSize);
		lPreloadRect.setHeight(lPreloadRect.height() + 2.f * 3.f * lItem->mImageSize);

		const int lStartDrawRow = lItem->mYOffset / lItem->mImageSize;
		const int lEndDrawRow = lStartDrawRow + lBoudingRect.height() / lItem->mImageSize + 1;

		const int lStartPreloadRow = lItem->mYOffset / lItem->mImageSize; // No need  to preload rows before because they are already in the cache
		const int lEndPreloadRow = lStartPreloadRow + lPreloadRect.height() / lItem->mImageSize + 1;

		// Release slots
		for (auto it = mImageToTextureSlot.begin(); it != mImageToTextureSlot.end();)
		{
			if (it->second.mRow < lStartDrawRow || it->second.mRow > lEndDrawRow)
			{
				mFreeImageTextureSlots.push_back(it->second.mTextureSlot);
				it = mImageToTextureSlot.erase(it);
			}
			else
			{
				++it;
			}
		}

        mImageTextures->bind();
		for (int lRow = lStartPreloadRow; lRow < lEndPreloadRow; ++lRow)
		{
			for (int lCol = 0; lCol < lItem->mNbColumns; ++lCol)
			{
				int lIndex = lRow * lItem->mNbColumns + lCol;

				if (lIndex >= lItem->mImages.size())
					break;

				float lX = lCol * lItem->mImageSize;
				float lY = lRow * lItem->mImageSize - lItem->mYOffset;
				QRectF lImageRect(lX, lY, lItem->mImageSize, lItem->mImageSize);

				if (!lPreloadRect.intersects(lImageRect))
					continue;

				std::shared_ptr<ESImage>& lImageWrapper = lItem->mImages[lIndex];
				lImageWrapper->updateLastUsed();

				if (!lImageWrapper->isLoaded() && !lImageWrapper->isLoading())
					lImageWrapper->loadImage();

				if (!lBoudingRect.intersects(lImageRect))
					continue;

				bool lDrawPlaceholder = true;
				if (lImageWrapper->isLoaded() && !lImageWrapper->isNull())
				{
					int lTextureSlot = -1;
					auto lTextureSlotFound = mImageToTextureSlot.find(lImageWrapper.get());
					if (lTextureSlotFound == mImageToTextureSlot.end())
					{
						if (!mFreeImageTextureSlots.empty())
						{
							lTextureSlot = mFreeImageTextureSlots.back();
							mFreeImageTextureSlots.pop_back();
							const QImage& lImage = lImageWrapper->getImage();
							QImage lGLImage(CACHE_IMAGE_SIZE, CACHE_IMAGE_SIZE, QImage::Format_RGBA8888);
							lGLImage.fill(Qt::transparent);
							QPainter lPainter(&lGLImage);
							int x = (CACHE_IMAGE_SIZE - lImage.width()) / 2;
							int y = (CACHE_IMAGE_SIZE - lImage.height()) / 2;
							lPainter.drawImage(x, y, lImage);
                            //mImageTextures->setData(0, lTextureSlot, QOpenGLTexture::RGBA, QOpenGLTexture::UInt8, lScaledImage.constBits()); checkOpengGLErrors();
                            glTexSubImage3D(GL_TEXTURE_2D_ARRAY,0,0,0, lTextureSlot, CACHE_IMAGE_SIZE, CACHE_IMAGE_SIZE, 1, GL_RGBA, GL_UNSIGNED_BYTE, lGLImage.constBits());  checkOpengGLErrors();
							mImageToTextureSlot[lImageWrapper.get()] = { lTextureSlot, lRow };
						}
					}
					else
					{
						lTextureSlot = lTextureSlotFound->second.mTextureSlot;
						assert(lTextureSlotFound->second.mRow == lRow);
					}
					if (lTextureSlot >= 0)
					{
						mInstanceData.emplace_back(lX, lY, lItem->mImageSize, lTextureSlot);
						lDrawPlaceholder = false;
					}
				}

				if (lDrawPlaceholder)
				{
					QPointF lCenter = lImageRect.center();
					constexpr float cHalfSize = 8;
					mInstanceData.emplace_back(lCenter.x() - cHalfSize, lCenter.y() - cHalfSize, 2.f * cHalfSize, -1);
				}
			}
		}
        mImageTextures->release();
	}
}

/********************************************************************************/

/*virtual*/ void ESImageGridQuickItemRenderer::render() /*override*/
{
	if (!mInstanceData.empty())
	{
		glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		mShaderProgram->bind(); checkOpengGLErrors();

		int lPosAttr = mShaderProgram->attributeLocation("pos");
		int lInstPosAttr = mShaderProgram->attributeLocation("instancePos");
		int lInstSizeAttr = mShaderProgram->attributeLocation("instanceSize");
		int lInstTexIdxAttr = mShaderProgram->attributeLocation("textureIndex");

		assert(lPosAttr >= 0 && lInstPosAttr >= 0 && lInstSizeAttr >= 0 && lInstTexIdxAttr >= 0 && "Fix Shader Errors");

		QMatrix4x4 lMatrix;
		lMatrix.ortho(0, mSize.width(), 0, mSize.height(), -1, 1);
		mShaderProgram->setUniformValue("matrix", lMatrix);

		mImageTextures->bind(0); checkOpengGLErrors();
		mShaderProgram->setUniformValue("texArray", 0);

		mVAO->bind();

		mVBOGeometry->bind(); checkOpengGLErrors();

		static_assert(sizeof(ImageInstanceData) == 16);

		mShaderProgram->enableAttributeArray(lPosAttr);
		mShaderProgram->setAttributeBuffer(lPosAttr, GL_FLOAT, 0, 2);
		glVertexAttribDivisor(lPosAttr, 0);

		mVBOInstances->bind(); checkOpengGLErrors();
		mVBOInstances->write(0, mInstanceData.data(), int(mInstanceData.size() * sizeof(ImageInstanceData)));

		mShaderProgram->enableAttributeArray(lInstPosAttr);
		mShaderProgram->enableAttributeArray(lInstSizeAttr);
		mShaderProgram->enableAttributeArray(lInstTexIdxAttr);
		mShaderProgram->setAttributeBuffer(lInstPosAttr, GL_FLOAT, 0, 2, sizeof(ImageInstanceData));
		mShaderProgram->setAttributeBuffer(lInstSizeAttr, GL_FLOAT, 8, 1, sizeof(ImageInstanceData));
		mShaderProgram->setAttributeBuffer(lInstTexIdxAttr, GL_FLOAT, 12, 1, sizeof(ImageInstanceData));

		glVertexAttribDivisor(lInstPosAttr, 1);
		glVertexAttribDivisor(lInstSizeAttr, 1);
		glVertexAttribDivisor(lInstTexIdxAttr, 1);

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		glDrawArraysInstanced(GL_TRIANGLES, 0, 6, int(mInstanceData.size())); checkOpengGLErrors();

		glDisable(GL_BLEND);

		glVertexAttribDivisor(lPosAttr, 0);
		glVertexAttribDivisor(lInstPosAttr, 0);
		glVertexAttribDivisor(lInstSizeAttr, 0);
		glVertexAttribDivisor(lInstTexIdxAttr, 0);
		mShaderProgram->disableAttributeArray(lPosAttr);
		mShaderProgram->disableAttributeArray(lInstPosAttr);
		mShaderProgram->disableAttributeArray(lInstSizeAttr);
		mShaderProgram->disableAttributeArray(lInstTexIdxAttr);
		mImageTextures->release();
		mShaderProgram->release();
		mVBOGeometry->release();
		mVBOInstances->release();
		mVAO->release();
	}
}
