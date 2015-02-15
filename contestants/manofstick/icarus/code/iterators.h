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
	int32_t best_next() { return std::numeric_limits<int>::max(); }
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

	void reset(Rect surface_area, const Rect& rect)
	{
		ranks_iterator->reset();

		this->rect = rect;

		auto checks = (RectChecks)(
			(surface_area.lx <= rect.lx) * 0x1 +
			(surface_area.hx >= rect.hx) * 0x2 +
			(surface_area.ly <= rect.ly) * 0x4 +
			(surface_area.hy >= rect.hy) * 0x8);

		fn_get_next_idx = GetNextIdxIterator::get_next_idx(checks);
	}

	int32_t best_next()
	{
		return ranks_iterator->best_next();
	}

	int32_t iterate()
	{
		return fn_get_next_idx(*ranks_iterator, coordinates, rect);
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

	SimpleIteratorSSE2(const Coordinates& coordinates, const Ranks& points)
		: points(points)
		, coordinates(coordinates)
	{}

	void reset(Rect surface_area, const Rect& rect)
	{
		this->rect = rect;

		auto checks = (RectChecks)(
			(surface_area.lx <= rect.lx) * 0x1 +
			(surface_area.hx >= rect.hx) * 0x2 +
			(surface_area.ly <= rect.ly) * 0x4 +
			(surface_area.hy >= rect.hy) * 0x8);

		fn_get_next_idx = GetNextIdxSSE2::get_next_idx(checks);

		max_index_check = (int)points.size() - 1;

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


struct SimpleIteratorAVX2
	: PointIterator
{
	const Coordinates& coordinates;
	const Ranks& points;

	Rect rect;
	int idx;

	GetNextIdxAVX2::FunctionDefinition fn_get_next_idx;
	int max_index_check;

	SimpleIteratorAVX2(const Coordinates& coordinates, const Ranks& points)
		: points(points)
		, coordinates(coordinates)
	{}

	void reset(Rect surface_area, const Rect& rect)
	{
		this->rect = rect;

		auto checks = (RectChecks)(
			(surface_area.lx <= rect.lx) * 0x1 +
			(surface_area.hx >= rect.hx) * 0x2 +
			(surface_area.ly <= rect.ly) * 0x4 +
			(surface_area.hy >= rect.hy) * 0x8);

		fn_get_next_idx = GetNextIdxAVX2::get_next_idx(checks);

		max_index_check = (int)points.size() - 1;

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

		aValue = UNKNOWN_VALUE;
		bValue = UNKNOWN_VALUE;

		a_best_next = a->best_next();
		b_best_next = b->best_next();
	}

	int32_t best_next()
	{
		return std::min(aValue, std::min(bValue, std::min(a_best_next, b_best_next)));
	}

	int32_t iterate()
	{
	tryagain:
		if (aValue == END_OF_DATA)
		{
			if (bValue == UNKNOWN_VALUE)
				return b->iterate();
			auto result = bValue;
			bValue = UNKNOWN_VALUE;
			return result;
		}
		
		if (bValue == END_OF_DATA)
		{
			if (aValue == UNKNOWN_VALUE)
				return a->iterate();
			auto result = aValue;
			aValue = UNKNOWN_VALUE;
			return result;
		}

		if (aValue == UNKNOWN_VALUE)
		{
			if (bValue == UNKNOWN_VALUE)
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
				bValue = UNKNOWN_VALUE;
				return result;
			}
			else
			{
				aValue = a->iterate();
				a_best_next = a->best_next();
				goto tryagain;
			}
		}
		else if (bValue == UNKNOWN_VALUE)
		{
			if (aValue < b_best_next)
			{
				auto result = aValue;
				aValue = UNKNOWN_VALUE;
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
			aValue = UNKNOWN_VALUE;
			return result;
		}
		else
		{
			auto result = bValue;
			bValue = UNKNOWN_VALUE;
			return result;
		}
	}
};


#endif