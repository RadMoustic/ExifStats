#include "ESQuadTree.h"

/********************************************************************************/

ESQuadTree::ESQuadTree(const QRectF& pRootRect, const QVector<QPointF>& pPoints)
	: mRootNode(pRootRect, pPoints)
{

}

/********************************************************************************/

QVector<QVector3D> ESQuadTree::getPoints(int pDepth, const QRectF& pRect)
{
	QVector<QVector3D> lResult;

	mRootNode.getPoints(lResult, pDepth, pRect);

	return lResult;
}

/********************************************************************************/

ESQuadTree::Node::Node(QRectF pRect, const QVector<QPointF>& pPoints)
	: mRect(pRect)
	, mTotalPoints(0)
{
	QVector<QPointF> lNodePoints;
	for (const QPointF& aPoint : pPoints)
	{
		if (mRect.contains(aPoint))
		{
			lNodePoints.append(aPoint);
		}
	}

	mTotalPoints = lNodePoints.count();

	if (lNodePoints.count() > 1)
	{
		QSizeF lChildSize = mRect.size() / 2.f;

		mTopLeft = std::make_unique<Node>(QRectF(mRect.topLeft(), lChildSize), lNodePoints);
		mTopRight = std::make_unique<Node>(QRectF(mRect.topLeft() + QPointF(lChildSize.width(), 0), lChildSize), lNodePoints);
		mBottomLeft = std::make_unique<Node>(QRectF(mRect.topLeft() + QPointF(0, lChildSize.height()), lChildSize), lNodePoints);
		mBottomRight = std::make_unique<Node>(QRectF(mRect.topLeft() + QPointF(lChildSize.width(), lChildSize.height()), lChildSize), lNodePoints);
	}
	else if (lNodePoints.count() == 1)
	{
		mRect = QRectF(lNodePoints[0], QSizeF(0, 0));
	}
	else
	{
		mRect = QRectF(0, 0, -1.f, -1.f);
	}
}

/********************************************************************************/

const void ESQuadTree::Node::getPoints(QVector<QVector3D>& pPoints, int pDepth, const QRectF& pRect)
{
	if (mRect.isNull())
	{
		QPointF lCenter = mRect.center();
		pPoints.append(QVector3D(lCenter.x(), lCenter.y(), 1));
	}
	else if (mRect.isValid() && pRect.intersects(mRect))
	{
		if (pDepth > 0)
		{
			mTopLeft->getPoints(pPoints, pDepth - 1, pRect);
			mTopRight->getPoints(pPoints, pDepth - 1, pRect);
			mBottomLeft->getPoints(pPoints, pDepth - 1, pRect);
			mBottomRight->getPoints(pPoints, pDepth - 1, pRect);
		}
		else
		{
			QPointF lCenter = mRect.center();
			pPoints.append(QVector3D(lCenter.x(), lCenter.y(), mTotalPoints));
		}
	}
}
