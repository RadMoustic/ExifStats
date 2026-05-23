#include "ESBarChartQuickItem.h"

// ES
#include "ESMaterialPalette.h"

// Qt
#include <QPainter>

constexpr float cMarkerTextSpacing = 5.f;
constexpr int cNbYMarker = 5;
constexpr float cMarkerHalfWidth = 2.f;

/********************************************************************************/

ESBarChartQuickItem::ESBarChartQuickItem()
	: mBarSpacing(1.)
	, mValueAxisMaxSizeAuto(0.f)
	, mValueAxisSizeAuto(true)
	, mValueAxisSize(0.f)
	, mCategoryAxisMaxSizeAuto(0.f)
	, mCategoryAxisSizeAuto(true)
	, mCategoryAxisSize(0.f)
	, mMargin(5.f)
	, mCategoryAxisOffset(0.f)
	, mCategoryAxisScale(1.f)
	, mValueAxisScale(1.f)
	, mAllCategoriesOnly(false)
	, mCategorySpacing(0.f)
	, mInvertAxis(false)
	, mMaxValue(0)
	, mBarThickness(0.)
	, mBarLengthFactor(1.f)
	, mRealCategoryAxisSize(0.f)
	, mActualBarSpacing(0.)
	, mValid(false)
	, mDataHasChanged(false)
	, mGeometryHasChanged(false)
{
	setRenderTarget(QQuickPaintedItem::FramebufferObject);
}

/********************************************************************************/

QPoint ESBarChartQuickItem::mapToValue(float pX)
{
	if(!mValid || std::isnan(pX))
		return QPoint(-1,-1);
	QPointF lPLotAreaPos = mInvertAxis ? mapToPlotArea(0.f, pX) : mapToPlotArea(pX, 0.f);
	float lX = (mInvertAxis ? lPLotAreaPos.y() : lPLotAreaPos.x()) - mCategoryAxisOffset;
	float lChartFullSize = getChartFullSize();
	if(lX < 0 || lX > lChartFullSize)
		return QPoint(-1, -1);
	int lCategory = int(lX * mCategories.size() / lChartFullSize);
	return QPoint(lCategory, mValues[lCategory]);
}

/********************************************************************************/

QPointF ESBarChartQuickItem::mapToPlotArea(float pX, float pY)
{
	if(mInvertAxis)
		return QPointF(pX - mRealCategoryAxisSize - mMargin, pY - mMargin);
	else
		return QPointF(pX - mValueAxisSize - mMargin, pY - mMargin);
}

/********************************************************************************/

float ESBarChartQuickItem::getChartFullSize() const
{
	return float(mValues.size()) * (mBarThickness * mCategoryAxisScale + std::min(mBarSpacing, mActualBarSpacing * mCategoryAxisScale));
}

/********************************************************************************/

template<typename T>
void InvertRect(T& pRect)
{
	pRect = T(pRect.y(), pRect.x(), pRect.height(), pRect.width());
}

/********************************************************************************/

template<typename T>
void InvertPos(T& pPos)
{
	pPos = T(pPos.y(), pPos.x());
}

/********************************************************************************/

/*virtual*/ void ESBarChartQuickItem::paint(QPainter* pPainter) /*override*/
{
	mGeometryHasChanged = mPreviousSize != size();
	mPreviousSize = size();

	updateInternal();

	if (mValid)
	{
		std::shared_ptr<ESMaterialPalette::PaletteData> lPalette = ESMaterialPalette::getPalette(this);

		pPainter->setPen(Qt::NoPen);
		
		pPainter->setRenderHint(QPainter::Antialiasing);
		pPainter->setRenderHint(QPainter::TextAntialiasing);
		pPainter->fillRect(pPainter->viewport(), Qt::transparent);

		pPainter->save();
		if (mInvertAxis)
			pPainter->setClipRect(mMargin, mMargin, width() - 2.f * mMargin, height() - 2.f * mMargin - mCategoryAxisSize);
		else
			pPainter->setClipRect(mMargin + mValueAxisSize, mMargin, width() - mMargin * 2.0 - mValueAxisSize, height() - mMargin * 2.0);
		QPointF lPos(mCategoryAxisOffset, 0);
		if (mInvertAxis)
			InvertPos(lPos);
		pPainter->translate(lPos);

		// Bars
		float lLeft = mMargin + (mInvertAxis ? 0 : mValueAxisSize);
		float lTextHeight = pPainter->boundingRect(QRectF(), "CAT").height();
		float lMaxBarHeight = mBarLengthFactor * mMaxValue;
		float lScaledBarWidth = mBarThickness * mCategoryAxisScale;
		float lActualBarWidth = std::max(0.5f, lScaledBarWidth);
		float lActualCategoriesTextHeight = lTextHeight * 0.7f;
		bool lCanDisplayAllCategories = lActualCategoriesTextHeight + mCategorySpacing < lScaledBarWidth;
		int lCategoriesToDisplayInterval = lCanDisplayAllCategories ? 1 : int(ceilf((lActualCategoriesTextHeight + mCategorySpacing) / lScaledBarWidth));
		for (int i = 0 ; i < mValues.size() ; ++i)
		{
			int lValue = mValues[i];
			float lBarHeight = std::max(0.001f, std::min(lMaxBarHeight, lValue * mBarLengthFactor * mValueAxisScale));
			QRectF lBarRect(lLeft, (mInvertAxis ? mMargin + mRealCategoryAxisSize : height() - mMargin - lBarHeight - mRealCategoryAxisSize), lActualBarWidth, lBarHeight);
			if (mInvertAxis)
				InvertRect(lBarRect);
			if(pPainter->clipBoundingRect().intersects(lBarRect))
			{
				lPalette->drawForegroundRect(pPainter, lBarRect, 200);

				// Category
				const QString& lCat = mCategories[i];
				if(lCanDisplayAllCategories || !mAllCategoriesOnly)
				{
					if(i % lCategoriesToDisplayInterval == 0)
					{
						pPainter->save();
						pPainter->setPen(lPalette->mForeground);
						if (mInvertAxis)
						{
							QRectF lTextRect(mMargin, lLeft + mBarThickness * mCategoryAxisScale / 2.f - lTextHeight / 2.f + 3, mRealCategoryAxisSize - cMarkerTextSpacing, lTextHeight);
							QRectF lTextBoundingRect = pPainter->boundingRect(QRectF(), Qt::TextSingleLine & Qt::TextDontClip, lCat);
							pPainter->drawText(lTextRect, lTextBoundingRect.width() > lTextRect.width() ? 0 : Qt::AlignRight, lCat);
						}
						else
						{
							pPainter->translate(lLeft + mBarThickness * mCategoryAxisScale / 2.f - lTextHeight / 2.f + 3, mInvertAxis ? mMargin : height() - mRealCategoryAxisSize - mMargin + cMarkerTextSpacing);
							pPainter->rotate(90);
							pPainter->drawText(0,0, lCat);
						}
						pPainter->restore();
					}
				}
			}

			lLeft += mBarThickness * mCategoryAxisScale + std::min(mBarSpacing, mActualBarSpacing * mCategoryAxisScale);
		}

		pPainter->restore();

		// Axis
		pPainter->setPen(lPalette->mForeground);
		pPainter->setBrush(Qt::NoBrush);

		// Y
		float lScaledMaxValue = float(mMaxValue) / mValueAxisScale;

		float lInterval = lScaledMaxValue / cNbYMarker;
		float lMult = 1;
		while (lInterval >= 1.0)
		{
			lMult *= 10;
			lInterval /= 10;
		}
		float lRounded = round(lInterval * 2.0) / 2.0;
		lInterval = lRounded == 0.f ? lMult / 10.f : lRounded * lMult;

		int lNbMarker = ceil(float(lScaledMaxValue) / lInterval);
		float lMarkerSpacing = ((mInvertAxis ? width() : height()) - mMargin * 2.f - mRealCategoryAxisSize) / lScaledMaxValue;

		if (mInvertAxis)
			pPainter->drawLine(QLineF(mMargin + mRealCategoryAxisSize, height() - mValueAxisSize - mMargin, width() - mMargin, height() - mValueAxisSize - mMargin));
		else
			pPainter->drawLine(QLineF(mValueAxisSize + mMargin, mMargin, mValueAxisSize + mMargin, height() - mRealCategoryAxisSize - mMargin));

		pPainter->save();
		QPen lPen(lPalette->mForeground);
		lPen.setStyle(Qt::DashLine);
		lPen.setWidthF(1.0f);
		pPainter->setPen(lPen);

		for (int i = 1; i < lNbMarker; ++i)
		{
			int lMarkerValue = lInterval * i;
			float lMarkerY;
			if (mInvertAxis)
				lMarkerY = mRealCategoryAxisSize + mMargin + lMarkerSpacing * lMarkerValue;
			else
				lMarkerY = height() - mRealCategoryAxisSize - mMargin - lMarkerSpacing * lMarkerValue;
			if (mInvertAxis)
				pPainter->drawLine(QLineF(lMarkerY, mMargin * 2.f, lMarkerY, height() - mValueAxisSize - cMarkerHalfWidth));
			else
				pPainter->drawLine(QLineF(mValueAxisSize + mMargin - cMarkerHalfWidth, lMarkerY, width() - mValueAxisSize - mMargin*2.f, lMarkerY));

			QString lMarkerText = QString::number(lMarkerValue);
			if (mInvertAxis)
			{
				pPainter->save();
				pPainter->translate(lMarkerY - lTextHeight / 2.f + 3, height() - mMargin - mValueAxisSize + cMarkerTextSpacing);
				pPainter->rotate(90);
				pPainter->drawText(0, 0, lMarkerText);
				pPainter->restore();
			}
			else
			{
				QRectF lTextBoundingRect = pPainter->boundingRect(QRectF(), Qt::AlignVCenter, lMarkerText);
				pPainter->drawText(mValueAxisSize + mMargin - lTextBoundingRect.width() - cMarkerTextSpacing, lMarkerY - lTextBoundingRect.y() - 3, lMarkerText); // why 3 no idea
			}
		}
		pPainter->restore();

		// X
		if (mInvertAxis)
			pPainter->drawLine(QLineF(mRealCategoryAxisSize + mMargin, mMargin, mRealCategoryAxisSize + mMargin, height() - mValueAxisSize - mMargin));
		else
			pPainter->drawLine(QLineF(mValueAxisSize + mMargin, height() - mRealCategoryAxisSize - mMargin, width() - mMargin, height() - mRealCategoryAxisSize - mMargin));
	}
}

/********************************************************************************/

void ESBarChartQuickItem::updateInternal()
{
	mValid = mCategories.size() == mValues.size() && mCategories.size() > 0;

	if(!mValid)
		return;

	if (mDataHasChanged)
	{
		mMaxValue = mValues.size() == 0 ? 0 : *std::max_element(mValues.begin(), mValues.end());

		if (mCategoryAxisSizeAuto)
		{
			QPixmap lPixmap(10,10);
			QPainter lPainter(&lPixmap);
			mRealCategoryAxisSize = 0;
			for (const QString& lCat : mCategories)
				mRealCategoryAxisSize = std::max(mRealCategoryAxisSize, lPainter.boundingRect(QRectF(), Qt::TextSingleLine & Qt::TextDontClip, lCat).width());

			mRealCategoryAxisSize += mMargin + mMargin + cMarkerTextSpacing;
			if(mCategoryAxisMaxSizeAuto > 0.f)
				mRealCategoryAxisSize = std::min<float>(mRealCategoryAxisSize, mCategoryAxisMaxSizeAuto);
		}
		else
		{
			mRealCategoryAxisSize = mCategoryAxisSize;
		}
	}
	if (mGeometryHasChanged || mDataHasChanged)
	{
		float lContentCategorySize = (mInvertAxis ? height() : width()) - mValueAxisSize - 2.f * mMargin;
		float lContentValuesSize = (mInvertAxis ? width() : height()) - mRealCategoryAxisSize - 2.f * mMargin;

		float lBarSpacingSum = mBarSpacing * (mCategories.size() - 1);
		if (lBarSpacingSum >= 0.5*lContentCategorySize) // Can't have at least 1px wide bars
		{
			mActualBarSpacing = 0.5 * lContentCategorySize / mCategories.size();
			lBarSpacingSum = mActualBarSpacing * (mCategories.size() - 1);
		}
		else
		{
			mActualBarSpacing = mBarSpacing;
		}
		mBarThickness = (lContentCategorySize - lBarSpacingSum) / mCategories.size();
		mBarLengthFactor = lContentValuesSize / mMaxValue;
	}

	mDataHasChanged = false;
	mGeometryHasChanged = false;
}
