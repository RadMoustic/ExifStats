#pragma once

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

// Qt
#include <QPainter>
#include <QQmlExpression>
#include <QColor>
#include <QQuickItem>

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/


/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

struct ESMaterialPalette
{
	struct PaletteData
	{
		bool mDarkTheme = false;
		QColor mBackground = QColor(Qt::white);
		QColor mForeground = QColor(Qt::black);
		QColor mPrimary = QColor(Qt::blue);
		QColor mAccent = QColor(Qt::cyan);
		qreal mElevation = 0.0;

		QColor getOffsetColor(QColor pColor, int pOffset) const;
		void drawForegroundRect(QPainter* pPainter, const QRectF& pRect, int pColorOffset = 0) const;
	};

	static std::shared_ptr<PaletteData> getPalette(QObject* pItem);
	static void clearCache();

private:
	static std::unordered_map<QObject*, std::shared_ptr<PaletteData>> msCache;
};
