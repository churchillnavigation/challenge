#include "stdafx.h"
#include <iterator>
#include <vector>
#include <algorithm>
#include <array>


int terminalNodeMaxPoints = DEFAULT_WANTED_POINTS_COUNT *5;
int wantedPointsCount = DEFAULT_WANTED_POINTS_COUNT;


bool CoordsInsideRectangle(const Rect &rect, float x, float y)
{
	if (rect.lx <= x && rect.hx >= x &&
		rect.ly <= y && rect.hy >= y)
		return true;
	return false;
}



bool PointInsideRectangle(const Rect &rect, const Point &point)
{
	if (rect.lx <= x && rect.hx >= x &&
		rect.ly <= y && rect.hy >= y)
		return true;
	return false;
}



bool RectangleGoesThroughAnother(const Rect &rect1, const Rect &rect2)
{
	return rect1.lx < rect2.lx && rect1.hx > rect2.hx &&
		rect2.ly < rect1.ly && rect2.hy > rect1.hy ||
		rect2.lx < rect1.lx && rect2.hx > rect1.hx &&
		rect1.ly < rect2.ly && rect1.hy > rect2.hy;
}



bool RectanglesIntersect(const Rect &rect1, const Rect &rect2)
{
	return CoordsInsideRectangle(rect1, rect2.lx, rect2.ly) ||
		CoordsInsideRectangle(rect1, rect2.hx, rect2.hy) ||
		CoordsInsideRectangle(rect1, rect2.hx, rect2.ly) ||
		CoordsInsideRectangle(rect1, rect2.lx, rect2.hy) ||
		CoordsInsideRectangle(rect2, rect1.lx, rect1.ly) ||
		CoordsInsideRectangle(rect2, rect1.hx, rect1.hy) ||
		CoordsInsideRectangle(rect2, rect1.hx, rect1.ly) ||
		CoordsInsideRectangle(rect2, rect1.lx, rect1.hy) ||
		RectangleGoesThroughAnother(rect1, rect2);
}



bool Rectangle1ContainsRectangle2(const Rect &rect1, const Rect &rect2)
{
	return rect1.lx <= rect2.lx && rect1.hx >= rect2.hx &&
		rect1.ly <= rect2.ly && rect1.hy >= rect2.hy;
}




bool PointABetterRank(const Point* a, const Point* b)
{
	return a->rank < b->rank;
}




bool SortByRankPredicate(const Point* a, const Point* b)
{
	return a->rank < b->rank;
}




float GetCenterOfMass(std::vector<Point*> &points, PointCoordinate coord)
{	
	float totalSum, totalRank;
	totalSum = totalRank = 0;

	if (coord == PointCoordinate::x)
		for (std::vector<Point*>::iterator it = points.begin(); it != points.end(); it = next(it))
		{
			totalRank += (*it)->rank;
			totalSum += (*it)->x * (*it)->rank;
		}
	else
		for (std::vector<Point*>::iterator it = points.begin(); it != points.end(); it = next(it))
		{
			totalRank += (*it)->rank;
			totalSum += (*it)->y * (*it)->rank;
		}
	
	return (totalSum / totalRank);
}



float GetSeparator(std::vector<Point*> &points, PointCoordinate coord)
{
	return GetCenterOfMass(points, coord);
}



void ClearVector(std::vector<Point*> &vector)
{
	vector.clear();
	vector.swap(std::vector<Point*>(vector));
}




TreeNode* BuildTreeNode(std::vector<Point*> &points, PointCoordinate coord, Rect currentArea, bool isSingleChild)
{
	bool isTerminalNode = false;

	TreeNode* node = new TreeNode;
	node->area = currentArea;
	node->beforeSeparatorNode = nullptr;
	node->afterSeparatorNode = nullptr;

	//if it's not a terminal node
	if (points.size() > terminalNodeMaxPoints)
	{
		long initialVectorAllocation = std::lround(points.size() * 0.33);

		std::vector<Point*> beforeSeparatorPoints;
		beforeSeparatorPoints.reserve(initialVectorAllocation);
		std::vector<Point*> afterSeparatorPoints;
		afterSeparatorPoints.reserve(initialVectorAllocation);

		float separator = GetCenterOfMass(points, coord);

		if (coord == PointCoordinate::x)
			for (int i = 0; i < points.size(); i++)
				if (points[i]->x < separator)
					beforeSeparatorPoints.push_back(points[i]);
				else
					afterSeparatorPoints.push_back(points[i]);
		else
			for (int i = 0; i < points.size(); i++)
				if (points[i]->y < separator)
					beforeSeparatorPoints.push_back(points[i]);
				else
					afterSeparatorPoints.push_back(points[i]);

		PointCoordinate newCoord = coord == PointCoordinate::x ? y : x;


		if (beforeSeparatorPoints.size() > 0 && afterSeparatorPoints.size() > 0)
		{
			Rect beforeRect, afterRect;

			beforeRect = afterRect = currentArea;
			if (coord == PointCoordinate::x)
			{
				beforeRect.hx = separator;
				afterRect.lx = separator;
			}
			else
			{
				beforeRect.hy = separator;
				afterRect.ly = separator;
			}

			node->beforeSeparatorNode = BuildTreeNode(beforeSeparatorPoints, newCoord, beforeRect, false);
			node->afterSeparatorNode = BuildTreeNode(afterSeparatorPoints, newCoord, afterRect, false);
		}
		else 
			isTerminalNode = true;
	}
	else
		isTerminalNode = true;
	


	std::sort(points.begin(), points.end(), SortByRankPredicate);

	size_t totalNodes = DEFAULT_WANTED_POINTS_COUNT;
	if (isTerminalNode)
		totalNodes = points.size();

	node->topPoints.reserve(totalNodes);

	for (int i = 0; i < totalNodes; i++)
		node->topPoints.push_back(points[i]);

	return node;
}



TreeRoot* BuildTree(std::vector<Point> &points, Rect pointArea)
{
	TreeRoot* root = new TreeRoot;

	if (pointArea.hx - pointArea.lx > pointArea.hy - pointArea.ly)
		root->startingTreeCoord = PointCoordinate::x;
	else
		root->startingTreeCoord = PointCoordinate::y;
	
	//Create Point* vector
	std::vector<Point*> pointsVector(points.size());

	for (int i = 0; i < points.size(); i++)
		pointsVector[i] = &points[i];

	root->node = BuildTreeNode(pointsVector, root->startingTreeCoord, pointArea, false);
	
	return root;
}



void SortLastPosition(std::vector<Point*> &orderedTopPoints)
{
	Point* tempPoint;
	for (size_t i = orderedTopPoints.size() - 1; i > 0; i--)
		if (orderedTopPoints[i]->rank < orderedTopPoints[i - 1]->rank)
		{
			tempPoint = orderedTopPoints[i - 1];
			orderedTopPoints[i - 1] = orderedTopPoints[i];
			orderedTopPoints[i] = tempPoint;
		}
		else
			return;
}


bool TryCopyRankedPoint(std::vector<Point*> &orderedTopPoints, Point &point)
{

	if (orderedTopPoints.size() < wantedPointsCount)
		orderedTopPoints.push_back(&point);
	else 
		if (orderedTopPoints[wantedPointsCount - 1]->rank > point.rank)
			orderedTopPoints[wantedPointsCount - 1] = &point;
		else
			return false;

	SortLastPosition(orderedTopPoints);

	return true;
}


void CopyBetterRankedPoints(std::vector<Point*> &orderedTopPoints, std::vector<Point*> &newOrderedPoints)
{
	for (int i = 0 ; i < newOrderedPoints.size() ; i++)
		if (!TryCopyRankedPoint(orderedTopPoints, *newOrderedPoints[i]))
			return;
}



void CopyBetterRankedPoints(std::vector<Point*> &orderedTopPoints, std::vector<Point*> &newOrderedPoints, Rect &searchRectangle)
{
	for (int i = 0; i < newOrderedPoints.size(); i++)
		if (PointInsideRectangle(searchRectangle, *newOrderedPoints[i]))
		{
			if (!TryCopyRankedPoint(orderedTopPoints, *newOrderedPoints[i]))
				return;
		}
}




bool IsTerminalNode(TreeNode* node)
{
	return node->beforeSeparatorNode == nullptr && node->afterSeparatorNode == nullptr;
}



std::vector<Point*> GetBestPoints(SearchContext* sc, TreeNode &node, Rect searchRect)
{
	std::vector<TreeNode*> searchNodes;
	std::vector<Point*> rankedBestPoints;
	rankedBestPoints.reserve(wantedPointsCount);

	searchNodes.push_back(node.beforeSeparatorNode);
	searchNodes.push_back(node.afterSeparatorNode);


	for (int i = 0; i < searchNodes.size(); i++)
	{

		//point area is inside search rectangle
		if (Rectangle1ContainsRectangle2(searchRect, searchNodes[i]->area))
			CopyBetterRankedPoints(rankedBestPoints, searchNodes[i]->topPoints);
		else //rectangles intersect
			if (RectanglesIntersect(searchNodes[i]->area, searchRect))
			{
				//terminal node, may have points outside search rectangle
				if (IsTerminalNode(searchNodes[i]))
					CopyBetterRankedPoints(rankedBestPoints, searchNodes[i]->topPoints, searchRect);
				else//check if the topPoints[0]-> rank is better than worst bestPoints
					if (rankedBestPoints.size() < wantedPointsCount ||
						PointABetterRank(searchNodes[i]->topPoints[0], rankedBestPoints[rankedBestPoints.size() - 1]))
					{

						if (searchNodes[i]->beforeSeparatorNode != nullptr)
							searchNodes.push_back(searchNodes[i]->beforeSeparatorNode);

						if (searchNodes[i]->afterSeparatorNode != nullptr)
							searchNodes.push_back(searchNodes[i]->afterSeparatorNode);
					}
			}
	}

	return rankedBestPoints;
}


int SearchTree(SearchContext* sc, const Rect rect, const int32_t count, Point* out_points)
{
	wantedPointsCount = count;

	if (sc == nullptr)
		return 0;

	std::vector<Point*> bestPoints = GetBestPoints(sc, *sc->tree->node, rect);
	for (int i = 0; i < bestPoints.size(); i++)
		out_points[i] = *bestPoints[i];


	return bestPoints.size();
}
