#include "ESBarChartQuickItem.h"

// Qt
#include <QPainter>

constexpr float cMarkerTextSpacing = 5.f;
constexpr int cNbYMarker = 5;
constexpr float cMarkerHalfWidth = 2.f;

/********************************************************************************/

ESBarChartQuickItem::ESBarChartQuickItem()
	: mBarSpacing(1.)
	, mYAxisWidthAuto(true)
	, mYAxisWidth(0.f)
	, mXAxisHeightAuto(true)
	, mXAxisHeight(0.f)
	, mMargin(5.f)
	, mXOffset(0.f)
	, mXScale(1.f)
	, mYScale(1.f)
	, mAllCategoriesOnly(false)
	, mCategorySpacing(0.f)
	, mMaxValue(0)
	, mBarWidth(0.)
	, mBarHeightFactor(1.f)
	, mAvailableHeight(0.f)
	, mRealXAxisHeight(0.f)
	, mActualBarSpacing(0.)
	, mValid(false)
	, mDataHasChanged(false)
	, mGeometryHasChanged(false)
{

}

/********************************************************************************/

QPoint ESBarChartQuickItem::mapToValue(float pX)
{
	if(!mValid || std::isnan(pX))
		return QPoint(-1,-1);
	QPointF lPLotAreaPos = mapToPlotArea(pX, 0.f);
	float lX = lPLotAreaPos.x() - mXOffset;
	float lChartFullWidth = getChartFullWidth();
	if(lX < 0 || lX > lChartFullWidth)
		return QPoint(-1, -1);
	int lCategory = int(lX * mCategories.size() / lChartFullWidth);
	return QPoint(lCategory, mValues[lCategory]);
}

/********************************************************************************/

QPointF ESBarChartQuickItem::mapToPlotArea(float pX, float pY)
{
	return QPointF(pX - mYAxisWidth - mMargin, pY - mMargin);
}

/********************************************************************************/

float ESBarChartQuickItem::getChartFullWidth() const
{
	return float(mValues.size()) * (mBarWidth * mXScale + std::min(mBarSpacing, mActualBarSpacing * mXScale));
}

/********************************************************************************/

/*virtual*/ void ESBarChartQuickItem::paint(QPainter* pPainter) /*override*/
{
	mGeometryHasChanged = mPreviousSize != size();
	mPreviousSize = size();

	updateInternal();

	if (mValid)
	{
		pPainter->setPen(Qt::NoPen);
		pPainter->setBrush(Qt::lightGray);
		pPainter->setRenderHint(QPainter::Antialiasing);

		pPainter->save();
		pPainter->setClipRect(mMargin + mYAxisWidth, mMargin, width() - mMargin * 2.0 - mYAxisWidth, height() - mMargin * 2.0 - mXAxisHeight);
		pPainter->translate(mXOffset, 0);

		// Bars
		float lLeft = mMargin + mYAxisWidth;
		float lTextHeight = pPainter->boundingRect(QRectF(), "CAT").height();
		float lMaxBarHeight = mBarHeightFactor * mMaxValue;
		float lActualBarWith = std::max(0.5f, mBarWidth * mXScale);
		float lActualCategoriesTextHeight = lTextHeight * 0.7f;
		bool lCanDisplayAllCategories = lActualCategoriesTextHeight + mCategorySpacing < lActualBarWith;
		int lCategoriesToDisplayInterval = lCanDisplayAllCategories ? 1 : int(ceilf((lActualCategoriesTextHeight + mCategorySpacing) / lActualBarWith));
		for (int i = 0 ; i < mValues.size() ; ++i)
		{
			int lValue = mValues[i];
			float lBarHeight = std::min(lMaxBarHeight, lValue * mBarHeightFactor * mYScale);
			QRectF lBarRect(lLeft, height() - mMargin - lBarHeight - mRealXAxisHeight, lActualBarWith, lBarHeight);
			if(pPainter->clipBoundingRect().intersects(lBarRect))
			{
				pPainter->drawRect(lBarRect);

				// Category
				const QString& lCat = mCategories[i];
				if(lCanDisplayAllCategories || !mAllCategoriesOnly)
				{
					if(i % lCategoriesToDisplayInterval == 0)
					{
						pPainter->save();
						pPainter->setPen(Qt::black);
						pPainter->translate(lLeft + mBarWidth * mXScale / 2.f - lTextHeight / 2.f + 3, height() - mRealXAxisHeight - mMargin + cMarkerTextSpacing);
						pPainter->rotate(90);
						pPainter->drawText(0,0, lCat);
						pPainter->restore();
					}
				}
			}

			lLeft += mBarWidth * mXScale + std::min(mBarSpacing, mActualBarSpacing * mXScale);
		}

		pPainter->restore();

		// Axis
		pPainter->setPen(Qt::black);
		pPainter->setBrush(Qt::NoBrush);

		// Y
		float lScaledMaxValue = float(mMaxValue) / mYScale;

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
		float lMarkerSpacing = (height() - mMargin * 2.f - mRealXAxisHeight) / lScaledMaxValue;
		pPainter->drawLine(QLineF(mYAxisWidth + mMargin, mMargin, mYAxisWidth + mMargin, height() - mRealXAxisHeight - mMargin));

		pPainter->save();
		QPen lPen(Qt::darkGray);
		lPen.setStyle(Qt::DashLine);
		lPen.setWidthF(1.0f);
		pPainter->setPen(lPen);

		for (int i = 1; i < lNbMarker; ++i)
		{
			int lMarkerValue = lInterval * i;
			float lMarkerY = height() - mRealXAxisHeight - mMargin - lMarkerSpacing * lMarkerValue;
			pPainter->drawLine(QLineF(mYAxisWidth + mMargin - cMarkerHalfWidth, lMarkerY, width() - mYAxisWidth - mMargin*2.f, lMarkerY));
			QString lMarkerText = QString::number(lMarkerValue);
			QRectF lTextBoundingRect = pPainter->boundingRect(QRectF(), Qt::AlignVCenter, lMarkerText);
			pPainter->drawText(mYAxisWidth + mMargin - lTextBoundingRect.width() - cMarkerTextSpacing, lMarkerY - lTextBoundingRect.y() - 3, lMarkerText); // why 3 no idea
		}
		pPainter->restore();

		// X
		pPainter->drawLine(QLineF(mYAxisWidth + mMargin, height() - mRealXAxisHeight - mMargin, width() - mMargin, height() - mRealXAxisHeight - mMargin));
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

		if (mXAxisHeightAuto)
		{
			QPixmap lPixmap(10,10);
			QPainter lPainter(&lPixmap);
			setupPainterText(lPainter);
			mRealXAxisHeight = 0;
			for (const QString& cat : mCategories)
				mRealXAxisHeight = std::max(mRealXAxisHeight, lPainter.boundingRect(QRectF(), cat).width());

			mRealXAxisHeight += mXAxisHeight + mMargin + cMarkerTextSpacing;
		}
		else
		{
			mRealXAxisHeight = mXAxisHeight;
		}
	}
	if (mGeometryHasChanged || mDataHasChanged)
	{
		double lContentWidth = width() - mYAxisWidth - 2.f * mMargin;
		float lContentHeight = height() - mRealXAxisHeight - 2.f * mMargin;
		float lBarSpacingSum = mBarSpacing * (mCategories.size() - 1);
		if (lBarSpacingSum >= 0.5*lContentWidth) // Can't have at least 1px wide bars
		{
			mActualBarSpacing = 0.5 * lContentWidth / mCategories.size();
			lBarSpacingSum = mActualBarSpacing * (mCategories.size() - 1);
		}
		else
		{
			mActualBarSpacing = mBarSpacing;
		}
		mBarWidth = (lContentWidth - lBarSpacingSum) / mCategories.size();
		mBarHeightFactor = lContentHeight / mMaxValue;
	}

	mDataHasChanged = false;
	mGeometryHasChanged = false;
}

/********************************************************************************/

void ESBarChartQuickItem::setupPainterText(QPainter& pPainter)
{
	pPainter.setPen(Qt::black);
}