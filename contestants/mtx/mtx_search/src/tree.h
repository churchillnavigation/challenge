#pragma once

#include "point_search.h"

namespace cc
{
	static const int point_count_in_leaves = 4000; // 4000 eddigi legjobb
	//static const int point_count_in_leaves = 100000000;

	enum EdgeState
	{
		BELOW, INSIDE, ABOVE
	};

	struct Node
	{
		// begin and end index of data
		Point *ptBegin, *ptEnd;

		// bounding of the node
		float lx, ly, hx, hy;

		// information on the split
		enum
		{
			SPLIT_X,
			SPLIT_Y,
		} direction;

		// children
		size_t childNodeIdxs[2];
		size_t childLeafIdxs[2];
		Node* children[2];
		size_t leavesIdxFirst;
		size_t leavesIdxLast;

		Node() : ptBegin(nullptr), ptEnd(nullptr), direction(SPLIT_X), children()
		{
			childNodeIdxs[0] = childNodeIdxs[1] = -1;
			childLeafIdxs[0] = childLeafIdxs[1] = -1;
		}
	};

	struct SearchStateParams
	{
		Node * node;
		EdgeState lxe, lye, hxe, hye;
		SearchStateParams() : node(nullptr) {}
		SearchStateParams(Node * node, EdgeState lxe, EdgeState lye, EdgeState hxe, EdgeState hye)
			: node(node), lxe(lxe), lye(lye), hxe(hxe), hye(hye)
		{
		}
	};
	
	struct HeapItem
	{
		union
		{
			struct
			{
				const Point * begin;
				const Point * end;
				bool black;
			};
			char c[32];
		};
	};

	struct SearchContext
	{
		// search tree structure
		std::vector<Point> pts;
		std::vector<Node> nodes;
		std::vector<Node> leaves;
		bool oneLeafOnly;
		Node * root;

		size_t AllocateNodeIdx();
		Node * GetNode(size_t nodeIdx);
		size_t SetAsLeafIdx(size_t nodeIdx);
		Node * GetLeaf(size_t leafIdx);

		// multi threaded search tree processing
		std::vector<SearchStateParams> localSearchStateParams;

		void ProcessSearchState(const SearchStateParams & params, SearchContext & ctx, SearchStateParams * lssp, size_t & lsspHead, size_t & lsspTail);
		void SearchCycle();

		// search parameters
		float searchLx, searchLy, searchHx, searchHy;

		// search results
		std::vector<HeapItem> heap;
		size_t heapHead;
		void BuildHeap();
		void RecursiveFixHeapItem(size_t idx);
		void FixEmptyRoot();
		const Point * PopHeapRoot(bool & black);

		// statistics
		unsigned totalGreyLeaves;
		unsigned totalBlackLeaves;
		unsigned totalQueries;

		SearchContext(std::vector<Point> && in_pts);
		~SearchContext();

		int Search(float searchLx, float searchLy, float searchHx, float searchHy, const int32_t count, Point* out_points);

		SearchContext(const SearchContext & other) = delete;
		SearchContext & operator=(const SearchContext & other) = delete;
	};

}

