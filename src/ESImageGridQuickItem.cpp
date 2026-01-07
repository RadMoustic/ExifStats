#include "ESImageGridQuickItem.h"

// ES
#include "ESExifStatListFiles.h"
#include "ESDatabase.h"

// Qt
#include <QPainter>

constexpr float cMarkerTextSpacing = 5.f;
constexpr int cNbYMarker = 5;
constexpr float cMarkerHalfWidth = 2.f;

/********************************************************************************/

ESImageGridQuickItem::ESImageGridQuickItem()
	: mFilteredFilesList(nullptr)
	, mImageWidth(250)
	, mImageHeight(250)
	, mContentHeight(100)
	, mYOffset(0.f)
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

	connect(&ESImageCache::getInstance(), &ESImageCache::imageCachingProgressUpdated, this, &ESImageGridQuickItem::onImageCachingProgressUpdated);

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
			connect(mFilteredFilesListComponent, &ESExifStatListFilesComponent::listFilesChanged, this, 
			[this]()
			{
				setYOffset(0.f);
				mDataHasChanged = true;
				updateInternal();
			});
		}

		mDataHasChanged = true;
		updateInternal();
	});

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

QString ESImageGridQuickItem::getImageFileAtPos(float pX, float pY) const
{
	int col = static_cast<int>(pX / mImageWidth);
	int row = static_cast<int>((pY + mYOffset) / mImageHeight);
	int lIndex = row * mNbColumns + col;
	if(lIndex >= 0 && lIndex < mImages.size())
	{
		const std::shared_ptr<ESImage>& lImage = mImages[lIndex];
#ifdef QT_DEBUG
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

/*virtual*/ void ESImageGridQuickItem::paint(QPainter* pPainter) /*override*/
{
	mGeometryHasChanged = mPreviousSize != size();
	mPreviousSize = size();

	updateInternal();

	if (mValid)
	{
		pPainter->setPen(Qt::NoPen);
		pPainter->setBrush(Qt::lightGray);
		if(mImageWidth > CACHE_IMAGE_SIZE || mImageHeight > CACHE_IMAGE_SIZE)
			pPainter->setRenderHint(QPainter::SmoothPixmapTransform);
			
		for(int row = 0 ; row < mNbRows ; ++row)
		{
			for(int col = 0 ; col < mNbColumns ; ++col)
			{
				int lIndex = row * mNbColumns + col;

				if(lIndex >= mImages.size())
					break;

				float lX = col * mImageWidth;
				float lY = row * mImageHeight - mYOffset;
				QRectF lImageRect(lX, lY, mImageWidth, mImageHeight);
				if(!boundingRect().intersects(lImageRect))
					continue;
				std::shared_ptr<ESImage>& lImageWrapper = mImages[lIndex];
				lImageWrapper->updateLastUsed();

				if(!lImageWrapper->isLoaded() && !lImageWrapper->isLoading())
					lImageWrapper->loadImage();
				
				if (lImageWrapper->isLoaded() && !lImageWrapper->isNull())
				{
					const QImage& lImage = lImageWrapper->getImage();

					QRectF lSourceRect(0, 0, lImage.width(), lImage.height());
					float imgAspectRatio = static_cast<float>(lImage.width()) / static_cast<float>(lImage.height());
					float cellAspectRatio = static_cast<float>(mImageWidth) / static_cast<float>(mImageHeight);
					if(imgAspectRatio > cellAspectRatio)
					{
						// Image is wider than cell
						float lNewHeight = mImageWidth / imgAspectRatio;
						lImageRect.setY(lY + (mImageHeight - lNewHeight) * 0.5f);
						lImageRect.setHeight(lNewHeight);
					}
					else
					{
						// Image is taller than cell
						float lNewWidth = mImageHeight * imgAspectRatio;
						lImageRect.setX(lX + (mImageWidth - lNewWidth) * 0.5f);
						lImageRect.setWidth(lNewWidth);
					}
					pPainter->drawImage(lImageRect, lImage, lSourceRect);
				}
				else
				{
					QPointF lCenter = lImageRect.center();
					constexpr float cHalfSize = 8;
					pPainter->setBrush(lImageWrapper->isLoaded() ? QColor(150, 0, 0) : QColor(220,220,220));
					pPainter->drawRoundedRect(QRectF(lCenter.x() - cHalfSize, lCenter.y() - cHalfSize, 2.f * cHalfSize, 2.f * cHalfSize), 5, 5);
				}
			}
		}
	}
	else
	{
		pPainter->setRenderHint(QPainter::Antialiasing);
		QFont lFont;
		lFont.setPointSizeF(20.f);
		pPainter->setFont(lFont);
		pPainter->drawText(boundingRect(), "Loading...", QTextOption(Qt::AlignCenter));
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
		int nbImages = int(mImageFiles.size() > 0 ? mImageFiles.size() : (mFilteredFilesList ? mFilteredFilesList->mListFilesComp.mFiles.size() : 0));

		mNbColumns = std::max<int>(1, width() / mImageWidth);
		mNbRows = CeilIntDiv(nbImages, mNbColumns);
		setContentHeight(std::max(1, mNbRows * mImageHeight));
		mGeometryHasChanged = false;
	}

	if(mDataHasChanged)
	{
		for(std::shared_ptr<ESImage>& image: mImages)
		{
			disconnect(image.get(), nullptr, this, nullptr);
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
			for(const QString& imageFilePath: mImageFiles)
				lGetImage(imageFilePath);
		}
		else if (mFilteredFilesList)
		{
			mImages.reserve(mFilteredFilesList->mListFilesComp.mFiles.size());
			for (const StringId& imageFilePath : mFilteredFilesList->mListFilesComp.mFiles)
				lGetImage(imageFilePath);
		}

		sort();

		mDataHasChanged = false;
	}
}

/********************************************************************************/

void ESImageGridQuickItem::onImageCachingProgressUpdated(int pCachedCount, int pCachingCount)
{
	if (pCachingCount == 0)
	{
		setLoadingProgress(100.f);
		setLoading(false);
	}
	else
	{
		setLoading(true);
		float loadingProgress = static_cast<float>(pCachedCount) / pCachingCount;
		if (abs(loadingProgress - getLoadingProgress()) >= 0.001)
			setLoadingProgress(loadingProgress);
	}
}

/********************************************************************************/

void ESImageGridQuickItem::sort()
{
	std::sort(mImages.begin(), mImages.end(),
	[](const std::shared_ptr<ESImage>& a, const std::shared_ptr<ESImage>& b)
	{
		return a->getExif().mDateTime < b->getExif().mDateTime;
	});
}