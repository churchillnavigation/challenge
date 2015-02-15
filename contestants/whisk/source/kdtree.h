// Copyright 2015 Paul Lange
#ifndef KDTREE_HEADER
#define KDTREE_HEADER
#include "fastPoint.h"
#include "BinaryTree.h"
#include "LocalQueue.h"
#include "Task.h"

#include <assert.h>
#include <vector>
#include <algorithm>
//#include <memory>
#include <array>
#include <utility>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>


namespace {
	static const size_t node_cache_length = 20;
	static const size_t leaf_cache_length = 160; // multiple of 16 (160 was a good value)
	static const int32_t null_rank = 0xFFFFFF;
	typedef std::array<Rank3b, node_cache_length> RankList;
	typedef LocalQueue<Task, 1024> TaskQueue;
	struct Empty{};

	// merge two sorted lists, sorted from highest to lowest and only keeping the lowest
	// in2_begin must already have out_size elements
	template<class T1, class T2, class T3>
	int32_t merge_results(T1 in1_begin, T1 in1_end, T2 in2_begin, T3 out_begin, size_t out_size) {
		//std::fill(out_begin, out_begin+out_size, null_rank);
		auto out = out_begin;
		auto size = out_size;
		while (size--) {
			if ((in1_begin == in1_end) || (*in2_begin < *in1_begin)) {
				*out = *in2_begin;
				++in2_begin;
			} else if(*in2_begin == *in1_begin) {
				++in1_begin;
				*out = *in2_begin;
				++in2_begin;
			} else {
				*out = *in1_begin;
				++in1_begin;
			}
			++out;
		}
		return out_begin[out_size-1];
	}
};

struct Partition {
	fastFloat divider;
	struct Child {
		int32_t lowest_rank;
		//int32_t highest_rank;
	} left, right;
};

struct RankCache {
	// RankList loads unaligned dwords, so we can't have it at the end of a cpu cache line
	RankList cache;
	char padding[4]; 
};
static_assert(sizeof(RankCache)%64==0, "Not cache line aligned");

struct RankPoint {
	int32_t rank;
	fastFloat x,y;
	RankPoint& operator=(const Point &point) {
		rank = point.rank;
		x = (fastFloat)point.x;
		y = (fastFloat)point.y;
		return *this;
	}
	RankPoint(const int32_t rank, const float x, const float y)
		:rank(rank), x((fastFloat)x), y((fastFloat)y)
	{}
};

typedef std::array<RankPoint, leaf_cache_length> RankPointList;
static_assert(sizeof(RankPointList)%64==0, "Not cache line aligned");

inline int test_x(const fastRect &rect, const fastFloat &divider) {
	int dir = 0;
	if (rect.lx <= divider) dir -= 1;
	if (rect.hx >= divider) dir += 1;
	return dir;
}
inline int test_y(const fastRect &rect, const fastFloat &divider) {
	int dir = 0;
	if (rect.ly <= divider) dir -= 1;
	if (rect.hy >= divider) dir += 1;
	return dir;
}

template<class T1, class T2>
auto round_up_divide(const T1 dividend, const T2 divisor) -> decltype(dividend / divisor) {return (dividend + divisor - 1)/divisor;}

class PointTree {
	typedef std::vector<Point> PointList; // TODO: make the Point class aligned so it doesn't cross the cache line boundary, which probably means just making it 16 bytes
	
	PointList points;
	BinaryTree<Partition, Empty> partitions;
	BinaryTree<RankCache, RankCache> rankCache;
	BinaryTree<Empty, RankPointList> rankPointCache;

	fastRect query_rect;
	std::vector<int32_t> results, scratch;
	size_t max_result; // size of results and scratch arrays
	int32_t highest_rank;
	//int32_t highest_enqueued_rank;
	TaskQueue taskQueue;

	void SetHighestRank(int32_t rank) {
		highest_rank = rank;
	//	EnqueueHighestRank(rank);
	}
	//void EnqueueHighestRank(int32_t rank) {
	//	if (rank < highest_enqueued_rank)
	//		highest_enqueued_rank = rank;
	//}

public:
	PointTree(const Point* points_begin, const Point* points_end)
		: points(points_begin, points_end)
		, partitions(TreeDimensions::fromLeaves(round_up_divide(points.size(), leaf_cache_length)))
		, rankCache(Dimensions())
		, rankPointCache(Dimensions())
	{
		BuildTree(TreeIndex::Root(), points.begin(), points.end(), /*TODO:ugly workaround*/ partitions.getNode(TreeIndex::Root()).left);
		std::sort(points.begin(), points.end(), sort_rank<Point>);
		
		results.reserve(20);
		scratch.reserve(20);

		// prefetch the index root for next search
		partitions.PrefetchNode(0, 1);
	}
	
private:
	bool is_x_divided(const TreeIndex index) const {return index.Level() % 2 == 0;}
	bool is_y_divided(const TreeIndex index) const {return index.Level() % 2 == 1;}
	TreeDimensions Dimensions() const {
		return partitions.Dimensions();
	}

	void BuildLeaf(const TreeIndex index, PointList::iterator begin, PointList::iterator end, Partition::Child &child) {
		RankPointList &leafCache = rankPointCache.getLeaf(index);
		assert( (size_t(end - begin) <= leafCache.size()) && "Too many elements to build leaf" );
		std::sort(begin, end, sort_rank<Point>);
		auto out = std::copy(begin, end, leafCache.begin());
		if (out < leafCache.end())
			out->rank = null_rank;

		RankList &ranks = rankCache.getLeaf(index).cache;
		BuildRankCache(ranks, begin, end);
		child.lowest_rank = ranks[0];
	}
	// return the highest valid rank in the list
	void BuildRankCache(RankList &ranks, PointList::iterator begin, PointList::iterator end) {
		size_t length = end - begin;
		auto cache_end = end;
		if (length > ranks.size())
			cache_end = begin + ranks.size();
		std::partial_sort(begin, cache_end, end, sort_rank<Point>);
		auto out = ranks.begin();
		for (; begin != cache_end; ++begin, ++out)
			*out = begin->rank;
		if (out < ranks.end())
			*out = null_rank;
		//return *(--out);
	}
	// Note that equal values can appear on either side of a partition
	// if we didn't allow that then the tree could be unbalanced
	// so we must make sure our range query checks boths sides when values are equal
	void BuildTree(const TreeIndex index, PointList::iterator begin, PointList::iterator end, Partition::Child &child) {
		if (index.isLeaf(Dimensions())) {
			BuildLeaf(index, begin, end, child);
		} else {
			// Build node
			size_t size = end - begin;
			RankList &ranks = rankCache.getNode(index).cache;
			BuildRankCache(ranks, begin, end);
			child.lowest_rank = ranks[0];

			Partition &part = partitions.getNode(index);
			auto middle = begin + size / 2;
			if (is_x_divided(index)) {
				std::nth_element(begin, middle, end, sort_x<Point>);
				part.divider = (fastFloat)middle->x;
			} else {
				std::nth_element(begin, middle, end, sort_y<Point>);
				part.divider = (fastFloat)middle->y;
			}
			
			BuildTree(index.LeftChild(),  begin,  middle, part.left);
			BuildTree(index.RightChild(), middle, end,    part.right);
		}
	}

public:
	int32_t search(const Rect rect, const int32_t count, Point* out_points) {
		query_rect = rect;
		max_result = count;
		results.clear(); results.resize(max_result, null_rank);
		scratch.clear(); scratch.resize(max_result, null_rank);
		highest_rank = null_rank;

		Enqueue(Task::Query(TreeIndex::Root()));
		RunTasks();

		// honestly not much of an improvement, but was measurable
		for (auto i=results.cbegin(), e=results.cend(); i!=e; ++i) {
			const int32_t value = *i;
			if (value == null_rank)
				break;
			_mm_prefetch((char*)&points[value], _MM_HINT_T0);
		}

		{ // Prefetch out_points (13bytes * 20, 5 cache lines)
			char* out_ptr = (char*)out_points;
			_mm_prefetch(out_ptr, _MM_HINT_T0);
			_mm_prefetch(out_ptr + 64, _MM_HINT_T0);
			_mm_prefetch(out_ptr + 64 * 2, _MM_HINT_T0);
			_mm_prefetch(out_ptr + 64 * 3, _MM_HINT_T0);
			_mm_prefetch(out_ptr + 64 * 4, _MM_HINT_T0);
		}

		size_t result_count = 0;
		for (auto i=results.cbegin(), e=results.cend(); i!=e; ++i) {
			const int32_t value = *i;
			if (value == null_rank)
				break;
			*out_points = (Point)points[value];
			++out_points;
			++result_count;
		}
		
		// prefetch the index root for next search
		partitions.PrefetchNode(0, 1);
		return (int32_t)result_count;
	}

	void RunTasks() {
		Task task;
		while (taskQueue.pop(task))
			task.Run(*this);
	}
	void PrefetchTestAndMerge(const TreeIndex index, const int level) const {
		//rankPointCache.PrefetchLeaf(index, level);
		char * ptr = (char*)&rankPointCache.getLeaf(index);
		_mm_prefetch(ptr, 1);
		_mm_prefetch(ptr+64, 1);
	}
	void TestAndMerge(const TreeIndex index) {
		RankPointList &list = rankPointCache.getLeaf(index);
		int32_t *out = (int32_t*)_alloca(sizeof(int32_t) * max_result);
		int32_t *out_begin = out, *out_end = out + max_result;
		auto list_begin = list.begin(), list_end = list.end();

		for (; out != out_end && list_begin != list_end; ++list_begin) {
			int32_t cur_rank = list_begin->rank;
			if (cur_rank >= highest_rank)
				break;
			if (is_inside(query_rect, *list_begin)) {
				*out = list_begin->rank;
				++out;
			}
		}
		MergeResults(out_begin, out);
	}
	void PrefetchAddAll(const TreeIndex index, const int level) const {
		rankCache.PrefetchNode(index, level);
	}
	void AddAll(const TreeIndex index) {
		RankList &rankList = rankCache.getNode(index).cache;
		MergeResults(rankList.cbegin(), rankList.cend());
		// TODO: add if (node_cache_length < max_result) and if the last rank in the rank20 cache  is less than the new highest_rank than we need to add the children also
	}
	void PrefetchQuery(const TreeIndex index, const int level) const {
		partitions.PrefetchNode(index, level);
	}
	void Query(const TreeIndex index, const int border_check) {
		const Partition &part = partitions.getNode(index);
		int dir;
		int left_border  = border_check;
		int right_border = border_check;
		if (is_x_divided(index)) {
			dir = test_x(query_rect, part.divider);
			if (dir==0) {
				left_border  |= 1;
				right_border |= 2;
			}
		} else {
			dir = test_y(query_rect, part.divider);
			if (dir==0) {
				left_border  |= 4;
				right_border |= 8;
			}
		}
		if (dir <= 0)
			QueryChild(index.LeftChild(),  left_border,  part.left);
		if (dir >= 0)
			QueryChild(index.RightChild(), right_border, part.right);
	}
	void QueryChild(const TreeIndex index, const int border_check, const Partition::Child &child) {
		if (highest_rank <= child.lowest_rank)
			return;
		if (border_check == 15) {
			PrefetchAddAll(index, 1);
			Enqueue(Task::AddAll(index));
		} else if (index.isLeaf(Dimensions())) {
			PrefetchTestAndMerge(index, 1);
			Enqueue(Task::TestAndMerge(index));
		} else {
			PrefetchQuery(index, 1);
			Enqueue(Task::Query(index, border_check));
		}
	}
	template<class T>
	void MergeResults(T in_begin, T in_end) {
		SetHighestRank( merge_results(in_begin, in_end, results.begin(), scratch.begin(), scratch.size()) );
		results.swap(scratch);
	}
	void Enqueue(const Task &task) {
		if (!taskQueue.push(task))
			task.Run(*this);
	}
};
#endif