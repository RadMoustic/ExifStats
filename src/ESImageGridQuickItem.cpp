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

#ifdef Q_OS_ANDROID
constexpr int cMaxDisplayedImages = 256;
#else
constexpr int cMaxDisplayedImages = 512;
#endif

/********************************************************************************/

ESImageGridQuickItem::ESImageGridQuickItem()
	: mFilteredFilesList(nullptr)
	, mTargetImageSize(CACHE_IMAGE_SIZE)
	, mImageSize(CACHE_IMAGE_SIZE)
	, mVisibleImageSize(CACHE_IMAGE_SIZE)
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
	, mZoomCenter(-1,-1)
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

float ESImageGridQuickItem::getMinImageSize() const
{
	return msMinImageSize;
}

/********************************************************************************/

QString ESImageGridQuickItem::getImageFileAtPos(float pX, float pY) const
{
	QString lResult;
	int lIndex = getImageIndexAtPos(pX, pY);
	if (lIndex >= 0 && lIndex < mImages.size())
	{
		const std::shared_ptr<ESImage>& lImage = mImages[lIndex];
#if defined(QT_DEBUG) && defined(IMAGETAGGER_ENABLE) && !defined(EXIFSTATS_READONLY)
		if (!ESImageTaggerManager::getInstance().isLoading())
		{
			ESDatabase& db = ESDatabase::getInstance();
			QStringList lTagLabels = db.getTagsLabels(db.getFileInfo(lImage->getImagePath())->mTagIndexes);
			qDebug() << lTagLabels.join(", ");
		}

		ESImageCache::getInstance().printImageDebugInfo(lImage);
#endif //  defined(QT_DEBUG) && defined(IMAGETAGGER_ENABLE) && !defined(EXIFSTATS_READONLY)
		lResult = lImage->getImagePath();
	}
	
	return lResult;
}

/********************************************************************************/

QGeoCoordinate ESImageGridQuickItem::getImageGeoCoordinateAtPos(float pX, float pY) const
{
	QGeoCoordinate lResult;
	int lIndex = getImageIndexAtPos(pX, pY);
	if (lIndex >= 0 && lIndex < mImages.size())
	{
		const std::shared_ptr<ESImage>& lImage = mImages[lIndex];
		const ESUsefullExif::GeoLocation& lGeoLoc = lImage->getExif().mGeoLocation;
		if (lGeoLoc.mLatitude != 0 || lGeoLoc.mLongitude != 0)
		{
			lResult.setLatitude(lGeoLoc.mLatitude);
			lResult.setLongitude(lGeoLoc.mLongitude);
		}
	}

	return lResult;
}

/********************************************************************************/

int ESImageGridQuickItem::getImageIndexAtPos(float pX, float pY, bool pGetClosestIfNotFound) const
{
	int lIndex = -1;

	int lClosestImageIdx = -1;
	float lCLosestImageVerticalDist = std::numeric_limits<float>::max();

	for(int i = 0; i < mImagesYOffsets.size(); ++i)
	{
		int lImageCol = 1 + i % mNbColumns;

		float lOrientedRatio = mImagesRatios[i];
		float lImageWidth = mNbColumns > 1 && lOrientedRatio < 1 ? mVisibleImageSize * lOrientedRatio : mVisibleImageSize;
		float lImageHeight = mNbColumns == 1 || lOrientedRatio > 1 ? mVisibleImageSize / lOrientedRatio : mVisibleImageSize;
		float lMarginX = (mVisibleImageSize - lImageWidth) / 2.f;
		float lMarginY = std::max(0.f, (mVisibleImageSize - lImageHeight) / 2.f);

		QRectF lImageRect;
		lImageRect.setX((lImageCol-1) * mVisibleImageSize + lMarginX);
		lImageRect.setY(mImagesYOffsets[i] * mVisibleImageSize + lMarginY);
		lImageRect.setWidth(lImageWidth);
		lImageRect.setHeight(lImageHeight);
		
		if(lImageRect.contains(pX,pY))
		{
			lIndex = i;
			break;
		}

		if(pGetClosestIfNotFound)
		{
			float lMinHDist = std::min(abs(pX - lImageRect.left()), abs(pX - lImageRect.right()));
			float lMinVDist = std::min(abs(pY - lImageRect.top()), abs(pY - lImageRect.bottom()));
			float lMaxMinDist = std::max(lMinHDist, lMinVDist);
			if (lMaxMinDist < lCLosestImageVerticalDist)
			{
				lCLosestImageVerticalDist = lMaxMinDist;
				lClosestImageIdx = i;
			}
		}
	}
	if(lIndex == -1)
		lIndex = lClosestImageIdx;
	return lIndex;
}

/********************************************************************************/

QString ESImageGridQuickItem::getPreviousImage(QString pImage, int pPreloadCountAround) const
{
	ESStringId lImagePath = pImage;
	ESStringId lResult;
	int lImageIdx = 0;
	for (const std::shared_ptr<ESImage>& lImage : mImages)
	{
		if(lImagePath == lImage->getImagePath())
		{
			preloadImagesAround(lImageIdx, pPreloadCountAround);
			break;
		}
		lResult = lImage->getImagePath();
		++lImageIdx;
	}

	return lResult;
}

/********************************************************************************/

QString ESImageGridQuickItem::getNextImage(QString pImage, int pPreloadCountAround) const
{
	ESStringId lImagePath = pImage;
	ESStringId lResult;
	bool lBreakNext = false;
	int lImageIdx = 0;
	for (const std::shared_ptr<ESImage>& lImage : mImages)
	{
		if (lBreakNext)
		{
			lResult = lImage->getImagePath();
			break;
		}
		if (lImage->getImagePath() == lImagePath)
		{
			preloadImagesAround(lImageIdx, pPreloadCountAround);
			lBreakNext = true;
		}
		++lImageIdx;
	}

	return lResult;
}

/********************************************************************************/

float ESImageGridQuickItem::scrollViewTo(QString pImage)
{
	QVector2D lImagePos = getImagePos(pImage);
	setYOffset(lImagePos.y());
	return lImagePos.y();
}

/********************************************************************************/

QVector2D ESImageGridQuickItem::getImagePos(QString pImage)
{
	QVector2D lResult(0,0);
	ESStringId lImagePath = pImage;
	int lImageIdx = 0;
	for (const std::shared_ptr<ESImage>& lImage : mImages)
	{
		if (lImage->getImagePath() == lImagePath)
		{
			lResult = getImagePos(lImageIdx);
			break;
		}
		++lImageIdx;
	}

	return lResult;
}

/********************************************************************************/

QVector2D ESImageGridQuickItem::getImagePos(int pIndex) const
{
	QVector2D lResult(0, 0);

	if(!mImagesYOffsets.empty())
		lResult.setY(mImagesYOffsets[pIndex] * mVisibleImageSize);

	return lResult;
}

/********************************************************************************/

void ESImageGridQuickItem::preloadImagesAround(int pImageIdx, int pPreloadCountAround) const
{
	if (pPreloadCountAround > 0 && pImageIdx < mImages.size())
	{
		for (int i = std::max(0, pImageIdx - pPreloadCountAround);
			i < std::min(int(mImages.size()), pImageIdx + pPreloadCountAround);
			++i)
		{
			const std::shared_ptr<ESImage>& lImage = mImages[i];
			if (!lImage->isLoaded())
			{
				lImage->loadImage();
				lImage->updateLastUsed();
			}
		}
	}
}

/********************************************************************************/

#define ESClamp(v,low,high) std::max<float>(low, std::min<float>(high, v))

void ESImageGridQuickItem::updateInternal()
{
	mValid = !ESImageCache::getInstance().isUpdating();

	if(!mValid)
		return;

	if (mDataHasChanged)
	{
		for (std::shared_ptr<ESImage>& lImage : mImages)
		{
			disconnect(lImage.get(), nullptr, this, nullptr);
		}
		mImages.clear();

		auto lGetImage = [this](const QString& pImageFilePath)
			{
				std::shared_ptr<ESImage> lImage = ESImageCache::getInstance().getImage(pImageFilePath);
				if (lImage)
				{
					connect(lImage.get(), &ESImage::imageLoadedOrCanceled, this, [this]() { update(); });
					mImages.push_back(lImage);
				}
			};

		if (mImageFiles.size() > 0)
		{
			mImages.reserve(mImageFiles.size());
			for (const QString& lImageFilePath : mImageFiles)
				lGetImage(lImageFilePath);
		}
		else if (mFilteredFilesList)
		{
			mImages.reserve(mFilteredFilesList->mListFilesComp.mFiles.size());
			for (const ESStringId& lImageFilePath : mFilteredFilesList->mListFilesComp.mFiles)
				lGetImage(lImageFilePath);
		}

		sort();
	}

	if (mGeometryHasChanged || mDataHasChanged)
	{
		int lNbImages = int(mImages.size());

		int lImageAtZoomCenter = -1;
		float lKeepInViewImageYOffset = 0.f;
		if(mNbColumns > 0)
		{
			QVector2D lZoomCenter = mZoomCenter.x() >= 0 && mZoomCenter.y() >= 0 ? mZoomCenter : QVector2D(width()/2.f, height()/2.f);
			lImageAtZoomCenter = getImageIndexAtPos(ESClamp(mZoomCenter.x(), 0, width()), ESClamp(mZoomCenter.y(), 0, height()) + mYOffset, true);
			if(lImageAtZoomCenter == -1)
				lImageAtZoomCenter = lNbImages - 1;
			QVector2D lKeepInViewImagePos = getImagePos(lImageAtZoomCenter);
			lKeepInViewImageYOffset = lKeepInViewImagePos.y() - getYOffset();
			assert(lKeepInViewImageYOffset < height());
		}

		int lPreviousNbColumns = mNbColumns;

		mNbColumns = std::max<int>(1, width() / mImageSize);
		mNbRows = CeilIntDiv(lNbImages, mNbColumns);

		float lNewContentHeight = 1;

		if(mDataHasChanged || lPreviousNbColumns != mNbColumns)
		{
			mImagesYOffsets.clear();
			mImagesRatios.clear();
		}
		if(lNbImages)
		{
			if(mImagesYOffsets.size() == 0)
			{
				mImagesYOffsets.reserve(lNbImages);
				mImagesRatios.reserve(lNbImages);

				float lYOffset = 0.f;
				for(int i = 0; i < mNbRows; ++i)
				{
					float lRowHeight = 0.f;
					for(int j = 0; j < mNbColumns; ++j)
					{
						int lImageIdx = i * mNbColumns + j;
						if(lImageIdx >= lNbImages)
							break;
						float lRatio = mImages[lImageIdx]->getExif().getOrientedRatio();
						float lImageHeight = mNbColumns == 1 ? 1.f / lRatio : std::min(1.0f, 1.f / lRatio);
				
						lRowHeight = std::max(lRowHeight, lImageHeight);

						if (lRatio > 1.f)
						{
							float lImageOffset = (1.f - lImageHeight) / 2.f;
							mImagesYOffsets.push_back(lYOffset - lImageOffset);
						}
						else
						{
							mImagesYOffsets.push_back(lYOffset);
						}
						mImagesRatios.push_back(lRatio);
					}
					lYOffset += lRowHeight;
				}

				lNewContentHeight = lYOffset;
			}
			else
			{
				lNewContentHeight = mImagesYOffsets.back() + (mImageSize / mImages.back()->getExif().getOrientedRatio());
			}

			lNewContentHeight *= mImageSize;
		}

		mVisibleImageSize = mImageSize;

		if (!mDataHasChanged && lImageAtZoomCenter >= 0)
		{
			QVector2D lNewKeepInViewImagePos = getImagePos(lImageAtZoomCenter);
			setYOffset(lNewKeepInViewImagePos.y() - lKeepInViewImageYOffset);
		}
		setContentHeight(std::max(1.f, lNewContentHeight));
				
		mGeometryHasChanged = false;
	}

	mDataHasChanged = false;
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
	: mCurrentTextureSize(0.f)
{
	initializeGL(); checkOpengGLErrors();
}

/********************************************************************************/

void ESImageGridQuickItemRenderer::initializeGL()
{
	if (!mShaderProgram)
	{
		initializeOpenGLFunctions(); checkOpengGLErrors();

#ifdef QT_DEBUG__
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
		QString lGLSLPrecision = lIsOpenGLES ? "precision highp float;\nprecision highp sampler2DArray;\n" : "";

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

		mVAO.reset(new QOpenGLVertexArrayObject());
		mVAO->create(); checkOpengGLErrors();
	}
}

/********************************************************************************/

void ESImageGridQuickItemRenderer::allocateImageTextures(float pTextureSize)
{
	if(!mImageTextures || mCurrentTextureSize != pTextureSize)
	{
		mCurrentTextureSize = pTextureSize;
		mImageTextures.reset(new QOpenGLTexture(QOpenGLTexture::Target2DArray)); checkOpengGLErrors();
		mImageTextures->create(); checkOpengGLErrors();
		mImageTextures->setSize(pTextureSize, pTextureSize); checkOpengGLErrors();
		mImageTextures->setLayers(cMaxDisplayedImages); checkOpengGLErrors();
		mImageTextures->setMipLevels(1); checkOpengGLErrors();
		mImageTextures->setFormat(QOpenGLTexture::RGBA8_UNorm); checkOpengGLErrors();
		//mImageTextures->allocateStorage(); checkOpengGLErrors();
		mImageTextures->bind(); checkOpengGLErrors();
		glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, 0x8058, pTextureSize, pTextureSize, cMaxDisplayedImages); checkOpengGLErrors();
		mImageTextures->release(); checkOpengGLErrors();
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

/*virtual*/ void ESImageGridQuickItemRenderer::synchronize(QQuickFramebufferObject* pItem) /*override*/
{
	ESImageGridQuickItem* lItem = static_cast<ESImageGridQuickItem*>(pItem);

	lItem->mGeometryHasChanged |= lItem->mPreviousSize != lItem->size();
	lItem->mPreviousSize = lItem->size();
	mSize = lItem->size();

	int lTextureSize = std::min(lItem->mTargetImageSize, CACHE_IMAGE_SIZE);

	if(lItem->mDataHasChanged || lTextureSize != mCurrentTextureSize)
	{
		mImageToTextureSlot.clear();
		mFreeImageTextureSlots.clear();
		for (int i = 0; i < cMaxDisplayedImages; ++i)
			mFreeImageTextureSlots.push_back(i);
	}

	lItem->updateInternal();

	if (lItem->mValid)
	{
		assert(mShaderProgram->isLinked() && "Fix Shader Errors");

		allocateImageTextures(lTextureSize);

		mInstanceData.clear();
		mInstanceData.reserve(cMaxDisplayedImages);

		QRectF lBoudingRect = lItem->boundingRect();

		int lStartDrawRow = -1;
		int lEndDrawRow = -1;

		float lTop = lBoudingRect.top() + lItem->mYOffset;
		float lBottom = lBoudingRect.bottom() + lItem->mYOffset;
		lEndDrawRow = int(lItem->mImagesYOffsets.size()) - 1;
		for (int i = 0; i < lItem->mImagesYOffsets.size(); ++i)
		{
			int lRowIndex = i / lItem->mNbColumns;
			const float lYOffset = lItem->mImagesYOffsets[i] * lItem->mVisibleImageSize;
			if(lYOffset > lTop && lStartDrawRow < 0)
				lStartDrawRow = std::max(0, lRowIndex - 2);
			if (lYOffset > lBottom)
			{
				lEndDrawRow = std::max(0, lRowIndex);
				break;
			}
		}

		const int lStartPreloadRow = std::max(0, lStartDrawRow - 5);
		const int lEndPreloadRow = std::min(lItem->mNbRows, lEndDrawRow + 5);

		// Release slots
		for (auto it = mImageToTextureSlot.begin(); it != mImageToTextureSlot.end();)
		{
			int lRow = it->second.mIndex / lItem->mNbColumns;
			if (lRow < lStartDrawRow || lRow > lEndDrawRow)
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

				std::shared_ptr<ESImage>& lImageWrapper = lItem->mImages[lIndex];
				lImageWrapper->updateLastUsed();

				float lImageRatio = lImageWrapper->getExif().getOrientedRatio();
				float lImageQuadSize = lItem->mImageSize;

				float lX = lCol * lItem->mImageSize;
				if(lItem->mNbColumns == 1 && lImageRatio < 1.f)
				{
					lImageQuadSize = lItem->mImageSize / lImageRatio;
					lX -= (lImageQuadSize - lItem->mImageSize) / 2.f;
				}
				float lY = lItem->mImagesYOffsets[lIndex] * lItem->mVisibleImageSize - lItem->mYOffset;

				QRectF lImageRect(lX, lY, lImageQuadSize, lImageQuadSize);

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
							std::shared_ptr<const QImage> lImage = lImageWrapper->getImage();
							QImage lGLImage(mCurrentTextureSize, mCurrentTextureSize, QImage::Format_RGBA8888);
							lGLImage.fill(Qt::transparent);
							QPainter lPainter(&lGLImage);
#ifdef Q_OS_ANDROID
							if(mCurrentTextureSize < 300) // Too slow on android
#endif
								lPainter.setRenderHint(QPainter::SmoothPixmapTransform);
							float lX, lY, lWidth, lHeight;
							if (lImageRatio >= 1.0f)
							{
								lWidth = mCurrentTextureSize;
								lHeight = mCurrentTextureSize / lImageRatio;
								lX = 0.f;
								lY = (mCurrentTextureSize - lHeight) / 2.f;
							}
							else
							{
								lWidth = mCurrentTextureSize * lImageRatio;
								lHeight = mCurrentTextureSize;
								lX = (mCurrentTextureSize - lWidth) / 2.f;
								lY = 0.f;
							}
							lPainter.drawImage(QRectF(lX, lY, lWidth, lHeight), *lImage);
							//mImageTextures->setData(0, lTextureSlot, QOpenGLTexture::RGBA, QOpenGLTexture::UInt8, lScaledImage.constBits()); checkOpengGLErrors();
							glTexSubImage3D(GL_TEXTURE_2D_ARRAY,0,0,0, lTextureSlot, mCurrentTextureSize, mCurrentTextureSize, 1, GL_RGBA, GL_UNSIGNED_BYTE, lGLImage.constBits());  checkOpengGLErrors();
							mImageToTextureSlot[lImageWrapper.get()] = { lTextureSlot, lIndex };
						}
					}
					else
					{
						lTextureSlot = lTextureSlotFound->second.mTextureSlot;
						assert(lTextureSlotFound->second.mIndex == lIndex);
					}
					if (lTextureSlot >= 0)
					{
						mInstanceData.emplace_back(lX, lY, lImageQuadSize, lTextureSlot);
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
		assert(mInstanceData.size() < cMaxDisplayedImages);
		mImageTextures->release();
		mImageTextures->generateMipMaps();
	}
}

/********************************************************************************/

/*virtual*/ void ESImageGridQuickItemRenderer::render() /*override*/
{
	if (!mInstanceData.empty())
	{
		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
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
		mVBOInstances->write(0, mInstanceData.data(), int(mInstanceData.size() * sizeof(ImageInstanceData))); checkOpengGLErrors();

		mShaderProgram->enableAttributeArray(lInstPosAttr);
		mShaderProgram->enableAttributeArray(lInstSizeAttr);
		mShaderProgram->enableAttributeArray(lInstTexIdxAttr);
		mShaderProgram->setAttributeBuffer(lInstPosAttr, GL_FLOAT, 0, 2, sizeof(ImageInstanceData)); checkOpengGLErrors();
		mShaderProgram->setAttributeBuffer(lInstSizeAttr, GL_FLOAT, 8, 1, sizeof(ImageInstanceData)); checkOpengGLErrors();
		mShaderProgram->setAttributeBuffer(lInstTexIdxAttr, GL_FLOAT, 12, 1, sizeof(ImageInstanceData)); checkOpengGLErrors();

		glVertexAttribDivisor(lInstPosAttr, 1);
		glVertexAttribDivisor(lInstSizeAttr, 1);
		glVertexAttribDivisor(lInstTexIdxAttr, 1);

		glEnable(GL_BLEND); checkOpengGLErrors();
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); checkOpengGLErrors();

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
