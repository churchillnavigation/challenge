#pragma once

#include "point_search.h"

namespace cc
{

	struct HeapItem
	{
		unsigned begin;
		int32_t rank;
	};

	struct SearchContext
	{

		// search tree structure
		unsigned levels;
		std::vector<Point> pts;
		float * ptsParamsMemBase;
		float * ptsParams;
		int32_t * ptsRanks;
		std::vector<float> gridParams;
		std::vector<unsigned> gridPoints;
		std::vector<unsigned> gridChildren;

		void ProcessTopGrid(unsigned * out);
		void ProcessIntermediateGrid(const unsigned * in, unsigned * out);
		HeapItem * ProcessLowestGrid(const unsigned * in, HeapItem * heap);

		// search parameters
		__declspec(align(16)) float searchWindow[4];

		// search results
		std::vector<unsigned *> childrenToCheck;
		HeapItem * heapMemBase;
		HeapItem * heap;
		void FixHeap(HeapItem * begin, HeapItem *& end);
		void BuildHeap(HeapItem * begin, HeapItem * end);
		void RecursiveFixHeapItem(unsigned idx, HeapItem * begin, HeapItem * end);
		void FixEmptyRoot(HeapItem * begin, HeapItem *& end);
		const Point * PopHeapRoot(HeapItem * begin, HeapItem *& end);
		bool WindHeapItem(HeapItem & hi);

		SearchContext(const Point * begin, const Point * end, unsigned width, unsigned height, unsigned level);
		~SearchContext();

		int Search(float searchLx, float searchLy, float searchHx, float searchHy, const int32_t count, Point* out_points);

		SearchContext(const SearchContext & other) = delete;
		SearchContext & operator=(const SearchContext & other) = delete;
	};

	struct CompoundSearchContext
	{
		std::list<SearchContext> ctxs;
		CompoundSearchContext(std::vector<Point> & in_pts);
		~CompoundSearchContext();
		int Search(float searchLx, float searchLy, float searchHx, float searchHy, const int32_t count, Point* out_points);


		CompoundSearchContext(const CompoundSearchContext & other) = delete;
		CompoundSearchContext & operator=(const CompoundSearchContext & other) = delete;

		// stat
		std::map<int, int> iterCnt;
	};

}

