#include "ESQuadTree.h"

/********************************************************************************/

ESQuadTree::ESQuadTree(const QRectF& pRootRect, const QVector<QPointF>& pPoints)
	: mRootNode(pRootRect, pPoints)
{

}

/********************************************************************************/

void ESQuadTree::clear()
{
	mRootNode.clear();
}

/********************************************************************************/

void ESQuadTree::fill(const QRectF& pRootRect, const QVector<QPointF>& pPoints)
{
	mRootNode.fill(pRootRect, pPoints);
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
	: mTotalPoints(0)
{
	fill(pRect, pPoints);
}

/********************************************************************************/

void ESQuadTree::Node::clear()
{
	mTotalPoints = 0;
	mRect = QRectF(0, 0, -1.f, -1.f);
}

/********************************************************************************/

void ESQuadTree::Node::fill(QRectF pRect, const QVector<QPointF>& pPoints)
{
	clear();
	mRect = pRect;

	mPoints.clear();
	for (const QPointF& lPoint : pPoints)
	{
		if (mRect.contains(lPoint))
		{
			mPoints.append(lPoint);
		}
	}

	mTotalPoints = mPoints.count();

	if (mPoints.count() > 1)
	{
		QSizeF lChildSize = mRect.size() / 2.f;

		if (mTopLeft)
		{
			mTopLeft->fill(QRectF(mRect.topLeft(), lChildSize), mPoints);
			mTopRight->fill(QRectF(mRect.topLeft() + QPointF(lChildSize.width(), 0), lChildSize), mPoints);
			mBottomLeft->fill(QRectF(mRect.topLeft() + QPointF(0, lChildSize.height()), lChildSize), mPoints);
			mBottomRight->fill(QRectF(mRect.topLeft() + QPointF(lChildSize.width(), lChildSize.height()), lChildSize), mPoints);
		}
		else
		{
			mTopLeft = std::make_unique<Node>(QRectF(mRect.topLeft(), lChildSize), mPoints);
			mTopRight = std::make_unique<Node>(QRectF(mRect.topLeft() + QPointF(lChildSize.width(), 0), lChildSize), mPoints);
			mBottomLeft = std::make_unique<Node>(QRectF(mRect.topLeft() + QPointF(0, lChildSize.height()), lChildSize), mPoints);
			mBottomRight = std::make_unique<Node>(QRectF(mRect.topLeft() + QPointF(lChildSize.width(), lChildSize.height()), lChildSize), mPoints);
		}
	}
	else if (mPoints.count() == 1)
	{
		mRect = QRectF(mPoints[0], QSizeF(0, 0));
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
