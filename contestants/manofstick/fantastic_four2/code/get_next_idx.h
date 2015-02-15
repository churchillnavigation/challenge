#ifndef GET_NEXT_IDX_H
#define GET_NEXT_IDX_H

#include "common.h"

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
			auto ys = _mm_set_ps(coordinates.ys[points[i]], coordinates.ys[points[i + 1]], coordinates.ys[points[i + 2]], coordinates.ys[points[i + 3]]);
			auto rly_check = _mm_cmpge_ps(ys, rly);
			auto rhy_check = _mm_cmple_ps(ys, rhy);
			auto mm_ys = _mm_movemask_ps(rly_check) & _mm_movemask_ps(rhy_check);
			if (!mm_ys)
				continue;

			auto xs = _mm_set_ps(coordinates.xs[points[i]], coordinates.xs[points[i + 1]], coordinates.xs[points[i + 2]], coordinates.xs[points[i + 3]]);
			auto rlx_check = _mm_cmpge_ps(xs, rlx);
			auto rhx_check = _mm_cmple_ps(xs, rhx);
			auto mm_xs = _mm_movemask_ps(rlx_check) & _mm_movemask_ps(rhx_check);
			if (mm_ys & mm_xs)
				break;
		}

		for (; i < count; ++i)
		{
			auto rank = points[i];
			auto x = coordinates.xs[rank];
			auto y = coordinates.ys[rank];
			if ((y >= r.ly && y <= r.hy && x >= r.lx && x <= r.hx))
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
			auto rank = points[i];
			auto x = coordinates.xs[rank];
			auto y = coordinates.ys[rank];
			if ((y >= r.ly && x >= r.lx && x <= r.hx))
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
			auto rank = points[i];
			auto x = coordinates.xs[rank];
			auto y = coordinates.ys[rank];
			if ((y <= r.hy && x >= r.lx && x <= r.hx))
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
			auto rank = points[i];
			auto x = coordinates.xs[rank];
			if ((x >= r.lx && x <= r.hx))
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
			auto rank = points[i];
			auto x = coordinates.xs[rank];
			auto y = coordinates.ys[rank];
			if ((y >= r.ly && y <= r.hy && x >= r.lx))
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
			auto rank = points[i];
			auto x = coordinates.xs[rank];
			auto y = coordinates.ys[rank];
			if ((y >= r.ly && x >= r.lx))
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
			auto rank = points[i];
			auto x = coordinates.xs[rank];
			auto y = coordinates.ys[rank];
			if ((y <= r.hy && x >= r.lx))
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
			auto rank = points[i];
			auto x = coordinates.xs[rank];
			if ((x >= r.lx))
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
			auto rank = points[i];
			auto x = coordinates.xs[rank];
			auto y = coordinates.ys[rank];
			if ((y >= r.ly && y <= r.hy && x <= r.hx))
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
			auto rank = points[i];
			auto x = coordinates.xs[rank];
			auto y = coordinates.ys[rank];
			if ((y >= r.ly && x <= r.hx))
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
			auto rank = points[i];
			auto x = coordinates.xs[rank];
			auto y = coordinates.ys[rank];
			if ((y <= r.hy && x <= r.hx))
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
			auto rank = points[i];
			auto x = coordinates.xs[rank];
			if ((x <= r.hx))
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
			auto rank = points[i];
			auto y = coordinates.ys[rank];
			if ((y >= r.ly && y <= r.hy))
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
			auto rank = points[i];
			auto y = coordinates.ys[rank];
			if ((y >= r.ly))
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
			auto rank = points[i];
			auto y = coordinates.ys[rank];
			if ((y <= r.hy))
				return i;
		}

		return count;
	}

public:
	typedef int(*FunctionDefinition)(const Ranks& points, const Coordinates& coordinates, Rect rect, int idx);

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