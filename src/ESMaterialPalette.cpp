#include "ESMaterialPalette.h"

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

/*static*/ std::unordered_map<QObject*, std::shared_ptr<ESMaterialPalette::PaletteData>> ESMaterialPalette::mCache;

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

QColor ESMaterialPalette::PaletteData::getOffsetColor(QColor pColor, int pOffset) const
{
	QColor lColor;
	if (mDarkTheme)
		lColor = pColor.darker(pOffset);
	else
	{
		int lVeryDarkOffset = (pOffset-100) * 0.5f;
		lColor = pColor.lightness() > 50 ? pColor.lighter(pOffset) : QColor(pColor.red() + lVeryDarkOffset, pColor.green() + lVeryDarkOffset, pColor.blue() + lVeryDarkOffset, pColor.alpha());
	}

	return lColor;
}

/********************************************************************************/

void ESMaterialPalette::PaletteData::drawForegroundRect(QPainter* pPainter, const QRectF& pRect, int pColorOffset) const
{
	pPainter->save();
	pPainter->setBrush(getOffsetColor(mForeground, pColorOffset));
	pPainter->drawRect(pRect);
	pPainter->restore();
}

/********************************************************************************/

/*static*/ void ESMaterialPalette::clearCache()
{
	mCache.clear();
}

/********************************************************************************/

/*static*/ std::shared_ptr<ESMaterialPalette::PaletteData> ESMaterialPalette::getPalette(QObject* pItem)
{
	std::shared_ptr<ESMaterialPalette::PaletteData>& lResult = mCache[pItem];

	if (!lResult)
	{
		lResult = std::make_shared<PaletteData>();
		QQmlContext* lContext = qmlContext(pItem);

		if (lContext)
		{
			auto lEval = [&](const QString& pProp, QVariant& pOut)
				{
					QQmlExpression lExpr(lContext, pItem, pProp);
					QVariant lResult = lExpr.evaluate();
					if (!lExpr.hasError() && lResult.isValid())
						pOut = lResult;
				};

			QVariant lTheme;
			QVariant lBg;
			QVariant lFg;
			QVariant lPri;
			QVariant lAcc;
			QVariant lEle;

			lEval("Material.theme", lTheme);
			lEval("Material.background", lBg);
			lEval("Material.foreground", lFg);
			lEval("Material.primary", lPri);
			lEval("Material.accent", lAcc);
			lEval("Material.elevation", lEle);

			lResult->mDarkTheme = lTheme.toInt() == 1;
			if (lBg.isValid())
				lResult->mBackground = lBg.value<QColor>();
			if (lFg.isValid())
				lResult->mForeground = lFg.value<QColor>();
			if (lPri.isValid())
				lResult->mPrimary = lPri.value<QColor>();
			if (lAcc.isValid())
				lResult->mAccent = lAcc.value<QColor>();
			if (lEle.isValid())
				lResult->mElevation = lEle.toReal();
		}
	}

	return lResult;
}