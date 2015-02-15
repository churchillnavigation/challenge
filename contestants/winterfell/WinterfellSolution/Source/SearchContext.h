#pragma once

#include "KdTree.h"

class MemoryPool;

struct SearchContext
{
	SearchContext(const Point* points_begin, const Point* points_end);
	~SearchContext();

	int Search(const Rect& rect, const int32_t count, Point* out_points);

	KdTree<Axis_X>*		mTree;
	MemoryPool*			mKDTreeMemPool;
	MemoryPool*			mIteratorMemPool;
	std::vector<Point>	mPoints;
};
