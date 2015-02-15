#ifndef GET_NEXT_IDX_ITERATOR_H
#define GET_NEXT_IDX_ITERATOR_H

#include "common.h"

class GetNextIdxIterator
{
	static int Aget_next_idx_lxhxlyhy(PointIterator& ranks, const Coordinates& cs, Rect rect)
	{
		Rect r = rect;
		for (auto rank = ranks.iterate(); rank != END_OF_DATA; rank = ranks.iterate())
		{
			if (cs.ys[rank] >= r.ly && cs.ys[rank] <= r.hy && cs.xs[rank] >= r.lx && cs.xs[rank] <= r.hx)
				return rank;
		}
		return END_OF_DATA;
	}

	static int Aget_next_idx_lxhxly(PointIterator& ranks, const Coordinates& cs, Rect rect)
	{
		Rect r = rect;
		for (auto rank = ranks.iterate(); rank != END_OF_DATA; rank = ranks.iterate())
		{
			if (cs.ys[rank] >= r.ly && cs.xs[rank] >= r.lx && cs.xs[rank] <= r.hx)
				return rank;
		}
		return END_OF_DATA;
	}

	static int Aget_next_idx_lxhxhy(PointIterator& ranks, const Coordinates& cs, Rect rect)
	{
		Rect r = rect;
		for (auto rank = ranks.iterate(); rank != END_OF_DATA; rank = ranks.iterate())
		{
			if (cs.ys[rank] <= r.hy && cs.xs[rank] >= r.lx && cs.xs[rank] <= r.hx)
				return rank;
		}
		return END_OF_DATA;
	}

	static int Aget_next_idx_lxhx(PointIterator& ranks, const Coordinates& cs, Rect rect)
	{
		Rect r = rect;
		for (auto rank = ranks.iterate(); rank != END_OF_DATA; rank = ranks.iterate())
		{
			if (cs.xs[rank] >= r.lx && cs.xs[rank] <= r.hx)
				return rank;
		}
		return END_OF_DATA;
	}

	static int Aget_next_idx_lxlyhy(PointIterator& ranks, const Coordinates& cs, Rect rect)
	{
		Rect r = rect;
		for (auto rank = ranks.iterate(); rank != END_OF_DATA; rank = ranks.iterate())
		{
			if (cs.ys[rank] >= r.ly && cs.ys[rank] <= r.hy && cs.xs[rank] >= r.lx)
				return rank;
		}
		return END_OF_DATA;
	}

	static int Aget_next_idx_lxly(PointIterator& ranks, const Coordinates& cs, Rect rect)
	{
		Rect r = rect;
		for (auto rank = ranks.iterate(); rank != END_OF_DATA; rank = ranks.iterate())
		{
			if (cs.ys[rank] >= r.ly && cs.xs[rank] >= r.lx)
				return rank;
		}
		return END_OF_DATA;
	}

	static int Aget_next_idx_lxhy(PointIterator& ranks, const Coordinates& cs, Rect rect)
	{
		Rect r = rect;
		for (auto rank = ranks.iterate(); rank != END_OF_DATA; rank = ranks.iterate())
		{
			if (cs.ys[rank] <= r.hy && cs.xs[rank] >= r.lx)
				return rank;
		}
		return END_OF_DATA;
	}

	static int Aget_next_idx_lx(PointIterator& ranks, const Coordinates& cs, Rect rect)
	{
		Rect r = rect;
		for (auto rank = ranks.iterate(); rank != END_OF_DATA; rank = ranks.iterate())
		{
			if (cs.xs[rank] >= r.lx)
				return rank;
		}
		return END_OF_DATA;
	}

	static int Aget_next_idx_hxlyhy(PointIterator& ranks, const Coordinates& cs, Rect rect)
	{
		Rect r = rect;
		for (auto rank = ranks.iterate(); rank != END_OF_DATA; rank = ranks.iterate())
		{
			if (cs.ys[rank] >= r.ly && cs.ys[rank] <= r.hy && cs.xs[rank] <= r.hx)
				return rank;
		}
		return END_OF_DATA;
	}

	static int Aget_next_idx_hxly(PointIterator& ranks, const Coordinates& cs, Rect rect)
	{
		Rect r = rect;
		for (auto rank = ranks.iterate(); rank != END_OF_DATA; rank = ranks.iterate())
		{
			if (cs.ys[rank] >= r.ly && cs.xs[rank] <= r.hx)
				return rank;
		}
		return END_OF_DATA;
	}

	static int Aget_next_idx_hxhy(PointIterator& ranks, const Coordinates& cs, Rect rect)
	{
		Rect r = rect;
		for (auto rank = ranks.iterate(); rank != END_OF_DATA; rank = ranks.iterate())
		{
			if (cs.ys[rank] <= r.hy && cs.xs[rank] <= r.hx)
				return rank;
		}
		return END_OF_DATA;
	}

	static int Aget_next_idx_hx(PointIterator& ranks, const Coordinates& cs, Rect rect)
	{
		Rect r = rect;
		for (auto rank = ranks.iterate(); rank != END_OF_DATA; rank = ranks.iterate())
		{
			if (cs.xs[rank] <= r.hx)
				return rank;
		}
		return END_OF_DATA;
	}

	static int Aget_next_idx_lyhy(PointIterator& ranks, const Coordinates& cs, Rect rect)
	{
		Rect r = rect;
		for (auto rank = ranks.iterate(); rank != END_OF_DATA; rank = ranks.iterate())
		{
			if (cs.ys[rank] >= r.ly && cs.ys[rank] <= r.hy)
				return rank;
		}
		return END_OF_DATA;
	}

	static int Aget_next_idx_ly(PointIterator& ranks, const Coordinates& cs, Rect rect)
	{
		Rect r = rect;
		for (auto rank = ranks.iterate(); rank != END_OF_DATA; rank = ranks.iterate())
		{
			if (cs.ys[rank] >= r.ly)
				return rank;
		}
		return END_OF_DATA;
	}

	static int Aget_next_idx_hy(PointIterator& ranks, const Coordinates& cs, Rect rect)
	{
		Rect r = rect;
		for (auto rank = ranks.iterate(); rank != END_OF_DATA; rank = ranks.iterate())
		{
			if (cs.ys[rank] <= r.hy)
				return rank;
		}
		return END_OF_DATA;
	}

	static int Aget_next_idx_(PointIterator& ranks, const Coordinates& cs, Rect rect)
	{
		return ranks.iterate();
	}

public:
	typedef int(*FunctionDefinition)(PointIterator& ranks, const Coordinates& cs, Rect rect);

	static FunctionDefinition get_next_idx(RectChecks checks)
	{
		switch (checks)
		{
		case CheckLx | CheckHx | CheckLy | CheckHy: return Aget_next_idx_lxhxlyhy;
		case CheckLx | CheckHx | CheckHy:			return Aget_next_idx_lxhxhy;
		case CheckLx | CheckHx | CheckLy:			return Aget_next_idx_lxhxly;
		case CheckLx | CheckHx:						return Aget_next_idx_lxhx;
		case CheckLx | CheckLy | CheckHy:			return Aget_next_idx_lxlyhy;
		case CheckLx | CheckLy:						return Aget_next_idx_lxly;
		case CheckLx | CheckHy:						return Aget_next_idx_lxhy;
		case CheckLx:								return Aget_next_idx_lx;
		case CheckHx | CheckLy | CheckHy:			return Aget_next_idx_hxlyhy;
		case CheckHx | CheckLy:						return Aget_next_idx_hxly;
		case CheckHx | CheckHy:						return Aget_next_idx_hxhy;
		case CheckHx:								return Aget_next_idx_hx;
		case CheckLy | CheckHy:						return Aget_next_idx_lyhy;
		case CheckLy:								return Aget_next_idx_ly;
		case CheckHy:								return Aget_next_idx_hy;
		default:
			std::cout << std::endl << checks << std::endl;
			throw "Bad";
		}
	}

};

#endif