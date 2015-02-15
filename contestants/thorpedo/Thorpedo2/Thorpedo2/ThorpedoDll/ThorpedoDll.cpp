//Brian Thorpe 2015
#include "stdafx.h"
#include "ThorpedoDll.h"
#include "CommonStructs.h"
#include "QuadTree.h"

SearchContext* __stdcall create(const Point* points_begin, const Point* points_end)
{
	QuadTree* quadTree = QuadTree::ConstructFromPoints(points_begin, points_end);
	return (SearchContext*)quadTree;
}

int32_t  __stdcall search(SearchContext* o, const Rect rect, const int32_t count, Point* out_points)
{
	QuadTree* quadTree = (QuadTree*)o;
	size_t pts = quadTree->GetPoints(rect, count, out_points);
	return (int32_t)pts;
}

SearchContext* __stdcall destroy(SearchContext* o)
{
	try
	{
		QuadTree* quadTree = (QuadTree*)o;
		delete quadTree;
	}
	catch(std::exception& e)
	{
		std::cout << "Standard Exception: " << e.what() << std::endl;
		return o;
	}
	return nullptr;
}