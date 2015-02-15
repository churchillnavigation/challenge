#include "stdafx.h"

#include "tree.h"

namespace std
{
	template<>
	void swap<cc::HeapItem>(cc::HeapItem & a, cc::HeapItem & b)
	{
		std::swap(a.begin, b.begin);
		std::swap(a.end, b.end);
		std::swap(a.black, b.black);
	}

}


namespace cc
{

	template<typename T>
	static T sqr(T value)
	{
		return value*value;
	}

	struct datastat
	{
		float mean;
		float variance;
	};

	template<typename it_type, typename selector_type>
	datastat mean_variance(it_type begin, it_type end, selector_type selector)
	{
		const size_t count = end - begin;
		std::map<float, int> occurences;

		double sum = 0;

		for (auto i = begin; i != end; ++i)
		{
			sum += selector(i);
		}

		const float mean = sum / count;

		sum = 0;
		for (auto i = begin; i != end; ++i)
		{
			double diff = mean - selector(i);
			sum += sqr(diff);
		}

		const float variance = sum / count;
		const float stdev = std::sqrt(variance);

		return{ mean, variance };

	}

	static inline void UpdateParentToLeafReference(size_t nodeIdx, size_t parentIdx, SearchContext & ctx)
	{
		const size_t leafIdx = ctx.SetAsLeafIdx(nodeIdx);
		Node * leaf = ctx.GetLeaf(leafIdx);
		leaf->leavesIdxFirst = leafIdx;
		leaf->leavesIdxLast = leafIdx;
		if (parentIdx != -1)
		{
			Node * const parent = ctx.GetNode(parentIdx);
			if (parent->childNodeIdxs[0] == nodeIdx)
			{
				parent->childNodeIdxs[0] = -1;
				parent->childLeafIdxs[0] = leafIdx;
			}
			else
			{
				parent->childNodeIdxs[1] = -1;
				parent->childLeafIdxs[1] = leafIdx;
			}
		}
		else
		{
			ctx.oneLeafOnly = true;
		}
	}

	static void BuildNode(size_t nodeIdx, size_t parentIdx, float lx, float ly, float hx, float hy, SearchContext & ctx)
	{
		Node * node = ctx.GetNode(nodeIdx);

		node->lx = lx;
		node->ly = ly;
		node->hx = hx;
		node->hy = hy;

		auto begin = node->ptBegin;
		auto end = node->ptEnd;
		const size_t count = node->ptEnd - node->ptBegin;

		if (count <= point_count_in_leaves)
		{
			std::sort(begin, end, [](const Point & pt1, const Point & pt2)
			{
				return pt1.rank < pt2.rank;
			});
			UpdateParentToLeafReference(nodeIdx, parentIdx, ctx);
			return;
		}

		// calculate variance in x
		const auto mean_var_x = mean_variance(begin, end, [](const Point * pt) { return pt->x; });
		const auto mean_var_y = mean_variance(begin, end, [](const Point * pt) { return pt->y; });

		node->direction = mean_var_x.variance >= mean_var_y.variance ? Node::SPLIT_X : Node::SPLIT_Y;
		const float split = node->direction == Node::SPLIT_X ? mean_var_x.mean : mean_var_y.mean;

		auto sortPred = [&node](const Point & p1, const Point & p2)
		{
			return node->direction == Node::SPLIT_X ? p1.x < p2.x : p1.y < p2.y;
		};
		std::sort(begin, end, sortPred);

		if (node->direction == Node::SPLIT_X && begin->x == (end - 1)->x || node->direction == Node::SPLIT_Y && begin->y == (end - 1)->y)
		{
			std::sort(begin, end, [](const Point & pt1, const Point & pt2)
			{
				return pt1.rank < pt2.rank;
			});
			UpdateParentToLeafReference(nodeIdx, parentIdx, ctx);
			return;
		}

		auto searchPred = [&node](const Point & p1, float f)
		{
			return node->direction == Node::SPLIT_X ? p1.x < f : p1.y < f;
		};
		auto mean = std::lower_bound(begin, end, split, searchPred);

		auto valueChangePred = [&node](const Point & p1, const Point & p2)
		{
			return node->direction == Node::SPLIT_X ? p1.x != p2.x : p1.y != p2.y;
		};
		auto firstAllowed = std::adjacent_find(begin, end, valueChangePred);

		if (mean < firstAllowed+1)
		{
			mean = firstAllowed+1;
		}


		const size_t child1Idx = ctx.AllocateNodeIdx();
		node = ctx.GetNode(nodeIdx); // node ptr might have been invalidated
		node->childNodeIdxs[0] = child1Idx;
		Node * child = ctx.GetNode(child1Idx);
		child->ptBegin = begin;
		child->ptEnd = mean;
		if (node->direction == Node::SPLIT_X)
		{
			BuildNode(child1Idx, nodeIdx, lx, ly, std::nextafter((mean - 1)->x, std::numeric_limits<float>::infinity()), hy, ctx);
		}
		else
		{
			BuildNode(child1Idx, nodeIdx, lx, ly, hx, std::nextafter((mean - 1)->y, std::numeric_limits<float>::infinity()), ctx);
		}

		const size_t child2Idx = ctx.AllocateNodeIdx();
		node = ctx.GetNode(nodeIdx); // node ptr might have been invalidated
		node->childNodeIdxs[1] = child2Idx;
		child = ctx.GetNode(child2Idx);
		child->ptBegin = mean;
		child->ptEnd = end;
		if (node->direction == Node::SPLIT_X)
		{
			BuildNode(child2Idx, nodeIdx, mean->x, ly, hx, hy, ctx);
		}
		else
		{
			BuildNode(child2Idx, nodeIdx, lx, mean->y, hx, hy, ctx);
		}
	}

	static void PostProcessNode(Node * node, SearchContext & ctx)
	{
		if (node->childNodeIdxs[0] != -1)
		{
			node->children[0] = ctx.GetNode(node->childNodeIdxs[0]);
			PostProcessNode(node->children[0], ctx);
		}
		else
		{
			node->children[0] = ctx.GetLeaf(node->childLeafIdxs[0]);
		}

		if (node->childNodeIdxs[1] != -1)
		{
			node->children[1] = ctx.GetNode(node->childNodeIdxs[1]);
			PostProcessNode(node->children[1], ctx);
		}
		else
		{
			node->children[1] = ctx.GetLeaf(node->childLeafIdxs[1]);
		}

		node->leavesIdxFirst = node->children[0]->leavesIdxFirst;
		node->leavesIdxLast = node->children[1]->leavesIdxLast;

	}

	static inline size_t pow2roundup(size_t x)
	{
		--x;
		x |= x >> 1;
		x |= x >> 2;
		x |= x >> 4;
		x |= x >> 8;
		x |= x >> 16;
		x |= x >> 32;
		return x + 1;
	}

	SearchContext::SearchContext(std::vector<Point> && in_pts)
		: pts(std::move(in_pts)), oneLeafOnly(false), totalGreyLeaves(0), totalBlackLeaves(0), totalQueries(0)
	{
		const size_t nodeIdx = AllocateNodeIdx();
		// check if nodeIdx == 0;
		Node * n = GetNode(nodeIdx);

		if (pts.empty())
		{
			oneLeafOnly = true;
			const size_t leafIdx = SetAsLeafIdx(nodeIdx);
			// check if leafIdx == 0
			root = GetLeaf(0);
			root->lx = 0;
			root->ly = 0;
			root->hx = 0;
			root->hy = 0;
			root->leavesIdxFirst = 0;
			root->leavesIdxLast = 0;
			root->ptBegin = nullptr;
			root->ptEnd = nullptr;
		}
		else
		{
			n->ptBegin = &pts.front();
			n->ptEnd = &pts.back()+1;

			// calculate bounding
			n->lx = n->hx = pts[0].x;
			n->ly = n->hy = pts[0].y;

			for (const Point & p : pts)
			{
				if (p.x < n->lx)
					n->lx = p.x;
				if (p.x > n->hx)
					n->hx = p.x;
				if (p.y < n->ly)
					n->ly = p.y;
				if (p.y > n->hy)
					n->hy = p.y;
			}

			n->hx = std::nextafterf(n->hx, std::numeric_limits<float>::infinity());
			n->hy = std::nextafterf(n->hy, std::numeric_limits<float>::infinity());

			BuildNode(nodeIdx, -1, n->lx, n->ly, n->hx, n->hy, *this);

			if (oneLeafOnly)
			{
				root = GetLeaf(0);
			}
			else
			{
				root = GetNode(0);
				PostProcessNode(root, *this);
			}
		}

		size_t queueSize = pow2roundup(nodes.size() + leaves.size());
		if (queueSize < 2)
		{
			queueSize = 2;
		}
		heap.resize(leaves.size()+1);
		localSearchStateParams.resize(queueSize);

		//printf("\n\nNodes: %d, Leaves: %d\n\n", (int)nodes.size(), (int)leaves.size());

		//printf("\n\n\nHeapItem: %d\n\n\n", sizeof(HeapItem));
	}

	SearchContext::~SearchContext()
	{

		//printf("\n\nTotoal Black Leaves: %u\nTotal Grey Leaves: %d\nNumber of Queries: %d\n", totalBlackLeaves, totalGreyLeaves, totalQueries);

	}


	size_t SearchContext::AllocateNodeIdx()
	{
		nodes.emplace_back();
		return nodes.size() - 1;
	}

	Node * SearchContext::GetNode(size_t nodeIdx)
	{
		return &nodes[nodeIdx];
	}

	size_t SearchContext::SetAsLeafIdx(size_t nodeIdx)
	{
		if (nodeIdx != nodes.size() - 1)
		{
			printf("szar!!!\n");
		}
		leaves.push_back(nodes[nodeIdx]);
		nodes.pop_back();
		return leaves.size() - 1;
	}

	Node * SearchContext::GetLeaf(size_t leafIdx)
	{
		return &leaves[leafIdx];
	}

	static inline void CalcualteBlackGrey(Node * node, EdgeState lxe, EdgeState lye, EdgeState hxe, EdgeState hye, bool & black, bool & grey)
	{
		black = lxe == INSIDE & hxe == INSIDE & lye == INSIDE & hye == INSIDE;
		grey = !black & ((lxe == INSIDE | hxe == INSIDE | (lxe == BELOW & hxe == ABOVE)) & (lye == INSIDE | hye == INSIDE | (lye == BELOW & hye == ABOVE)));
	}

	void SearchContext::ProcessSearchState(const SearchStateParams & params, SearchContext & ctx, SearchStateParams * lssp, size_t & lsspHead, size_t & lsspTail)
	{
		bool black;
		bool grey;
		CalcualteBlackGrey(params.node, params.lxe, params.lye, params.hxe, params.hye, black, grey);

		if (black)
		{
			const size_t first = params.node->leavesIdxFirst;
			const size_t last = params.node->leavesIdxLast;
			const size_t count = last - first + 1;
			const size_t base = heapHead;
			heapHead += count;
			for (size_t i = 0; i < count; i++)
			{
				Node * n = GetLeaf(first + i);
				heap[base + i].begin = n->ptBegin;
				heap[base + i].end = n->ptEnd;
				heap[base + i].black = true;
			}
			return;
		}
		else if (grey)
		{
			if (params.node->children[0] == nullptr)
			{
				const size_t first = params.node->leavesIdxFirst;
				const size_t last = params.node->leavesIdxLast;
				const size_t count = last - first + 1;
				const size_t base = heapHead;
				heapHead += count;
				for (size_t i = 0; i < count; i++)
				{
					Node * n = GetLeaf(first + i);
					heap[base + i].begin = n->ptBegin;
					heap[base + i].end = n->ptEnd;
					heap[base + i].black = false;
				}
				return;
			}
		}
		else
		{
			return;
		}

		Node * child1 = params.node->children[0];
		Node * child2 = params.node->children[1];

		if (params.node->direction == Node::SPLIT_X)
		{
			lssp[lsspHead++] = SearchStateParams(child1, params.lxe, params.lye, child1->hx <= ctx.searchLx ? BELOW : child1->hx > ctx.searchHx ? ABOVE : INSIDE, params.hye);
			lssp[lsspHead++] = SearchStateParams(child2, child2->lx < ctx.searchLx ? BELOW : child2->lx >= ctx.searchHx ? ABOVE : INSIDE, params.lye, params.hxe, params.hye);
		}
		else // SPLIT_Y
		{
			lssp[lsspHead++] = SearchStateParams(child1, params.lxe, params.lye, params.hxe, child1->hy <= ctx.searchLy ? BELOW : child1->hy > ctx.searchHy ? ABOVE : INSIDE);
			lssp[lsspHead++] = SearchStateParams(child2, params.lxe, child2->ly < ctx.searchLy ? BELOW : child2->ly >= ctx.searchHy ? ABOVE : INSIDE, params.hxe, params.hye);
		}
	}

	void SearchContext::SearchCycle()
	{
		SearchStateParams * lssp = localSearchStateParams.data();
		size_t lsspHead = 1;
		size_t lsspTail = 0;

		while (lsspHead != lsspTail)
		{
			ProcessSearchState(lssp[lsspTail++], *this, lssp, lsspHead, lsspTail);
		}
	}


	void SearchContext::BuildHeap()
	{
		for (size_t i = heapHead / 2 + 2; i != 0; --i)
		{
			RecursiveFixHeapItem(i);
		}
	}

	static bool heap_less(const HeapItem & n1, const HeapItem & n2)
	{
		return n1.begin->rank < n2.begin->rank;
	}

	void SearchContext::FixEmptyRoot()
	{
		size_t idx = 1;
		for (;;)
		{
			const size_t idxLeft = 2 * idx;
			const size_t idxRight = 2 * idx + 1;

			if (idxRight < heapHead)
			{
				if (heap_less(heap[idxLeft], heap[idxRight]))
				{
					heap[idx] = heap[idxLeft];
					idx = idxLeft;
					continue;
				}
				else
				{
					heap[idx] = heap[idxRight];
					idx = idxRight;
					continue;
				}
			}
			else if (idxLeft < heapHead)
			{
				heap[idx] = heap[idxLeft];
				idx = idxLeft;
				continue;
			}
			else
			{
				heap[idx] = heap[--heapHead];
			}
			break;
		}
	}

	void SearchContext::RecursiveFixHeapItem(size_t idx)
	{
		for (;;)
		{
			const size_t idxLeft = 2 * idx;
			const size_t idxRight = 2 * idx + 1;

			if (idxRight < heapHead)
			{
				uint32_t r = heap[idx].begin->rank;
				uint32_t lr = heap[idxLeft].begin->rank;
				uint32_t rr = heap[idxRight].begin->rank;


				if (lr < r | rr < r)
				{
					if (lr < rr)
					{
						std::swap(heap[idx], heap[idxLeft]);
						idx = idxLeft;
						continue;
					}
					else
					{
						std::swap(heap[idx], heap[idxRight]);
						idx = idxRight;
						continue;
					}
				}
			}
			else if (idxLeft < heapHead)
			{
				if (heap_less(heap[idxLeft], heap[idx]))
				{
					std::swap(heap[idx], heap[idxLeft]);
					idx = idxLeft;
					continue;
				}
			}
			break;
		}
	}

	const Point * SearchContext::PopHeapRoot(bool & black)
	{
		const Point * ret = heap[1].begin;
		black = heap[1].black;

		heap[1].begin++;
		if (heap[1].begin == heap[1].end)
		{
			FixEmptyRoot();
		}
		else
		{
			RecursiveFixHeapItem(1);
		}
		
		return ret;
	}

	int SearchContext::Search(float searchLx, float searchLy, float searchHx, float searchHy, const int32_t count, Point* out_points)
	{
		totalQueries++;

		this->searchLx = searchLx;
		this->searchLy = searchLy;
		this->searchHx = searchHx;
		this->searchHy = searchHy;


		EdgeState lxe = root->lx <  searchLx ? BELOW : root->lx >= searchHx ? ABOVE : INSIDE;
		EdgeState lye = root->ly <  searchLy ? BELOW : root->ly >= searchHy ? ABOVE : INSIDE;
		EdgeState hxe = root->hx <= searchLx ? BELOW : root->hx >  searchHx ? ABOVE : INSIDE;
		EdgeState hye = root->hy <= searchLy ? BELOW : root->hy >  searchHy ? ABOVE : INSIDE;
		

		localSearchStateParams[0] = SearchStateParams(root, lxe, lye, hxe, hye);
		heapHead = 1;
		SearchCycle();

		//return 0;

		BuildHeap();

		int found = 0;

		while(heapHead != 1)
		{
			bool black;
			const Point * pt = PopHeapRoot(black);
			//__m128 a = _mm_setr_ps(searchLx, -searchHx, searchLy, -searchHy);
			//__m128 b = _mm_setr_ps(ret->x, -ret->x, ret->y, -ret->y);
			//__m128 c = _mm_cmple_ps(a, b);

			if (black | (pt->x >= searchLx & pt->x <= searchHx & pt->y >= searchLy & pt->y <= searchHy))
			//if (!(c.m128_i32[0] & c.m128_i32[1] & c.m128_i32[2] & c.m128_i32[3]))
			{
				*out_points++ = *pt;
				if (++found == count)
				{
					break;
				}
			}
		}

		return found;
	}

}
