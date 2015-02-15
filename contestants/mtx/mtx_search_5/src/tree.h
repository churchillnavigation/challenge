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
		unsigned end;
		unsigned placeholder;
	};

	struct SearchContext : SearchContextBase
	{

		// search tree structure
		unsigned levels;
		std::vector<Point> pts;
		
		int * ptsParamsMemBase;
		int * ptsParams;
		int32_t * ptsRanks;
		int * gridParamsMemBase;
		int * gridParams;
		std::vector<unsigned> gridPoints;
		std::vector<unsigned> gridChildren;

		static unsigned * ProcessTopGrid(unsigned * childrenToCheckEnd, const int * searchWindow, const int * gridParams, const unsigned * gridChildren);
		static unsigned * ProcessIntermediateGrid(unsigned * childrenToCheckBegin, unsigned * childrenToCheckEnd, const int * searchWindow, const int * gridParams, const unsigned * gridChildren);
		static HeapItem * ProcessLowestGrid(const unsigned * childrenToCheckBegin, const unsigned * childrenToCheckEnd, HeapItem * heap, const int * searchWindow, const int * gridParams, const unsigned * gridPoints);

		// search results
		unsigned * childrenToCheck;
		HeapItem * heapMemBase;
		HeapItem * heap;
		static void FixHeap(HeapItem * begin, HeapItem *& end, const int * ptsParams, const int * searchWindow, const int32_t * ptsRanks);
		static void BuildHeap(HeapItem * begin, HeapItem * end);
		static void RecursiveFixHeapItem(unsigned idx, HeapItem * begin, HeapItem * end);
		static void FixEmptyRoot(HeapItem * begin, HeapItem *& end);
		static const Point * PopHeapRoot(HeapItem * begin, HeapItem *& end, const int * ptsParams, const int * searchWindow, const int32_t * ptsRanks, const Point * pts, bool last);
		static int ProcessHeap(HeapItem * begin, HeapItem * end, const int * ptsParams, const int * searchWindow, const int32_t * ptsRanks, const Point * pts, int count, Point * out_points);
		static bool WindHeapItem(HeapItem & hi, const int * ptsParams, const int * searchWindow, const int32_t * ptsRanks);
		static int FlatWind(HeapItem & hi, const int * ptsParams, const int * sw, const int32_t * ptsRanks, const Point * pts, int count, Point * out_points);
		static int MapFloatToInt(float value);

		SearchContext(const Point * begin, const Point * end);
		~SearchContext() override;

		int Search(float searchLx, float searchLy, float searchHx, float searchHy, const int32_t count, Point* out_points) const override;

		SearchContext(const SearchContext & other) = delete;
		SearchContext & operator=(const SearchContext & other) = delete;
	};

}

