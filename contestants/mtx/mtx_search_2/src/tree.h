#pragma once

#include "point_search.h"

inline void* alignedMalloc(size_t n);
inline void alignedFree(void* p);

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
		unsigned cellCount;
		std::vector<Point> pts;
		float * ptsParamsMemBase;
		float * ptsParams;
		int32_t * ptsRanks;
		std::vector<float> gridParams;
		std::vector<unsigned> gridPoints;

		unsigned ProcessGrid();

		// search parameters
		__declspec(align(16)) float searchWindow[4];

		// search results
		HeapItem * heapMemBase;
		HeapItem * heap;
		unsigned FixHeap(unsigned heapHead);
		void BuildHeap(unsigned heapHead);
		void RecursiveFixHeapItem(unsigned idx, unsigned heapHead);
		void FixEmptyRoot(unsigned & heapHead);
		const Point * PopHeapRoot(unsigned & heapHead);
		bool WindHeapItem(HeapItem & hi);

		SearchContext(std::vector<Point> && in_pts);
		~SearchContext();

		int Search(float searchLx, float searchLy, float searchHx, float searchHy, const int32_t count, Point* out_points);

		SearchContext(const SearchContext & other) = delete;
		SearchContext & operator=(const SearchContext & other) = delete;
	};

}

