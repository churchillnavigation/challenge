#ifndef ITERATORS_H
#define ITERATORS_H

#include <limits>
#include <algorithm>
#include <climits>

#include "common.h"
#include "get_next_idx.h"
#include "get_next_idx_iterator.h"
#include "get_next_idx_sse2.h"
#include "get_next_idx_avx2.h"
#include "rank_iterators.h"

struct NullIterator
	: PointIterator
{
	NullIterator()
	{
		this->best_next = std::numeric_limits<int>::max();
	}
	int32_t iterate() { return END_OF_DATA; }
};

NullIterator null_iterator;

struct SimpleRankIterator
	: PointIterator
{
	const Coordinates& coordinates;
	std::shared_ptr<RanksIterator> ranks_iterator;

	Rect rect;

	GetNextIdxIterator::FunctionDefinition fn_get_next_idx;

	SimpleRankIterator(const Coordinates& coordinates, std::shared_ptr<RanksIterator> ranks_iterator)
		: ranks_iterator(ranks_iterator)
		, coordinates(coordinates)
	{}

	void reset(const Rect& rect, RectChecks checks)
	{
		ranks_iterator->reset();
		this->rect = rect;
		fn_get_next_idx = GetNextIdxIterator::get_next_idx(checks);
		this->best_next = ranks_iterator->best_next;
	}

	int32_t iterate()
	{
		return fn_get_next_idx(*ranks_iterator, coordinates, rect);
		this->best_next = ranks_iterator->best_next;
	}
};


/*
struct SimpleIterator
	: PointIterator
{
	const Coordinates& coordinates;
	const Ranks& points;

	Rect rect;
	int idx;

	GetNextIdx::FunctionDefinition fn_get_next_idx;

	int max_index_check;

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

		max_index_check = (int) points.size()-1;
 
		idx = -1;
	}

	int32_t best_next()
	{
		if (idx >= max_index_check)
			return std::numeric_limits<int>::max();

		return points[idx + 1];
	}

	int32_t iterate()
	{
		if (idx == points.size())
			return END_OF_DATA;
		idx = fn_get_next_idx(points, coordinates, rect, idx + 1);
		if (idx == points.size())
			return END_OF_DATA;

		return points[idx];
	}
};
*/

struct SimpleIteratorSSE2
	: PointIterator
{
	const Coordinates& coordinates;
	const Ranks& points;

	Rect rect;
	int idx;

	GetNextIdxSSE2::FunctionDefinition fn_get_next_idx;
	int max_index_check;

	int32_t best_next_reset;

	SimpleIteratorSSE2(const Coordinates& coordinates, const Ranks& points)
		: points(points)
		, coordinates(coordinates)
	{}

	void init()
	{
		max_index_check = (int)points.size() - 1;
		idx = -1;
		best_next_reset = get_best_next();
	}

	void reset(const Rect& rect, RectChecks checks)
	{
		this->rect = rect;
		fn_get_next_idx = GetNextIdxSSE2::get_next_idx(checks);
		idx = -1;
		best_next = best_next_reset;
	}

	int32_t get_best_next()
	{
		if (idx >= max_index_check)
			return std::numeric_limits<int>::max();

		return points[idx + 1];
	}

	int32_t iterate()
	{
		if (idx == points.size())
			return END_OF_DATA;
		idx = fn_get_next_idx(points, coordinates, rect, idx + 1);
		best_next = get_best_next();
		if (idx == points.size())
			return END_OF_DATA;
		return points[idx];
	}
};


struct SimpleIteratorAVX2
	: PointIterator
{
	const Coordinates& coordinates;
	const Ranks& points;

	Rect rect;
	int idx;

	GetNextIdxAVX2::FunctionDefinition fn_get_next_idx;
	int max_index_check;

	int32_t best_next_reset;

	SimpleIteratorAVX2(const Coordinates& coordinates, const Ranks& points)
		: points(points)
		, coordinates(coordinates)
	{}

	void init()
	{
		max_index_check = (int)points.size() - 1;
		best_next_reset = get_best_next();
	}

	void reset(const Rect& rect, RectChecks checks)
	{
		this->rect = rect;
		fn_get_next_idx = GetNextIdxAVX2::get_next_idx(checks);
		idx = -1;
		best_next = best_next_reset;
	}

	int32_t get_best_next()
	{
		if (idx >= max_index_check)
			return std::numeric_limits<int>::max();

		return points[idx + 1];
	}

	int32_t iterate()
	{
		if (idx == points.size())
			return END_OF_DATA;
		idx = fn_get_next_idx(points, coordinates, rect, idx + 1);
		best_next = get_best_next();
		if (idx == points.size())
			return END_OF_DATA;

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

	enum States
	{
		a_best_next__b_best_next,
		b_best_next__a_best_next,
		aValue__a_best_next__b_best_next,
		aValue__b_best_next__a_best_next,
		b_best_next__aValue__a_best_next,
		bValue__a_best_next__b_best_next,
		bValue__b_best_next__a_best_next,
		a_best_next__bValue__b_best_next,
	};

	States state;

	void reset(PointIterator* a, PointIterator* b)
	{
		this->a = a;
		this->b = b;

		aValue = UNKNOWN_VALUE;
		bValue = UNKNOWN_VALUE;

		a_best_next = a->best_next;
		b_best_next = b->best_next;

		if (a_best_next < b_best_next)
		{
			state = a_best_next__b_best_next;
			best_next = a_best_next;
		}
		else
		{
			state = b_best_next__a_best_next;
			best_next = b_best_next;
		}
	}

	int32_t iterate()
	{
	tryagain:
		switch (state)
		{
		case CompositeIterator::a_best_next__b_best_next:
			aValue = a->iterate();
			a_best_next = a->best_next;
			if (aValue < b_best_next)
			{
				if (a_best_next < b_best_next)
				{
					state = a_best_next__b_best_next;
					best_next = a_best_next;
					return aValue;
				}
				state = b_best_next__a_best_next;
				best_next = b_best_next;
				return aValue;
			}
			state = b_best_next__aValue__a_best_next;
			goto tryagain;

		case CompositeIterator::b_best_next__a_best_next:
			bValue = b->iterate();
			b_best_next = b->best_next;
			if (bValue < a_best_next)
			{
				if (b_best_next < a_best_next)
				{
					state = b_best_next__a_best_next;
					best_next = b_best_next;
					return bValue;
				}
				state = a_best_next__b_best_next;
				best_next = a_best_next;
				return bValue;
			}
			state = a_best_next__bValue__b_best_next;
			goto tryagain;

		case CompositeIterator::aValue__a_best_next__b_best_next:
			state = a_best_next__b_best_next;
			best_next = a_best_next;
			return aValue;

		case CompositeIterator::aValue__b_best_next__a_best_next:
			state = b_best_next__a_best_next;
			best_next = b_best_next;
			return aValue;

		case CompositeIterator::b_best_next__aValue__a_best_next:
			bValue = b->iterate();
			b_best_next = b->best_next;
			if (bValue < aValue)
			{
				if (b_best_next < aValue)
				{
					best_next = b_best_next;
					return bValue;
				}
				if (b_best_next < a_best_next)
				{
					state = aValue__b_best_next__a_best_next;
					best_next = aValue;
					return bValue;
				}
				state = aValue__a_best_next__b_best_next;
				best_next = aValue;
				return bValue;
			}
			if (bValue < a_best_next)
			{
				if (b_best_next < a_best_next)
				{
					state = bValue__b_best_next__a_best_next;
					best_next = bValue;
					return aValue;
				}
				state = bValue__a_best_next__b_best_next;
				best_next = bValue;
				return aValue;
			}
			state = a_best_next__bValue__b_best_next;
			best_next = a_best_next;
			return aValue;

		case CompositeIterator::bValue__a_best_next__b_best_next:
			state = a_best_next__b_best_next;
			best_next = a_best_next;
			return bValue;

		case CompositeIterator::bValue__b_best_next__a_best_next:
			state = b_best_next__a_best_next;
			best_next = b_best_next;
			return bValue;

		case CompositeIterator::a_best_next__bValue__b_best_next:
			aValue = a->iterate();
			a_best_next = a->best_next;
			if (aValue < bValue)
			{
				if (a_best_next < bValue)
				{
					best_next = a_best_next;
					return aValue;
				}
				if (a_best_next < b_best_next)
				{
					state = bValue__a_best_next__b_best_next;
					best_next = bValue;
					return aValue;
				}
				state = bValue__b_best_next__a_best_next;
				best_next = bValue;
				return aValue;
			}
			if (aValue < b_best_next)
			{
				if (a_best_next < b_best_next)
				{
					state = aValue__a_best_next__b_best_next;
					best_next = aValue;
					return bValue;
				}
				state = aValue__b_best_next__a_best_next;
				best_next = aValue;
				return bValue;
			}
			state = b_best_next__aValue__a_best_next;
			best_next = b_best_next;
			return bValue;
		}

		throw "bad";
	}
};


#endif