#ifndef POINT_SEARCH_H
#include "point_search.h"
#endif
#include <vector>
#include <thread>

const short DEFAULT_WANTED_POINTS_COUNT = 20;


enum PointCoordinate
{
	x,
	y
};



struct TreeNode
{
	Rect area;
	TreeNode* beforeSeparatorNode;
	TreeNode* afterSeparatorNode;
	std::vector<Point*> topPoints;
};



struct TreeRoot
{
	TreeNode* node;
	PointCoordinate startingTreeCoord;
};



struct SearchContext
{
	std::thread otherThread;
	TreeRoot* tree;
	Rect pointArea;
	std::vector<Point> orderedPoints;
};



TreeRoot* BuildTree(std::vector<Point> &points, Rect pointArea);

int SearchTree(SearchContext* sc, const Rect rect, const int32_t count, Point* out_points);

