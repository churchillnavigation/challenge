#pragma once

#include "KdTree.h"
#include "DebugStats.h"

class MemoryPool;
class SearchIterator;

struct SearchContext
{
	SearchContext(const Point* points_begin, const Point* points_end);
	~SearchContext();

	int Search(const Rect& rect, const int32_t count, Point* out_points);

	KdTree<Axis_X>*		mTree;
	MemoryPool*			mKDTreeMemPool;
	std::vector<Point>	mPoints;

	SearchIterator*		mIteratorMemPool;

#ifdef _ENABLE_STATS_LOGGING
	StatsAllQueries		mStats;
	std::vector<Rect>	mQueryRects;
#endif // _ENABLE_STATS_LOGGING

};
