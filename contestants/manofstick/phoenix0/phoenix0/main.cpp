
#include "../../point_search.h"

#include <iostream>
#include <vector>
#include <functional>
#include <chrono>
#include "ppl.h"
#include "ppltasks.h"

#include "bitset.h"





#ifndef ALIGNMENT_ALLOCATOR_H
#define ALIGNMENT_ALLOCATOR_H

#include <stdlib.h>
#include <malloc.h>

template <typename T, std::size_t N = 16>
class AlignmentAllocator {
public:
	typedef T value_type;
	typedef std::size_t size_type;
	typedef std::ptrdiff_t difference_type;

	typedef T * pointer;
	typedef const T * const_pointer;

	typedef T & reference;
	typedef const T & const_reference;

public:
	inline AlignmentAllocator() throw () { }

	template <typename T2>
	inline AlignmentAllocator(const AlignmentAllocator<T2, N> &) throw () { }

	inline ~AlignmentAllocator() throw () { }

	inline pointer adress(reference r) {
		return &r;
	}

	inline const_pointer adress(const_reference r) const {
		return &r;
	}

	inline pointer allocate(size_type n) {
		return (pointer)_aligned_malloc(n*sizeof(value_type), N);
	}

	inline void deallocate(pointer p, size_type) {
		_aligned_free(p);
	}

	inline void construct(pointer p, const value_type & wert) {
		new (p)value_type(wert);
	}

	inline void destroy(pointer p) {
		p->~value_type();
	}

	inline size_type max_size() const throw () {
		return size_type(-1) / sizeof(value_type);
	}

	template <typename T2>
	struct rebind {
		typedef AlignmentAllocator<T2, N> other;
	};

	bool operator!=(const AlignmentAllocator<T, N>& other) const  {
		return !(*this == other);
	}

	// Returns true if and only if storage allocated from *this
	// can be deallocated from other, and vice versa.
	// Always returns true for stateless allocators.
	bool operator==(const AlignmentAllocator<T, N>& other) const {
		return true;
	}
};

#endif







using namespace std;

struct Coordinate {
	Coordinate() : x(0), y(0) {}
	Coordinate(float x, float y) : x(x), y(y) { }
	float x;
	float y;
};

typedef std::vector<float, AlignmentAllocator<float, 16>> Floats;
struct SplitCoordinates
{
	Floats xs;
	Floats ys;
};

const int MaxDepth = 8;
const int MinimumSizeToUseMappingSort = 10000;
const double MaxGapFraction = 0.1;

typedef std::vector<Coordinate> Coordinates;
typedef std::vector<int8_t>     Ids;
typedef std::vector<int32_t>    Ranks;

struct Mappings
{
	Mappings(const Mappings& that) = delete;

	Mappings(int point_count, const Coordinates& coordinates, const Ids& ids)
		: points_count(point_count)
		, coordinates(coordinates)
		, ids(ids)
		, x_sorted_rank_idxes(point_count)
		, y_sorted_rank_idxes(point_count)
		, rank_idx_to_x_sorted_idxes(point_count)
		, rank_idx_to_y_sorted_idxes(point_count)
	{}

	int points_count;
	const Coordinates& coordinates;
	const Ids& ids;
	vector<int32_t> x_sorted_rank_idxes;
	vector<int32_t> y_sorted_rank_idxes;
	vector<int32_t> rank_idx_to_x_sorted_idxes;
	vector<int32_t> rank_idx_to_y_sorted_idxes;
};

static void SortByRank(int total_points_count, Ranks& rankCollection)
{
	BitSet ws(total_points_count);
	for (auto it : rankCollection)
		ws.set(it);

	int idx = 0;
	for (auto it = ws.get_values_iterator(); !it.at_end(); ++it)
		rankCollection[idx++] = *it;
}

static void SortByCoordinate(int point_count, Ranks& rankCollection, const vector<int32_t>& rank_idx_to_coord_sorted_idxes, const vector<int32_t>& coord_sorted_rank_idxes)
{
	BitSet ws(point_count);
	for (auto it : rankCollection)
		ws.set(rank_idx_to_coord_sorted_idxes[it]);

	int idx = 0;
	for (auto it = ws.get_values_iterator(); !it.at_end(); ++it)
		rankCollection[idx++] = coord_sorted_rank_idxes[*it];
}

struct Surface;
typedef shared_ptr<Surface> SurfacePtr;
SurfacePtr surface_factory(const Mappings&, Ranks&, int depth);

struct PointIterator;

struct PointIterator
{
	virtual int32_t best_next() = 0;
	virtual int32_t iterate() = 0;
};

struct NullIterator
	: PointIterator
{
	int32_t best_next() { return numeric_limits<int>::max(); }
	int32_t iterate() { return -1; }
};

NullIterator null_iterator;

struct AllIterator
	: PointIterator
{
	const Ranks& points;

	Ranks::const_iterator it;

	AllIterator(const Ranks& points)
		: points(points)
	{}

	void reset()
	{
		this->it = points.begin();
	}

	int32_t best_next()
	{
		if (it == points.end())
			return numeric_limits<int>::max();
		return *it;
	}

	int32_t iterate()
	{
		if (it == points.end())
			return -1;
		auto result = *it;
		++it;
		return result;
	}
};

class GetNextIdx
{
	static int Aget_next_idx_lxhxlyhy(const Ranks& points, const Coordinates& coordinates, Rect rect, int idx)
	{
		auto count = (int)points.size();

		Rect r = rect;
		auto i = idx;

		auto rly = _mm_set1_ps(r.ly);
		auto rhy = _mm_set1_ps(r.hy);
		auto rlx = _mm_set1_ps(r.lx);
		auto rhx = _mm_set1_ps(r.hx);
		for (; i < count - 4; i += 4)
		{
			auto ys = _mm_set_ps(coordinates[points[i]].y, coordinates[points[i + 1]].y, coordinates[points[i + 2]].y, coordinates[points[i + 3]].y);
			auto rly_check = _mm_cmpge_ps(ys, rly);
			auto rhy_check = _mm_cmple_ps(ys, rhy);
			auto mm_ys = _mm_movemask_ps(rly_check) & _mm_movemask_ps(rhy_check);
			if (!mm_ys)
				continue;

			auto xs = _mm_set_ps(coordinates[points[i]].x, coordinates[points[i + 1]].x, coordinates[points[i + 2]].x, coordinates[points[i + 3]].x);
			auto rlx_check = _mm_cmpge_ps(xs, rlx);
			auto rhx_check = _mm_cmple_ps(xs, rhx);
			auto mm_xs = _mm_movemask_ps(rlx_check) & _mm_movemask_ps(rhx_check);
			if (mm_ys & mm_xs)
				break;
		}

		for (; i < count; ++i)
		{
			auto& pt = coordinates[points[i]];
			if ((pt.y >= r.ly && pt.y <= r.hy && pt.x >= r.lx && pt.x <= r.hx))
				return i;
		}

		return count;
	}

	static int Aget_next_idx_lxhxly(const Ranks& points, const Coordinates& coordinates, Rect rect, int idx)
	{
		auto count = (int)points.size();

		Rect r = rect;
		auto i = idx;
		for (; i < count; ++i)
		{
			auto& pt = coordinates[points[i]];
			if ((pt.y >= r.ly && pt.x >= r.lx && pt.x <= r.hx))
				return i;
		}

		return count;
	}

	static int Aget_next_idx_lxhxhy(const Ranks& points, const Coordinates& coordinates, Rect rect, int idx)
	{
		auto count = (int)points.size();

		Rect r = rect;
		auto i = idx;
		for (; i < count; ++i)
		{
			auto& pt = coordinates[points[i]];
			if ((pt.y <= r.hy && pt.x >= r.lx && pt.x <= r.hx))
				return i;
		}

		return count;
	}

	static int Aget_next_idx_lxhx(const Ranks& points, const Coordinates& coordinates, Rect rect, int idx)
	{
		auto count = (int)points.size();

		Rect r = rect;
		auto i = idx;
		for (; i < count; ++i)
		{
			auto& pt = coordinates[points[i]];
			if ((pt.x >= r.lx && pt.x <= r.hx))
				return i;
		}

		return count;
	}

	static int Aget_next_idx_lxlyhy(const Ranks& points, const Coordinates& coordinates, Rect rect, int idx)
	{
		auto count = (int)points.size();

		Rect r = rect;
		auto i = idx;
		for (; i < count; ++i)
		{
			auto& pt = coordinates[points[i]];
			if ((pt.y >= r.ly && pt.y <= r.hy && pt.x >= r.lx))
				return i;
		}

		return count;
	}

	static int Aget_next_idx_lxly(const Ranks& points, const Coordinates& coordinates, Rect rect, int idx)
	{
		auto count = (int)points.size();

		Rect r = rect;
		auto i = idx;
		for (; i < count; ++i)
		{
			auto& pt = coordinates[points[i]];
			if ((pt.y >= r.ly && pt.x >= r.lx))
				return i;
		}

		return count;
	}

	static int Aget_next_idx_lxhy(const Ranks& points, const Coordinates& coordinates, Rect rect, int idx)
	{
		auto count = (int)points.size();

		Rect r = rect;
		auto i = idx;
		for (; i < count; ++i)
		{
			auto& pt = coordinates[points[i]];
			if ((pt.y <= r.hy && pt.x >= r.lx))
				return i;
		}

		return count;
	}

	static int Aget_next_idx_lx(const Ranks& points, const Coordinates& coordinates, Rect rect, int idx)
	{
		auto count = (int)points.size();

		Rect r = rect;
		auto i = idx;
		for (; i < count; ++i)
		{
			auto& pt = coordinates[points[i]];
			if ((pt.x >= r.lx))
				return i;
		}

		return count;
	}

	static int Aget_next_idx_hxlyhy(const Ranks& points, const Coordinates& coordinates, Rect rect, int idx)
	{
		auto count = (int)points.size();

		Rect r = rect;
		auto i = idx;
		for (; i < count; ++i)
		{
			auto& pt = coordinates[points[i]];
			if ((pt.y >= r.ly && pt.y <= r.hy && pt.x <= r.hx))
				return i;
		}

		return count;
	}

	static int Aget_next_idx_hxly(const Ranks& points, const Coordinates& coordinates, Rect rect, int idx)
	{
		auto count = (int)points.size();

		Rect r = rect;
		auto i = idx;
		for (; i < count; ++i)
		{
			auto& pt = coordinates[points[i]];
			if ((pt.y >= r.ly && pt.x <= r.hx))
				return i;
		}

		return count;
	}

	static int Aget_next_idx_hxhy(const Ranks& points, const Coordinates& coordinates, Rect rect, int idx)
	{
		auto count = (int)points.size();

		Rect r = rect;
		auto i = idx;
		for (; i < count; ++i)
		{
			auto& pt = coordinates[points[i]];
			if ((pt.y <= r.hy && pt.x <= r.hx))
				return i;
		}

		return count;
	}

	static int Aget_next_idx_hx(const Ranks& points, const Coordinates& coordinates, Rect rect, int idx)
	{
		auto count = (int)points.size();

		Rect r = rect;
		auto i = idx;
		for (; i < count; ++i)
		{
			auto& pt = coordinates[points[i]];
			if ((pt.x <= r.hx))
				return i;
		}

		return count;
	}

	static int Aget_next_idx_lyhy(const Ranks& points, const Coordinates& coordinates, Rect rect, int idx)
	{
		auto count = (int)points.size();

		Rect r = rect;
		auto i = idx;
		for (; i < count; ++i)
		{
			auto& pt = coordinates[points[i]];
			if ((pt.y >= r.ly && pt.y <= r.hy))
				return i;
		}

		return count;
	}

	static int Aget_next_idx_ly(const Ranks& points, const Coordinates& coordinates, Rect rect, int idx)
	{
		auto count = (int)points.size();

		Rect r = rect;
		auto i = idx;
		for (; i < count; ++i)
		{
			auto& pt = coordinates[points[i]];
			if ((pt.y >= r.ly))
				return i;
		}

		return count;
	}

	static int Aget_next_idx_hy(const Ranks& points, const Coordinates& coordinates, Rect rect, int idx)
	{
		auto count = (int)points.size();

		Rect r = rect;
		auto i = idx;
		for (; i < count; ++i)
		{
			auto& pt = coordinates[points[i]];
			if ((pt.y <= r.hy))
				return i;
		}

		return count;
	}

	static int Aget_next_idx_(const Ranks& points, const Coordinates& coordinates, Rect rect, int idx)
	{
		return idx;
	}

public:
	typedef int(*FunctionDefinition)(const Ranks& points, const Coordinates& coordinates, Rect rect, int idx);

	static FunctionDefinition get_next_idx(bool check_lx, bool check_hx, bool check_ly, bool check_hy)
	{
		if (check_lx)
		{
			if (check_hx)
			{
				if (check_ly)
				{
					if (check_hy)
						return Aget_next_idx_lxhxlyhy;
					else
						return Aget_next_idx_lxhxly;
				}
				else
				{
					if (check_hy)
						return Aget_next_idx_lxhxhy;
					else
						return Aget_next_idx_lxhx;
				}
			}
			else
			{
				if (check_ly)
				{
					if (check_hy)
						return Aget_next_idx_lxlyhy;
					else
						return Aget_next_idx_lxly;
				}
				else
				{
					if (check_hy)
						return Aget_next_idx_lxhy;
					else
						return Aget_next_idx_lx;
				}
			}
		}
		else
		{
			if (check_hx)
			{
				if (check_ly)
				{
					if (check_hy)
						return Aget_next_idx_hxlyhy;
					else
						return Aget_next_idx_hxly;
				}
				else
				{
					if (check_hy)
						return Aget_next_idx_hxhy;
					else
						return Aget_next_idx_hx;
				}
			}
			else
			{
				if (check_ly)
				{
					if (check_hy)
						return Aget_next_idx_lyhy;
					else
						return Aget_next_idx_ly;
				}
				else
				{
					if (check_hy)
						return Aget_next_idx_hy;
					else
						return Aget_next_idx_;
				}
			}
		}
	}

};

struct SimpleIterator
	: PointIterator
{
	const Coordinates& coordinates;
	const Ranks& points;

	Rect rect;
	int idx;

	GetNextIdx::FunctionDefinition fn_get_next_idx;

	SimpleIterator(const Coordinates& coordinates, const Ranks& points)
		: points(points)
		, coordinates(coordinates)
	{}

	void reset(Rect surface_area, const Rect& rect)
	{
		this->rect = rect;

		auto check_lx = surface_area.lx <= rect.lx;
		auto check_hx = surface_area.hx >= rect.hx;
		auto check_ly = surface_area.ly <= rect.ly;
		auto check_hy = surface_area.hy >= rect.hy;

		fn_get_next_idx = GetNextIdx::get_next_idx(check_lx, check_hx, check_ly, check_hy);

		idx = -1;
	}

	int32_t best_next()
	{
		if (idx + 1 >= points.size())
			return numeric_limits<int>::max();

		return points[idx + 1];
	}

	int32_t iterate()
	{
		if (idx == points.size())
			return -1;
		idx = fn_get_next_idx(points, coordinates, rect, idx + 1);
		if (idx == points.size())
			return -1;

		return points[idx];
	}
};























class GetNextIdxSplit
{
	static int get_next_idx_lxhxlyhy(const Ranks& points, const SplitCoordinates& splitCoordinates, Rect rect, int idx)
	{
		auto count = (int)points.size();

		Rect r = rect;
		auto i = idx;

		for (; i < count && (i & 3); ++i)
		{
			auto y = splitCoordinates.ys[i];
			if (y >= r.ly && y <= r.hy)
			{
				auto x = splitCoordinates.xs[i];
				if (x >= r.lx && x <= r.hx)
					return i;
			}
		}

		auto ys_ptr = &splitCoordinates.ys[0];
		auto xs_ptr = &splitCoordinates.xs[0];
		auto rly = _mm_set1_ps(r.ly);
		auto rhy = _mm_set1_ps(r.hy);
		auto rlx = _mm_set1_ps(r.lx);
		auto rhx = _mm_set1_ps(r.hx);
		for (; i < count - 4; i += 4)
		{
			auto ys = _mm_load_ps(ys_ptr + i);
			auto rly_check = _mm_cmpge_ps(ys, rly);
			auto rhy_check = _mm_cmple_ps(ys, rhy);
			auto mm_ys = _mm_movemask_ps(rly_check) & _mm_movemask_ps(rhy_check);
			if (!mm_ys)
				continue;

			auto xs = _mm_load_ps(xs_ptr + i);
			auto rlx_check = _mm_cmpge_ps(xs, rlx);
			auto rhx_check = _mm_cmple_ps(xs, rhx);
			auto mm_xs = _mm_movemask_ps(rlx_check) & _mm_movemask_ps(rhx_check);
			if (mm_ys & mm_xs)
				break;
		}

		for (; i < count; ++i)
		{
			auto y = splitCoordinates.ys[i];
			if (y >= r.ly && y <= r.hy)
			{
				auto x = splitCoordinates.xs[i];
				if (x >= r.lx && x <= r.hx)
					return i;
			}
		}

		return count;
	}

	static int get_next_idx_lxhxly(const Ranks& points, const SplitCoordinates& splitCoordinates, Rect rect, int idx)
	{
		auto count = (int)points.size();

		Rect r = rect;
		auto i = idx;

		for (; i < count && (i & 3); ++i)
		{
			auto x = splitCoordinates.xs[i];
			if (x >= r.lx && x <= r.hx)
			{
				auto y = splitCoordinates.ys[i];
				if (y >= r.ly)
				{
					return i;
				}
			}
		}

		auto ys_ptr = &splitCoordinates.ys[0];
		auto xs_ptr = &splitCoordinates.xs[0];
		auto rly = _mm_set1_ps(r.ly);
		auto rlx = _mm_set1_ps(r.lx);
		auto rhx = _mm_set1_ps(r.hx);
		for (; i < count - 4; i += 4)
		{
			auto xs = _mm_load_ps(xs_ptr + i);
			auto rlx_check = _mm_cmpge_ps(xs, rlx);
			auto rhx_check = _mm_cmple_ps(xs, rhx);
			auto mm_xs = _mm_movemask_ps(rlx_check) & _mm_movemask_ps(rhx_check);
			if (!mm_xs)
				continue;

			auto ys = _mm_load_ps(ys_ptr + i);
			auto rly_check = _mm_cmpge_ps(ys, rly);
			auto mm_ys = _mm_movemask_ps(rly_check);
			if (mm_ys & mm_xs)
				break;

		}

		for (; i < count; ++i)
		{
			auto x = splitCoordinates.xs[i];
			if (x >= r.lx && x <= r.hx)
			{
				auto y = splitCoordinates.ys[i];
				if (y >= r.ly)
				{
					return i;
				}
			}
		}

		return count;
	}

	static int get_next_idx_lxhxhy(const Ranks& points, const SplitCoordinates& splitCoordinates, Rect rect, int idx)
	{
		auto count = (int)points.size();

		Rect r = rect;
		auto i = idx;

		for (; i < count && (i & 3); ++i)
		{
			auto x = splitCoordinates.xs[i];
			if (x >= r.lx && x <= r.hx)
			{
				auto y = splitCoordinates.ys[i];
				if (y <= r.hy)
					return i;
			}
		}

		auto ys_ptr = &splitCoordinates.ys[0];
		auto xs_ptr = &splitCoordinates.xs[0];
		auto rhy = _mm_set1_ps(r.hy);
		auto rlx = _mm_set1_ps(r.lx);
		auto rhx = _mm_set1_ps(r.hx);
		for (; i < count - 4; i += 4)
		{
			auto xs = _mm_load_ps(xs_ptr + i);
			auto rlx_check = _mm_cmpge_ps(xs, rlx);
			auto rhx_check = _mm_cmple_ps(xs, rhx);
			auto mm_xs = _mm_movemask_ps(rlx_check) & _mm_movemask_ps(rhx_check);
			if (!mm_xs)
				continue;

			auto ys = _mm_load_ps(ys_ptr + i);
			auto rhy_check = _mm_cmple_ps(ys, rhy);
			auto mm_ys = _mm_movemask_ps(rhy_check);
			if (mm_ys & mm_xs)
				break;
		}

		for (; i < count; ++i)
		{
			auto x = splitCoordinates.xs[i];
			if (x >= r.lx && x <= r.hx)
			{
				auto y = splitCoordinates.ys[i];
				if (y <= r.hy)
					return i;
			}
		}

		return count;
	}

	static int get_next_idx_lxhx(const Ranks& points, const SplitCoordinates& splitCoordinates, Rect rect, int idx)
	{
		auto count = (int)points.size();

		Rect r = rect;
		auto i = idx;

		for (; i < count && (i & 3); ++i)
		{
			auto x = splitCoordinates.xs[i];
			if (x >= r.lx && x <= r.hx)
				return i;
		}

		auto ptr = &splitCoordinates.xs[0];
		auto rlx = _mm_set1_ps(r.lx);
		auto rhx = _mm_set1_ps(r.hx);
		for (; i < count - 4; i += 4)
		{
			auto xs = _mm_load_ps(ptr + i);
			auto rlx_check = _mm_cmpge_ps(xs, rlx);
			auto rhx_check = _mm_cmple_ps(xs, rhx);
			if (_mm_movemask_ps(rlx_check) & _mm_movemask_ps(rhx_check))
				break;
		}

		for (; i < count; ++i)
		{
			auto x = splitCoordinates.xs[i];
			if (x >= r.lx && x <= r.hx)
			{
				return i;
			}
		}

		return count;
	}

	static int get_next_idx_lxlyhy(const Ranks& points, const SplitCoordinates& splitCoordinates, Rect rect, int idx)
	{
		auto count = (int)points.size();

		Rect r = rect;
		auto i = idx;

		for (; i < count && (i & 3); ++i)
		{
			auto y = splitCoordinates.ys[i];
			if (y >= r.ly && y <= r.hy)
			{
				auto x = splitCoordinates.xs[i];
				if (x >= r.lx)
					return i;
			}
		}

		auto ys_ptr = &splitCoordinates.ys[0];
		auto xs_ptr = &splitCoordinates.xs[0];
		auto rly = _mm_set1_ps(r.ly);
		auto rhy = _mm_set1_ps(r.hy);
		auto rlx = _mm_set1_ps(r.lx);
		for (; i < count - 4; i += 4)
		{
			auto ys = _mm_load_ps(ys_ptr + i);
			auto rly_check = _mm_cmpge_ps(ys, rly);
			auto rhy_check = _mm_cmple_ps(ys, rhy);
			auto mm_ys = _mm_movemask_ps(rly_check) & _mm_movemask_ps(rhy_check);
			if (!mm_ys)
				continue;

			auto xs = _mm_load_ps(xs_ptr + i);
			auto rlx_check = _mm_cmpge_ps(xs, rlx);
			auto mm_xs = _mm_movemask_ps(rlx_check);
			if (mm_ys & mm_xs)
				break;
		}

		for (; i < count; ++i)
		{
			auto y = splitCoordinates.ys[i];
			if (y >= r.ly && y <= r.hy)
			{
				auto x = splitCoordinates.xs[i];
				if (x >= r.lx)
					return i;
			}
		}

		return count;
	}

	static int get_next_idx_lxly(const Ranks& points, const SplitCoordinates& splitCoordinates, Rect rect, int idx)
	{
		auto count = (int)points.size();

		Rect r = rect;
		auto i = idx;

		for (; i < count && (i & 3); ++i)
		{
			auto y = splitCoordinates.ys[i];
			if (y >= r.ly)
			{
				auto x = splitCoordinates.xs[i];
				if (x >= r.lx)
					return i;
			}
		}

		auto ys_ptr = &splitCoordinates.ys[0];
		auto xs_ptr = &splitCoordinates.xs[0];
		auto rly = _mm_set1_ps(r.ly);
		auto rlx = _mm_set1_ps(r.lx);
		for (; i < count - 4; i += 4)
		{
			auto ys = _mm_load_ps(ys_ptr + i);
			auto rly_check = _mm_cmpge_ps(ys, rly);
			auto mm_ys = _mm_movemask_ps(rly_check);
			if (!mm_ys)
				continue;

			auto xs = _mm_load_ps(xs_ptr + i);
			auto rlx_check = _mm_cmpge_ps(xs, rlx);
			auto mm_xs = _mm_movemask_ps(rlx_check);
			if (mm_ys & mm_xs)
				break;
		}

		for (; i < count; ++i)
		{
			auto y = splitCoordinates.ys[i];
			if (y >= r.ly)
			{
				auto x = splitCoordinates.xs[i];
				if (x >= r.lx)
					return i;
			}
		}

		return count;
	}

	static int get_next_idx_lxhy(const Ranks& points, const SplitCoordinates& splitCoordinates, Rect rect, int idx)
	{
		auto count = (int)points.size();

		Rect r = rect;
		auto i = idx;

		for (; i < count && (i & 3); ++i)
		{
			auto y = splitCoordinates.ys[i];
			if (y <= r.hy)
			{
				auto x = splitCoordinates.xs[i];
				if (x >= r.lx)
					return i;
			}
		}

		auto ys_ptr = &splitCoordinates.ys[0];
		auto xs_ptr = &splitCoordinates.xs[0];
		auto rhy = _mm_set1_ps(r.hy);
		auto rlx = _mm_set1_ps(r.lx);
		for (; i < count - 4; i += 4)
		{
			auto ys = _mm_load_ps(ys_ptr + i);
			auto rhy_check = _mm_cmple_ps(ys, rhy);
			auto mm_ys = _mm_movemask_ps(rhy_check);
			if (!mm_ys)
				continue;

			auto xs = _mm_load_ps(xs_ptr + i);
			auto rlx_check = _mm_cmpge_ps(xs, rlx);
			auto mm_xs = _mm_movemask_ps(rlx_check);
			if (mm_ys & mm_xs)
				break;
		}

		for (; i < count; ++i)
		{
			auto y = splitCoordinates.ys[i];
			if (y <= r.hy)
			{
				auto x = splitCoordinates.xs[i];
				if (x >= r.lx)
					return i;
			}
		}

		return count;
	}

	static int get_next_idx_lx(const Ranks& points, const SplitCoordinates& splitCoordinates, Rect rect, int idx)
	{
		auto count = (int)points.size();

		Rect r = rect;
		auto i = idx;

		for (; i < count && (i & 3); ++i)
		{
			auto x = splitCoordinates.xs[i];
			if (x >= r.lx)
				return i;
		}

		auto xs_ptr = &splitCoordinates.xs[0];
		auto rlx = _mm_set1_ps(r.lx);
		for (; i < count - 4; i += 4)
		{
			auto xs = _mm_load_ps(xs_ptr + i);
			auto rlx_check = _mm_cmpge_ps(xs, rlx);
			auto mm_xs = _mm_movemask_ps(rlx_check);
			if (mm_xs)
				break;
		}

		for (; i < count; ++i)
		{
			auto x = splitCoordinates.xs[i];
			if (x >= r.lx)
				return i;
		}

		return count;
	}

	static int get_next_idx_hxlyhy(const Ranks& points, const SplitCoordinates& splitCoordinates, Rect rect, int idx)
	{
		auto count = (int)points.size();

		Rect r = rect;
		auto i = idx;

		for (; i < count && (i & 3); ++i)
		{
			auto y = splitCoordinates.ys[i];
			if (y >= r.ly && y <= r.hy)
			{
				auto x = splitCoordinates.xs[i];
				if (x <= r.hx)
					return i;
			}
		}

		auto ys_ptr = &splitCoordinates.ys[0];
		auto xs_ptr = &splitCoordinates.xs[0];
		auto rly = _mm_set1_ps(r.ly);
		auto rhy = _mm_set1_ps(r.hy);
		auto rhx = _mm_set1_ps(r.hx);
		for (; i < count - 4; i += 4)
		{
			auto ys = _mm_load_ps(ys_ptr + i);
			auto rly_check = _mm_cmpge_ps(ys, rly);
			auto rhy_check = _mm_cmple_ps(ys, rhy);
			auto mm_ys = _mm_movemask_ps(rly_check) & _mm_movemask_ps(rhy_check);
			if (!mm_ys)
				continue;

			auto xs = _mm_load_ps(xs_ptr + i);
			auto rhx_check = _mm_cmple_ps(xs, rhx);
			auto mm_xs = _mm_movemask_ps(rhx_check);
			if (mm_ys & mm_xs)
				break;
		}

		for (; i < count; ++i)
		{
			auto y = splitCoordinates.ys[i];
			if (y >= r.ly && y <= r.hy)
			{
				auto x = splitCoordinates.xs[i];
				if (x <= r.hx)
					return i;
			}
		}

		return count;
	}

	static int get_next_idx_hxly(const Ranks& points, const SplitCoordinates& splitCoordinates, Rect rect, int idx)
	{
		auto count = (int)points.size();

		Rect r = rect;
		auto i = idx;

		for (; i < count && (i & 3); ++i)
		{
			auto y = splitCoordinates.ys[i];
			if (y >= r.ly)
			{
				auto x = splitCoordinates.xs[i];
				if (x <= r.hx)
					return i;
			}
		}

		auto ys_ptr = &splitCoordinates.ys[0];
		auto xs_ptr = &splitCoordinates.xs[0];
		auto rly = _mm_set1_ps(r.ly);
		auto rhx = _mm_set1_ps(r.hx);
		for (; i < count - 4; i += 4)
		{
			auto ys = _mm_load_ps(ys_ptr + i);
			auto rly_check = _mm_cmpge_ps(ys, rly);
			auto mm_ys = _mm_movemask_ps(rly_check);
			if (!mm_ys)
				continue;

			auto xs = _mm_load_ps(xs_ptr + i);
			auto rhx_check = _mm_cmple_ps(xs, rhx);
			auto mm_xs = _mm_movemask_ps(rhx_check);
			if (mm_ys & mm_xs)
				break;
		}

		for (; i < count; ++i)
		{
			auto y = splitCoordinates.ys[i];
			if (y >= r.ly)
			{
				auto x = splitCoordinates.xs[i];
				if (x <= r.hx)
					return i;
			}
		}

		return count;
	}

	static int get_next_idx_hxhy(const Ranks& points, const SplitCoordinates& splitCoordinates, Rect rect, int idx)
	{
		auto count = (int)points.size();

		Rect r = rect;
		auto i = idx;

		for (; i < count && (i & 3); ++i)
		{
			auto y = splitCoordinates.ys[i];
			if (y <= r.hy)
			{
				auto x = splitCoordinates.xs[i];
				if (x <= r.hx)
					return i;
			}
		}

		auto ys_ptr = &splitCoordinates.ys[0];
		auto xs_ptr = &splitCoordinates.xs[0];
		auto rhy = _mm_set1_ps(r.hy);
		auto rhx = _mm_set1_ps(r.hx);
		for (; i < count - 4; i += 4)
		{
			auto ys = _mm_load_ps(ys_ptr + i);
			auto rhy_check = _mm_cmple_ps(ys, rhy);
			auto mm_ys = _mm_movemask_ps(rhy_check);
			if (!mm_ys)
				continue;

			auto xs = _mm_load_ps(xs_ptr + i);
			auto rhx_check = _mm_cmple_ps(xs, rhx);
			auto mm_xs = _mm_movemask_ps(rhx_check);
			if (mm_ys & mm_xs)
				break;
		}

		for (; i < count; ++i)
		{
			auto y = splitCoordinates.ys[i];
			if (y <= r.hy)
			{
				auto x = splitCoordinates.xs[i];
				if (x <= r.hx)
					return i;
			}
		}

		return count;
	}

	static int get_next_idx_hx(const Ranks& points, const SplitCoordinates& splitCoordinates, Rect rect, int idx)
	{
		auto count = (int)points.size();

		Rect r = rect;
		auto i = idx;

		for (; i < count && (i & 3); ++i)
		{
			auto x = splitCoordinates.xs[i];
			if (x <= r.hx)
				return i;
		}

		auto xs_ptr = &splitCoordinates.xs[0];
		auto rhx = _mm_set1_ps(r.hx);
		for (; i < count - 4; i += 4)
		{
			auto xs = _mm_load_ps(xs_ptr + i);
			auto rhx_check = _mm_cmple_ps(xs, rhx);
			auto mm_xs = _mm_movemask_ps(rhx_check);
			if (mm_xs)
				break;
		}

		for (; i < count; ++i)
		{
			auto x = splitCoordinates.xs[i];
			if (x <= r.hx)
				return i;
		}

		return count;
	}

	static int get_next_idx_lyhy(const Ranks& points, const SplitCoordinates& splitCoordinates, Rect rect, int idx)
	{
		auto count = (int)points.size();

		Rect r = rect;
		auto i = idx;

		for (; i < count && (i & 3); ++i)
		{
			auto y = splitCoordinates.ys[i];
			if (y >= r.ly && y <= r.hy)
				return i;
		}

		auto ptr = &splitCoordinates.ys[0];
		auto rly = _mm_set1_ps(r.ly);
		auto rhy = _mm_set1_ps(r.hy);
		for (; i < count - 4; i += 4)
		{
			auto ys = _mm_load_ps(ptr + i);
			auto rly_check = _mm_cmpge_ps(ys, rly);
			auto rhy_check = _mm_cmple_ps(ys, rhy);
			if (_mm_movemask_ps(rly_check) & _mm_movemask_ps(rhy_check))
				break;
		}

		for (; i < count; ++i)
		{
			auto y = splitCoordinates.ys[i];
			if (y >= r.ly && y <= r.hy)
			{
				return i;
			}
		}

		return count;
	}

	static int get_next_idx_ly(const Ranks& points, const SplitCoordinates& splitCoordinates, Rect rect, int idx)
	{
		auto count = (int)points.size();

		Rect r = rect;
		auto i = idx;

		for (; i < count && (i & 3); ++i)
		{
			auto y = splitCoordinates.ys[i];
			if (y >= r.ly)
				return i;
		}

		auto ptr = &splitCoordinates.ys[0];
		auto rly = _mm_set1_ps(r.ly);
		for (; i < count - 4; i += 4)
		{
			auto ys = _mm_load_ps(ptr + i);
			auto rly_check = _mm_cmpge_ps(ys, rly);
			if (_mm_movemask_ps(rly_check))
				break;
		}

		for (; i < count; ++i)
		{
			auto y = splitCoordinates.ys[i];
			if (y >= r.ly)
			{
				return i;
			}
		}

		return count;
	}

	static int get_next_idx_hy(const Ranks& points, const SplitCoordinates& splitCoordinates, Rect rect, int idx)
	{
		auto count = (int)points.size();

		Rect r = rect;
		auto i = idx;

		for (; i < count && (i & 3); ++i)
		{
			auto y = splitCoordinates.ys[i];
			if (y <= r.hy)
				return i;
		}

		auto ptr = &splitCoordinates.ys[0];
		auto rhy = _mm_set1_ps(r.hy);
		for (; i < count - 4; i += 4)
		{
			auto ys = _mm_load_ps(ptr + i);
			auto rhy_check = _mm_cmple_ps(ys, rhy);
			if (_mm_movemask_ps(rhy_check))
				break;
		}

		for (; i < count; ++i)
		{
			auto y = splitCoordinates.ys[i];
			if (y <= r.hy)
			{
				return i;
			}
		}

		return count;
	}

	static int get_next_idx_(const Ranks& points, const SplitCoordinates& splitCoordinates, Rect rect, int idx)
	{
		return idx;
	}

public:
	typedef int(*FunctionDefinition)(const Ranks& points, const SplitCoordinates& splitCoordinates, Rect rect, int idx);

	static FunctionDefinition get_next_idx(bool check_lx, bool check_hx, bool check_ly, bool check_hy)
	{
		if (check_lx)
		{
			if (check_hx)
			{
				if (check_ly)
				{
					if (check_hy)
						return get_next_idx_lxhxlyhy;
					else
						return get_next_idx_lxhxly;
				}
				else
				{
					if (check_hy)
						return get_next_idx_lxhxhy;
					else
						return get_next_idx_lxhx;
				}
			}
			else
			{
				if (check_ly)
				{
					if (check_hy)
						return get_next_idx_lxlyhy;
					else
						return get_next_idx_lxly;
				}
				else
				{
					if (check_hy)
						return get_next_idx_lxhy;
					else
						return get_next_idx_lx;
				}
			}
		}
		else
		{
			if (check_hx)
			{
				if (check_ly)
				{
					if (check_hy)
						return get_next_idx_hxlyhy;
					else
						return get_next_idx_hxly;
				}
				else
				{
					if (check_hy)
						return get_next_idx_hxhy;
					else
						return get_next_idx_hx;
				}
			}
			else
			{
				if (check_ly)
				{
					if (check_hy)
						return get_next_idx_lyhy;
					else
						return get_next_idx_ly;
				}
				else
				{
					if (check_hy)
						return get_next_idx_hy;
					else
						return get_next_idx_;
				}
			}
		}
	}

};

struct SimpleSplitIterator
	: PointIterator
{
	const SplitCoordinates& splitCoordinates;
	const Ranks& points;

	Rect rect;
	int idx;

	GetNextIdxSplit::FunctionDefinition fn_get_next_idx;

	SimpleSplitIterator(const SplitCoordinates& splitCoordinates, const Ranks& points)
		: points(points)
		, splitCoordinates(splitCoordinates)
	{}

	void reset(Rect surface_area, const Rect& rect)
	{
		this->rect = rect;

		auto check_lx = surface_area.lx <= rect.lx;
		auto check_hx = surface_area.hx >= rect.hx;
		auto check_ly = surface_area.ly <= rect.ly;
		auto check_hy = surface_area.hy >= rect.hy;

		fn_get_next_idx = GetNextIdxSplit::get_next_idx(check_lx, check_hx, check_ly, check_hy);

		idx = -1;
	}

	int32_t best_next()
	{
		if (idx + 1 >= points.size())
			return numeric_limits<int>::max();

		return points[idx + 1];
	}

	int32_t iterate()
	{
		if (idx == points.size())
			return -1;
		idx = fn_get_next_idx(points, splitCoordinates, rect, idx + 1);
		if (idx == points.size())
			return -1;

		return points[idx];
	}
};




















struct CompositeIterator
	: PointIterator
{
	PointIterator* a;
	PointIterator* b;

	int32_t aValue;
	int32_t bValue;

	int32_t a_best_next;
	int32_t b_best_next;

	void reset(PointIterator* a, PointIterator* b)
	{
		this->a = a;
		this->b = b;

		aValue = numeric_limits<int>::max();
		bValue = numeric_limits<int>::max();

		a_best_next = a->best_next();
		b_best_next = b->best_next();
	}

	int32_t best_next()
	{
		return min(aValue, min(bValue, min(a_best_next, b_best_next)));
	}

	int32_t iterate()
	{
	tryagain:
		if (aValue == -1)
		{
			if (bValue == numeric_limits<int>::max())
				return b->iterate();
			auto result = bValue;
			bValue = numeric_limits<int>::max();
			return result;
		}
		if (bValue == -1)
		{
			if (aValue == numeric_limits<int>::max())
				return a->iterate();
			auto result = aValue;
			aValue = numeric_limits<int>::max();
			return result;
		}

		if (aValue == numeric_limits<int>::max())
		{
			if (bValue == numeric_limits<int>::max())
			{
				if (a_best_next < b_best_next)
				{
					aValue = a->iterate();
					a_best_next = a->best_next();
					goto tryagain;
				}
				else
				{
					bValue = b->iterate();
					b_best_next = b->best_next();
					goto tryagain;
				}
			}
			else if (bValue < a_best_next)
			{
				auto result = bValue;
				bValue = numeric_limits<int>::max();
				return result;
			}
			else
			{
				aValue = a->iterate();
				a_best_next = a->best_next();
				goto tryagain;
			}
		}
		else if (bValue == numeric_limits<int>::max())
		{
			if (aValue < b_best_next)
			{
				auto result = aValue;
				aValue = numeric_limits<int>::max();
				return result;
			}
			else
			{
				bValue = b->iterate();
				b_best_next = b->best_next();
				goto tryagain;
			}
		}
		else if (aValue < bValue)
		{
			auto result = aValue;
			aValue = numeric_limits<int>::max();
			return result;
		}
		else
		{
			auto result = bValue;
			bValue = numeric_limits<int>::max();
			return result;
		}
	}
};

struct Surface
{
	const int depth;
	Rect area;

	Surface(int depth)
		: depth(depth)
	{}

	virtual PointIterator* get_iterator(const Rect& rect) = 0;
};


struct SimpleSurface
	: Surface
{
	Ranks points;
	SplitCoordinates coordinates;

	AllIterator all_points_iterator;
	SimpleSplitIterator simple_iterator;

	SimpleSurface(const Mappings& mappings, Ranks& points, int depth)
		: Surface(depth)
		, all_points_iterator(this->points)
		, simple_iterator(this->coordinates, this->points)
	{
		if (points.size() > MinimumSizeToUseMappingSort)
			SortByRank(mappings.points_count, points);
		else
			concurrency::parallel_sort(points.begin(), points.end(), [](int32_t a, int32_t b){ return a < b; });

		area.lx = numeric_limits<float>::max();
		area.hx = -numeric_limits<float>::max();
		area.ly = numeric_limits<float>::max();
		area.hy = -numeric_limits<float>::max();
		for (auto const r : points)
		{
			const auto& pt = mappings.coordinates[r];
			area.lx = min(area.lx, pt.x);
			area.hx = max(area.hx, pt.x);
			area.ly = min(area.ly, pt.y);
			area.hy = max(area.hy, pt.y);
		}

		auto count = points.size();
		coordinates.xs.resize(count);
		coordinates.ys.resize(count);
		for (auto i = 0; i < count; ++i)
		{
			coordinates.xs[i] = mappings.coordinates[points[i]].x;
			coordinates.ys[i] = mappings.coordinates[points[i]].y;
		}

		points.shrink_to_fit();
		this->points.swap(points);
	}

	PointIterator* get_iterator(const Rect& rect)
	{
		if (rect.lx > area.hx || rect.hx < area.lx || rect.ly > area.hy || rect.hy < area.ly)
			return &(null_iterator);

		if (rect.lx <= area.lx && rect.hx >= area.hx && rect.ly <= area.ly && rect.hy >= area.hy)
		{
			all_points_iterator.reset();
			return &(all_points_iterator);
		}

		simple_iterator.reset(area, rect);
		return &(simple_iterator);
	}
};

struct CompositeSurface
	: Surface
{
	SurfacePtr lower;
	SurfacePtr upper;
	Ranks rank_sorted_points;

	AllIterator all_iterator;
	SimpleIterator simple_iterator;
	CompositeIterator composite_iterator;

	CompositeSurface(const Mappings& mappings, const Ranks& points, int depth)
		: Surface(depth)
		, rank_sorted_points(points)
		, simple_iterator(mappings.coordinates, rank_sorted_points)
		, all_iterator(rank_sorted_points)
	{
		// TODO: Do something better about memory
		if (depth == 4)
		{
			rank_sorted_points.clear();
			rank_sorted_points.shrink_to_fit();
		}

		if (rank_sorted_points.size() > MinimumSizeToUseMappingSort)
			SortByRank(mappings.points_count, rank_sorted_points);
		else
			sort(rank_sorted_points.begin(), rank_sorted_points.end(), [](int32_t a, int32_t b){ return a < b; });
	}

	void split(const Mappings& mappings, Ranks& points, int midPoint)
	{
		if (midPoint == 0)
		{
			lower = SurfacePtr(new SimpleSurface(mappings, points, depth + 1));
			upper = SurfacePtr(new SimpleSurface(mappings, Ranks(), depth + 1));
			return;
		}

		auto upper_points = Ranks(points.size() - midPoint);
		copy(points.begin() + midPoint, points.end(), upper_points.begin());

		auto lower_points = Ranks();
		points.resize(midPoint);
		points.shrink_to_fit();
		points.swap(lower_points);

		auto make_lower = [&](){return surface_factory(mappings, lower_points, depth + 1); };
		auto make_upper = [&](){return surface_factory(mappings, upper_points, depth + 1); };

		if (depth < 2)
		{
			auto task_lower = concurrency::create_task(make_lower);
			auto task_upper = concurrency::create_task(make_upper);

			lower = task_lower.get();
			upper = task_upper.get();
		}
		else
		{
			lower = make_lower();
			upper = make_upper();
		}
	}

	virtual PointIterator* get_iterator_directional(const Rect& rect) = 0;

	PointIterator* get_iterator(const Rect& rect)
	{
		if (rect.lx > area.hx || rect.hx < area.lx || rect.ly > area.hy || rect.hy < area.ly)
			return &(null_iterator);

		// TODO: better memory management
		if (depth != 4)
		{
			if (rect.lx <= area.lx && rect.hx >= area.hx && rect.ly <= area.ly && rect.hy >= area.hy)
			{
				all_iterator.reset();
				return &(all_iterator);
			}

			if (depth < (MaxDepth - 1))
			{
				auto common_area = (min(rect.hx, area.hx) - max(rect.lx, area.lx)) * ((min(rect.hy, area.hy) - max(rect.ly, area.ly)));
				auto area_area = (area.hx - area.lx) * (area.hy - area.ly);
				if (common_area / area_area > 0.2)
				{
					simple_iterator.reset(area, rect);
					return &(simple_iterator);
				}
			}

			auto directional_iterator = get_iterator_directional(rect);
			if (directional_iterator)
				return directional_iterator;
		}

		auto lower_iterator = lower->get_iterator(rect);
		auto upper_iterator = upper->get_iterator(rect);
		if (lower_iterator == &(null_iterator))
			return upper_iterator;
		if (upper_iterator == &(null_iterator))
			return lower_iterator;
		composite_iterator.reset(lower_iterator, upper_iterator);
		return &(composite_iterator);
	}
};

struct CompositeSurfaceX
	: CompositeSurface
{
	CompositeSurfaceX(const Mappings& mappings, Ranks& points, int depth)
		: CompositeSurface(mappings, points, depth)
	{
		if (points.size() > MinimumSizeToUseMappingSort)
			SortByCoordinate(mappings.points_count, points, mappings.rank_idx_to_x_sorted_idxes, mappings.x_sorted_rank_idxes);
		else
			sort(points.begin(), points.end(), [&mappings](int32_t a, const int32_t b){ return mappings.coordinates[a].x < mappings.coordinates[b].x; });

		area.lx = mappings.coordinates[points.front()].x;
		area.hx = mappings.coordinates[points.back()].x;

		area.ly = numeric_limits<float>::max();
		area.hy = -numeric_limits<float>::max();
		for (auto const r : points)
		{
			const auto& pt = mappings.coordinates[r];
			area.ly = min(area.ly, pt.y);
			area.hy = max(area.hy, pt.y);
		}


		auto max_gap = -numeric_limits<float>::max();
		auto max_gap_idx = -1;
		for (auto i = 1; i < points.size() - 1; ++i)
		{
			auto n1 = mappings.coordinates[points[i - 1]].x;
			auto n2 = mappings.coordinates[points[i]].x;
			if (abs(n1) >= 1e10 || abs(n2) >= 1e10)
				continue;

			auto gap = n2 - n1;
			if (gap > max_gap)
			{
				max_gap = gap;
				max_gap_idx = i;
			}
		}

		auto area_lx = (abs(area.lx) >= 1e10) ? (mappings.coordinates[points[1]].x) : area.lx;
		auto area_hx = (abs(area.hx) >= 1e10) ? (mappings.coordinates[points[points.size() - 2]].x) : area.hx;

		auto midPoint = 0;
		auto gap_fraction = (max_gap / (area_hx - area_lx));
		if (gap_fraction > MaxGapFraction)
			midPoint = max_gap_idx;
		else
		{
			midPoint = (int)points.size() / 2;
			while (midPoint > 0 && mappings.coordinates[points[midPoint]].x == mappings.coordinates[points[midPoint + 1]].x)
				midPoint--;
		}

		split(mappings, points, midPoint);
	}

	PointIterator* get_iterator_directional(const Rect& rect)
	{
		if (rect.lx > lower->area.hx)
			return upper->get_iterator(rect);

		if (rect.hx < upper->area.lx)
			return lower->get_iterator(rect);

		return nullptr;
	}
};

struct CompositeSurfaceY
	: CompositeSurface
{
	CompositeSurfaceY(const Mappings& mappings, Ranks& points, int depth)
		: CompositeSurface(mappings, points, depth)
	{
		if (points.size() > MinimumSizeToUseMappingSort)
			SortByCoordinate(mappings.points_count, points, mappings.rank_idx_to_y_sorted_idxes, mappings.y_sorted_rank_idxes);
		else
			sort(points.begin(), points.end(), [&mappings](int32_t a, const int32_t b){ return mappings.coordinates[a].y < mappings.coordinates[b].y; });

		area.ly = mappings.coordinates[points.front()].y;
		area.hy = mappings.coordinates[points.back()].y;

		area.lx = numeric_limits<float>::max();
		area.hx = -numeric_limits<float>::max();
		for (auto const r : points)
		{
			auto pt = mappings.coordinates[r];
			area.lx = min(area.lx, pt.x);
			area.hx = max(area.hx, pt.x);
		}

		auto max_gap = -numeric_limits<float>::max();
		auto max_gap_idx = -1;
		for (auto i = 1; i < points.size() - 1; ++i)
		{
			auto n1 = mappings.coordinates[points[i - 1]].y;
			auto n2 = mappings.coordinates[points[i]].y;
			if (abs(n1) >= 1e10 || abs(n2) >= 1e10)
				continue;

			auto gap = n2 - n1;
			if (gap > max_gap)
			{
				max_gap = gap;
				max_gap_idx = i;
			}
		}

		auto area_ly = (abs(area.ly) >= 1e10) ? (mappings.coordinates[points[1]].y) : area.ly;
		auto area_hy = (abs(area.hy) >= 1e10) ? (mappings.coordinates[points[points.size() - 2]].y) : area.hy;

		auto midPoint = 0;
		auto gap_fraction = (max_gap / (area_hy - area_ly));
		if (gap_fraction > MaxGapFraction)
			midPoint = max_gap_idx;
		else
		{
			midPoint = (int)points.size() / 2;
			while (midPoint > 0 && mappings.coordinates[points[midPoint]].x == mappings.coordinates[points[midPoint + 1]].x)
				midPoint--;
		}


		split(mappings, points, midPoint);
	}

	PointIterator* get_iterator_directional(const Rect& rect)
	{
		if (rect.ly > lower->area.hy)
			return upper->get_iterator(rect);

		if (rect.hy < upper->area.ly)
			return lower->get_iterator(rect);

		return nullptr;
	}
};


SurfacePtr surface_factory(const Mappings& mappings, Ranks& points, int depth)
{
	if (points.size() < 20 || depth == MaxDepth)
		return SurfacePtr(new SimpleSurface(mappings, points, depth));

	if (depth % 2 == 0)
		return SurfacePtr(new CompositeSurfaceX(mappings, points, depth));

	return SurfacePtr(new CompositeSurfaceY(mappings, points, depth));
}

struct SearchContext
{
	SurfacePtr top;
	Coordinates coordinates;
	Ids ids;

	SearchContext(const Point* points_begin, const Point* points_end)
	{
		auto start_time = chrono::system_clock::now();

		auto objects = points_end - points_begin;
		if (objects > std::numeric_limits<int32_t>::max())
			return;

		auto points_count = (int32_t)objects;
		coordinates.resize(points_count);
		ids.resize(points_count);

		auto ranks = Ranks(points_count, -1);
		auto points = vector<Point>(points_count);
		for (auto it = points_begin; it != points_end; ++it)
		{
			auto idx = it->rank;
			if (idx < 0 || idx >= points_count || ranks[idx] != -1)
				return;
			coordinates[idx] = Coordinate(it->x, it->y);
			ids[idx] = it->id;
			ranks[idx] = idx;
			points[idx] = *it;
		}
		Mappings mappings(points_count, coordinates, ids);

		concurrency::parallel_sort(points.begin(), points.end(), [](const Point& a, const Point& b) { return a.x < b.x; });
		for (auto i = 0; i < points_count; ++i)
		{
			mappings.x_sorted_rank_idxes[i] = points[i].rank;
			mappings.rank_idx_to_x_sorted_idxes[points[i].rank] = i;
		}

		concurrency::parallel_sort(points.begin(), points.end(), [](const Point& a, const Point& b) { return a.y < b.y; });
		for (auto i = 0; i < points_count; ++i)
		{
			mappings.y_sorted_rank_idxes[i] = points[i].rank;
			mappings.rank_idx_to_y_sorted_idxes[points[i].rank] = i;
		}

		top = surface_factory(mappings, ranks, 0);
	}

	void create_pt(int32_t r, Point* pt) const
	{
		pt->id = ids[r];
		pt->rank = r;
		pt->x = coordinates[r].x;
		pt->y = coordinates[r].y;
	}

	int32_t search(const Rect& rect, const int32_t count, Point* out_points) const
	{
		auto found_count = 0;
		auto iterator = top->get_iterator(rect);

		for (auto p = iterator->iterate(); p != -1; p = iterator->iterate())
		{
			create_pt(p, out_points++);
			if (++found_count == count)
				break;
		}
		return found_count;
	}
};

extern "C" __declspec(dllexport) SearchContext* __stdcall create(const Point* points_begin, const Point* points_end)
{
	return new SearchContext(points_begin, points_end);
}

extern "C" __declspec(dllexport) int32_t __stdcall search(SearchContext* sc, const Rect rect, const int32_t count, Point* out_points)
{
	return sc->search(rect, count, out_points);
}

extern "C" __declspec(dllexport) SearchContext* __stdcall destroy(SearchContext* sc)
{
	delete sc;
	return nullptr;
}

