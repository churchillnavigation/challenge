#pragma once

#include "point_search.h"

namespace cc
{

	struct SearchContextBase
	{
		virtual ~SearchContextBase() {}
		virtual int Search(float searchLx, float searchLy, float searchHx, float searchHy, const int32_t count, Point* out_points) const = 0;
	};

	struct HeapItem
	{
		unsigned begin;
		int32_t rank;
	};

	struct SearchContext : SearchContextBase
	{

		// search tree structure
		unsigned levels;
		std::vector<Point> pts;
		
		int * ptsParamsMemBase;
		int * ptsParams;
		unsigned * ptsRangeEnds;
		int32_t * ptsRanks;
		std::vector<int> gridParams;
		std::vector<unsigned> gridPoints;
		std::vector<unsigned> gridChildren;

		static unsigned * ProcessTopGrid(unsigned * childrenToCheckEnd, const int * searchWindow, const int * gridParams, const unsigned * gridChildren);
		static unsigned * ProcessIntermediateGrid(unsigned * childrenToCheckBegin, unsigned * childrenToCheckEnd, const int * searchWindow, const int * gridParams, const unsigned * gridChildren);
		static HeapItem * ProcessLowestGrid(const unsigned * childrenToCheckBegin, const unsigned * childrenToCheckEnd, HeapItem * heap, const int * searchWindow, const int * gridParams, const unsigned * gridPoints);

		// search results
		unsigned * childrenToCheck;
		HeapItem * heapMemBase;
		HeapItem * heap;
		static void FixHeap(HeapItem * begin, HeapItem *& end, const int * ptsParams, const int * searchWindow, const int32_t * ptsRanks, const unsigned * ptsRangeEnds);
		static void BuildHeap(HeapItem * begin, HeapItem * end);
		static void RecursiveFixHeapItem(unsigned idx, HeapItem * begin, HeapItem * end);
		static void FixEmptyRoot(HeapItem * begin, HeapItem *& end);
		static const Point * PopHeapRoot(HeapItem * begin, HeapItem *& end, const int * ptsParams, const int * searchWindow, const int32_t * ptsRanks, const Point * pts, bool last, const unsigned * ptsRangeEnds);
		static int ProcessHeap(HeapItem * begin, HeapItem * end, const int * ptsParams, const int * searchWindow, const int32_t * ptsRanks, const Point * pts, int count, Point * out_points, const unsigned * ptsRangeEnds);
		static bool WindHeapItem(HeapItem & hi, const int * ptsParams, const int * searchWindow, const int32_t * ptsRanks, const unsigned * ptsRangeEnds);
		static int MapFloatToInt(float value);

		SearchContext(const Point * begin, const Point * end);
		~SearchContext() override;

		int Search(float searchLx, float searchLy, float searchHx, float searchHy, const int32_t count, Point* out_points) const override;

		SearchContext(const SearchContext & other) = delete;
		SearchContext & operator=(const SearchContext & other) = delete;
	};

}

