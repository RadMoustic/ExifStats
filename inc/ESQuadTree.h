#pragma once

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

// Qt
#include <QRectF>
#include <QVector3D>
#include <QVector>

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

class ESQuadTree
{
public:
	/********************************* METHODS ***********************************/

	ESQuadTree(const QRectF& pRootRect, const QVector<QPointF>& pPoints);

	QVector<QVector3D> getPoints(int pDepth, const QRectF& pRect);

private:
	/********************************** TYPES *************************************/

	struct Node
	{
		Node(QRectF pRect, const QVector<QPointF>& pPoints);

		const void getPoints(QVector<QVector3D>& pPoints, int pDepth, const QRectF& pRect);

		QRectF mRect;

		std::unique_ptr<Node> mTopLeft;
		std::unique_ptr<Node> mTopRight;
		std::unique_ptr<Node> mBottomLeft;
		std::unique_ptr<Node> mBottomRight;

		uint mTotalPoints;
	};

	/******************************** ATTRIBUTES **********************************/
	
	Node mRootNode;
};
