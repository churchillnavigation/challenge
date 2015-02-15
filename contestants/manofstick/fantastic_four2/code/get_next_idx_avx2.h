#ifndef GET_NEXT_IDX_AVX2
#define GET_NEXT_IDX_AVX2

#include "common.h"
#include <immintrin.h>

class GetNextIdxAVX2
{
	static int get_next_idx_lxhxlyhy(const Ranks& points, const Coordinates& coordinates, Rect rect, int idx)
	{
		auto count = (int)points.size();

		Rect r = rect;
		auto i = idx;

		for (; i < count && (i & 7); ++i)
		{
			auto y = coordinates.ys[i];
			if (y >= r.ly && y <= r.hy)
			{
				auto x = coordinates.xs[i];
				if (x >= r.lx && x <= r.hx)
					return i;
			}
		}

		auto ys_ptr = &coordinates.ys[0];
		auto xs_ptr = &coordinates.xs[0];
		auto rly = _mm256_set1_ps(r.ly);
		auto rhy = _mm256_set1_ps(r.hy);
		auto rlx = _mm256_set1_ps(r.lx);
		auto rhx = _mm256_set1_ps(r.hx);
		for (; i < count - 8; i += 8)
		{
			auto ys = _mm256_load_ps(ys_ptr + i);
			auto rly_check = _mm256_cmp_ps(ys, rly, _CMP_GE_OS);
			auto rhy_check = _mm256_cmp_ps(ys, rhy, _CMP_LE_OS);
			auto mm_ys = _mm256_movemask_ps(rly_check) & _mm256_movemask_ps(rhy_check);
			if (!mm_ys)
				continue;

			auto xs = _mm256_load_ps(xs_ptr + i);
			auto rlx_check = _mm256_cmp_ps(xs, rlx, _CMP_GE_OS);
			auto rhx_check = _mm256_cmp_ps(xs, rhx, _CMP_LE_OS);
			auto mm_xs = _mm256_movemask_ps(rlx_check) & _mm256_movemask_ps(rhx_check);
			if (mm_ys & mm_xs)
				break;
		}

		for (; i < count; ++i)
		{
			auto y = coordinates.ys[i];
			if (y >= r.ly && y <= r.hy)
			{
				auto x = coordinates.xs[i];
				if (x >= r.lx && x <= r.hx)
					return i;
			}
		}

		return count;
	}

	static int get_next_idx_lxhxly(const Ranks& points, const Coordinates& coordinates, Rect rect, int idx)
	{
		auto count = (int)points.size();

		Rect r = rect;
		auto i = idx;

		for (; i < count && (i & 7); ++i)
		{
			auto x = coordinates.xs[i];
			if (x >= r.lx && x <= r.hx)
			{
				auto y = coordinates.ys[i];
				if (y >= r.ly)
				{
					return i;
				}
			}
		}

		auto ys_ptr = &coordinates.ys[0];
		auto xs_ptr = &coordinates.xs[0];
		auto rly = _mm256_set1_ps(r.ly);
		auto rlx = _mm256_set1_ps(r.lx);
		auto rhx = _mm256_set1_ps(r.hx);
		for (; i < count - 8; i += 8)
		{
			auto xs = _mm256_load_ps(xs_ptr + i);
			auto rlx_check = _mm256_cmp_ps(xs, rlx, _CMP_GE_OS);
			auto rhx_check = _mm256_cmp_ps(xs, rhx, _CMP_LE_OS);
			auto mm_xs = _mm256_movemask_ps(rlx_check) & _mm256_movemask_ps(rhx_check);
			if (!mm_xs)
				continue;

			auto ys = _mm256_load_ps(ys_ptr + i);
			auto rly_check = _mm256_cmp_ps(ys, rly, _CMP_GE_OS);
			auto mm_ys = _mm256_movemask_ps(rly_check);
			if (mm_ys & mm_xs)
				break;
		}

		for (; i < count; ++i)
		{
			auto x = coordinates.xs[i];
			if (x >= r.lx && x <= r.hx)
			{
				auto y = coordinates.ys[i];
				if (y >= r.ly)
				{
					return i;
				}
			}
		}

		return count;
	}

	static int get_next_idx_lxhxhy(const Ranks& points, const Coordinates& coordinates, Rect rect, int idx)
	{
		auto count = (int)points.size();

		Rect r = rect;
		auto i = idx;

		for (; i < count && (i & 7); ++i)
		{
			auto x = coordinates.xs[i];
			if (x >= r.lx && x <= r.hx)
			{
				auto y = coordinates.ys[i];
				if (y <= r.hy)
					return i;
			}
		}

		auto ys_ptr = &coordinates.ys[0];
		auto xs_ptr = &coordinates.xs[0];
		auto rhy = _mm256_set1_ps(r.hy);
		auto rlx = _mm256_set1_ps(r.lx);
		auto rhx = _mm256_set1_ps(r.hx);
		for (; i < count - 8; i += 8)
		{
			auto xs = _mm256_load_ps(xs_ptr + i);
			auto rlx_check = _mm256_cmp_ps(xs, rlx, _CMP_GE_OS);
			auto rhx_check = _mm256_cmp_ps(xs, rhx, _CMP_LE_OS);
			auto mm_xs = _mm256_movemask_ps(rlx_check) & _mm256_movemask_ps(rhx_check);
			if (!mm_xs)
				continue;

			auto ys = _mm256_load_ps(ys_ptr + i);
			auto rhy_check = _mm256_cmp_ps(ys, rhy, _CMP_LE_OS);
			auto mm_ys = _mm256_movemask_ps(rhy_check);
			if (mm_ys & mm_xs)
				break;
		}

		for (; i < count; ++i)
		{
			auto x = coordinates.xs[i];
			if (x >= r.lx && x <= r.hx)
			{
				auto y = coordinates.ys[i];
				if (y <= r.hy)
					return i;
			}
		}

		return count;
	}

	static int get_next_idx_lxhx(const Ranks& points, const Coordinates& coordinates, Rect rect, int idx)
	{
		auto count = (int)points.size();

		Rect r = rect;
		auto i = idx;

		for (; i < count && (i & 7); ++i)
		{
			auto x = coordinates.xs[i];
			if (x >= r.lx && x <= r.hx)
				return i;
		}

		auto ptr = &coordinates.xs[0];
		auto rlx = _mm256_set1_ps(r.lx);
		auto rhx = _mm256_set1_ps(r.hx);
		for (; i < count - 8; i += 8)
		{
			auto xs = _mm256_load_ps(ptr + i);
			auto rlx_check = _mm256_cmp_ps(xs, rlx, _CMP_GE_OS);
			auto rhx_check = _mm256_cmp_ps(xs, rhx, _CMP_LE_OS);
			if (_mm256_movemask_ps(rlx_check) & _mm256_movemask_ps(rhx_check))
				break;
		}

		for (; i < count; ++i)
		{
			auto x = coordinates.xs[i];
			if (x >= r.lx && x <= r.hx)
			{
				return i;
			}
		}

		return count;
	}

	static int get_next_idx_lxlyhy(const Ranks& points, const Coordinates& coordinates, Rect rect, int idx)
	{
		auto count = (int)points.size();

		Rect r = rect;
		auto i = idx;

		for (; i < count && (i & 7); ++i)
		{
			auto y = coordinates.ys[i];
			if (y >= r.ly && y <= r.hy)
			{
				auto x = coordinates.xs[i];
				if (x >= r.lx)
					return i;
			}
		}

		auto ys_ptr = &coordinates.ys[0];
		auto xs_ptr = &coordinates.xs[0];
		auto rly = _mm256_set1_ps(r.ly);
		auto rhy = _mm256_set1_ps(r.hy);
		auto rlx = _mm256_set1_ps(r.lx);
		for (; i < count - 8; i += 8)
		{
			auto ys = _mm256_load_ps(ys_ptr + i);
			auto rly_check = _mm256_cmp_ps(ys, rly, _CMP_GE_OS);
			auto rhy_check = _mm256_cmp_ps(ys, rhy, _CMP_LE_OS);
			auto mm_ys = _mm256_movemask_ps(rly_check) & _mm256_movemask_ps(rhy_check);
			if (!mm_ys)
				continue;

			auto xs = _mm256_load_ps(xs_ptr + i);
			auto rlx_check = _mm256_cmp_ps(xs, rlx, _CMP_GE_OS);
			auto mm_xs = _mm256_movemask_ps(rlx_check);
			if (mm_ys & mm_xs)
				break;
		}

		for (; i < count; ++i)
		{
			auto y = coordinates.ys[i];
			if (y >= r.ly && y <= r.hy)
			{
				auto x = coordinates.xs[i];
				if (x >= r.lx)
					return i;
			}
		}

		return count;
	}

	static int get_next_idx_lxly(const Ranks& points, const Coordinates& coordinates, Rect rect, int idx)
	{
		auto count = (int)points.size();

		Rect r = rect;
		auto i = idx;

		for (; i < count && (i & 7); ++i)
		{
			auto y = coordinates.ys[i];
			if (y >= r.ly)
			{
				auto x = coordinates.xs[i];
				if (x >= r.lx)
					return i;
			}
		}

		auto ys_ptr = &coordinates.ys[0];
		auto xs_ptr = &coordinates.xs[0];
		auto rly = _mm256_set1_ps(r.ly);
		auto rlx = _mm256_set1_ps(r.lx);
		for (; i < count - 8; i += 8)
		{
			auto ys = _mm256_load_ps(ys_ptr + i);
			auto rly_check = _mm256_cmp_ps(ys, rly, _CMP_GE_OS);
			auto mm_ys = _mm256_movemask_ps(rly_check);
			if (!mm_ys)
				continue;

			auto xs = _mm256_load_ps(xs_ptr + i);
			auto rlx_check = _mm256_cmp_ps(xs, rlx, _CMP_GE_OS);
			auto mm_xs = _mm256_movemask_ps(rlx_check);
			if (mm_ys & mm_xs)
				break;
		}

		for (; i < count; ++i)
		{
			auto y = coordinates.ys[i];
			if (y >= r.ly)
			{
				auto x = coordinates.xs[i];
				if (x >= r.lx)
					return i;
			}
		}

		return count;
	}

	static int get_next_idx_lxhy(const Ranks& points, const Coordinates& coordinates, Rect rect, int idx)
	{
		auto count = (int)points.size();

		Rect r = rect;
		auto i = idx;

		for (; i < count && (i & 7); ++i)
		{
			auto y = coordinates.ys[i];
			if (y <= r.hy)
			{
				auto x = coordinates.xs[i];
				if (x >= r.lx)
					return i;
			}
		}

		auto ys_ptr = &coordinates.ys[0];
		auto xs_ptr = &coordinates.xs[0];
		auto rhy = _mm256_set1_ps(r.hy);
		auto rlx = _mm256_set1_ps(r.lx);
		for (; i < count - 8; i += 8)
		{
			auto ys = _mm256_load_ps(ys_ptr + i);
			auto rhy_check = _mm256_cmp_ps(ys, rhy, _CMP_LE_OS);
			auto mm_ys = _mm256_movemask_ps(rhy_check);
			if (!mm_ys)
				continue;

			auto xs = _mm256_load_ps(xs_ptr + i);
			auto rlx_check = _mm256_cmp_ps(xs, rlx, _CMP_GE_OS);
			auto mm_xs = _mm256_movemask_ps(rlx_check);
			if (mm_ys & mm_xs)
				break;
		}

		for (; i < count; ++i)
		{
			auto y = coordinates.ys[i];
			if (y <= r.hy)
			{
				auto x = coordinates.xs[i];
				if (x >= r.lx)
					return i;
			}
		}

		return count;
	}

	static int get_next_idx_lx(const Ranks& points, const Coordinates& coordinates, Rect rect, int idx)
	{
		auto count = (int)points.size();

		Rect r = rect;
		auto i = idx;

		for (; i < count && (i & 7); ++i)
		{
			auto x = coordinates.xs[i];
			if (x >= r.lx)
				return i;
		}

		auto xs_ptr = &coordinates.xs[0];
		auto rlx = _mm256_set1_ps(r.lx);
		for (; i < count - 8; i += 8)
		{
			auto xs = _mm256_load_ps(xs_ptr + i);
			auto rlx_check = _mm256_cmp_ps(xs, rlx, _CMP_GE_OS);
			auto mm_xs = _mm256_movemask_ps(rlx_check);
			if (mm_xs)
				break;
		}

		for (; i < count; ++i)
		{
			auto x = coordinates.xs[i];
			if (x >= r.lx)
				return i;
		}

		return count;
	}

	static int get_next_idx_hxlyhy(const Ranks& points, const Coordinates& coordinates, Rect rect, int idx)
	{
		auto count = (int)points.size();

		Rect r = rect;
		auto i = idx;

		for (; i < count && (i & 7); ++i)
		{
			auto y = coordinates.ys[i];
			if (y >= r.ly && y <= r.hy)
			{
				auto x = coordinates.xs[i];
				if (x <= r.hx)
					return i;
			}
		}

		auto ys_ptr = &coordinates.ys[0];
		auto xs_ptr = &coordinates.xs[0];
		auto rly = _mm256_set1_ps(r.ly);
		auto rhy = _mm256_set1_ps(r.hy);
		auto rhx = _mm256_set1_ps(r.hx);
		for (; i < count - 8; i += 8)
		{
			auto ys = _mm256_load_ps(ys_ptr + i);
			auto rly_check = _mm256_cmp_ps(ys, rly, _CMP_GE_OS);
			auto rhy_check = _mm256_cmp_ps(ys, rhy, _CMP_LE_OS);
			auto mm_ys = _mm256_movemask_ps(rly_check) & _mm256_movemask_ps(rhy_check);
			if (!mm_ys)
				continue;

			auto xs = _mm256_load_ps(xs_ptr + i);
			auto rhx_check = _mm256_cmp_ps(xs, rhx, _CMP_LE_OS);
			auto mm_xs = _mm256_movemask_ps(rhx_check);
			if (mm_ys & mm_xs)
				break;
		}

		for (; i < count; ++i)
		{
			auto y = coordinates.ys[i];
			if (y >= r.ly && y <= r.hy)
			{
				auto x = coordinates.xs[i];
				if (x <= r.hx)
					return i;
			}
		}

		return count;
	}

	static int get_next_idx_hxly(const Ranks& points, const Coordinates& coordinates, Rect rect, int idx)
	{
		auto count = (int)points.size();

		Rect r = rect;
		auto i = idx;

		for (; i < count && (i & 7); ++i)
		{
			auto y = coordinates.ys[i];
			if (y >= r.ly)
			{
				auto x = coordinates.xs[i];
				if (x <= r.hx)
					return i;
			}
		}

		auto ys_ptr = &coordinates.ys[0];
		auto xs_ptr = &coordinates.xs[0];
		auto rly = _mm256_set1_ps(r.ly);
		auto rhx = _mm256_set1_ps(r.hx);
		for (; i < count - 8; i += 8)
		{
			auto ys = _mm256_load_ps(ys_ptr + i);
			auto rly_check = _mm256_cmp_ps(ys, rly, _CMP_GE_OS);
			auto mm_ys = _mm256_movemask_ps(rly_check);
			if (!mm_ys)
				continue;

			auto xs = _mm256_load_ps(xs_ptr + i);
			auto rhx_check = _mm256_cmp_ps(xs, rhx, _CMP_LE_OS);
			auto mm_xs = _mm256_movemask_ps(rhx_check);
			if (mm_ys & mm_xs)
				break;
		}

		for (; i < count; ++i)
		{
			auto y = coordinates.ys[i];
			if (y >= r.ly)
			{
				auto x = coordinates.xs[i];
				if (x <= r.hx)
					return i;
			}
		}

		return count;
	}

	static int get_next_idx_hxhy(const Ranks& points, const Coordinates& coordinates, Rect rect, int idx)
	{
		auto count = (int)points.size();

		Rect r = rect;
		auto i = idx;

		for (; i < count && (i & 7); ++i)
		{
			auto y = coordinates.ys[i];
			if (y <= r.hy)
			{
				auto x = coordinates.xs[i];
				if (x <= r.hx)
					return i;
			}
		}

		auto ys_ptr = &coordinates.ys[0];
		auto xs_ptr = &coordinates.xs[0];
		auto rhy = _mm256_set1_ps(r.hy);
		auto rhx = _mm256_set1_ps(r.hx);
		for (; i < count - 8; i += 8)
		{
			auto ys = _mm256_load_ps(ys_ptr + i);
			auto rhy_check = _mm256_cmp_ps(ys, rhy, _CMP_LE_OS);
			auto mm_ys = _mm256_movemask_ps(rhy_check);
			if (!mm_ys)
				continue;

			auto xs = _mm256_load_ps(xs_ptr + i);
			auto rhx_check = _mm256_cmp_ps(xs, rhx, _CMP_LE_OS);
			auto mm_xs = _mm256_movemask_ps(rhx_check);
			if (mm_ys & mm_xs)
				break;
		}

		for (; i < count; ++i)
		{
			auto y = coordinates.ys[i];
			if (y <= r.hy)
			{
				auto x = coordinates.xs[i];
				if (x <= r.hx)
					return i;
			}
		}

		return count;
	}

	static int get_next_idx_hx(const Ranks& points, const Coordinates& coordinates, Rect rect, int idx)
	{
		auto count = (int)points.size();

		Rect r = rect;
		auto i = idx;

		for (; i < count && (i & 7); ++i)
		{
			auto x = coordinates.xs[i];
			if (x <= r.hx)
				return i;
		}

		auto xs_ptr = &coordinates.xs[0];
		auto rhx = _mm256_set1_ps(r.hx);
		for (; i < count - 8; i += 8)
		{
			auto xs = _mm256_load_ps(xs_ptr + i);
			auto rhx_check = _mm256_cmp_ps(xs, rhx, _CMP_LE_OS);
			auto mm_xs = _mm256_movemask_ps(rhx_check);
			if (mm_xs)
				break;
		}

		for (; i < count; ++i)
		{
			auto x = coordinates.xs[i];
			if (x <= r.hx)
				return i;
		}

		return count;
	}

	static int get_next_idx_lyhy(const Ranks& points, const Coordinates& coordinates, Rect rect, int idx)
	{
		auto count = (int)points.size();

		Rect r = rect;
		auto i = idx;

		for (; i < count && (i & 7); ++i)
		{
			auto y = coordinates.ys[i];
			if (y >= r.ly && y <= r.hy)
				return i;
		}

		auto ptr = &coordinates.ys[0];
		auto rly = _mm256_set1_ps(r.ly);
		auto rhy = _mm256_set1_ps(r.hy);
		for (; i < count - 8; i += 8)
		{
			auto ys = _mm256_load_ps(ptr + i);
			auto rly_check = _mm256_cmp_ps(ys, rly, _CMP_GE_OS);
			auto rhy_check = _mm256_cmp_ps(ys, rhy, _CMP_LE_OS);
			if (_mm256_movemask_ps(rly_check) & _mm256_movemask_ps(rhy_check))
				break;
		}

		for (; i < count; ++i)
		{
			auto y = coordinates.ys[i];
			if (y >= r.ly && y <= r.hy)
			{
				return i;
			}
		}

		return count;
	}

	static int get_next_idx_ly(const Ranks& points, const Coordinates& coordinates, Rect rect, int idx)
	{
		auto count = (int)points.size();

		Rect r = rect;
		auto i = idx;

		for (; i < count && (i & 7); ++i)
		{
			auto y = coordinates.ys[i];
			if (y >= r.ly)
				return i;
		}

		auto ptr = &coordinates.ys[0];
		auto rly = _mm256_set1_ps(r.ly);
		for (; i < count - 8; i += 8)
		{
			auto ys = _mm256_load_ps(ptr + i);
			auto rly_check = _mm256_cmp_ps(ys, rly, _CMP_GE_OS);
			if (_mm256_movemask_ps(rly_check))
				break;
		}

		for (; i < count; ++i)
		{
			auto y = coordinates.ys[i];
			if (y >= r.ly)
			{
				return i;
			}
		}

		return count;
	}

	static int get_next_idx_hy(const Ranks& points, const Coordinates& coordinates, Rect rect, int idx)
	{
		auto count = (int)points.size();

		Rect r = rect;
		auto i = idx;

		for (; i < count && (i & 7); ++i)
		{
			auto y = coordinates.ys[i];
			if (y <= r.hy)
				return i;
		}

		auto ptr = &coordinates.ys[0];
		auto rhy = _mm256_set1_ps(r.hy);
		for (; i < count - 8; i += 8)
		{
			auto ys = _mm256_load_ps(ptr + i);
			auto rhy_check = _mm256_cmp_ps(ys, rhy, _CMP_LE_OS);
			if (_mm256_movemask_ps(rhy_check))
				break;
		}

		for (; i < count; ++i)
		{
			auto y = coordinates.ys[i];
			if (y <= r.hy)
			{
				return i;
			}
		}

		return count;
	}

	static int get_next_idx_(const Ranks& points, const Coordinates& coordinates, Rect rect, int idx)
	{
		return idx;
	}

public:
	typedef int(*FunctionDefinition)(const Ranks& points, const Coordinates& coordinates, Rect rect, int idx);

	static FunctionDefinition get_next_idx(RectChecks checks)
	{
		switch (checks)
		{
		case CheckLx | CheckHx | CheckLy | CheckHy: return get_next_idx_lxhxlyhy;
		case CheckLx | CheckHx | CheckHy:			return get_next_idx_lxhxhy;
		case CheckLx | CheckHx | CheckLy:			return get_next_idx_lxhxly;
		case CheckLx | CheckHx:						return get_next_idx_lxhx;
		case CheckLx | CheckLy | CheckHy:			return get_next_idx_lxlyhy;
		case CheckLx | CheckLy:						return get_next_idx_lxly;
		case CheckLx | CheckHy:						return get_next_idx_lxhy;
		case CheckLx:								return get_next_idx_lx;
		case CheckHx | CheckLy | CheckHy:			return get_next_idx_hxlyhy;
		case CheckHx | CheckLy:						return get_next_idx_hxly;
		case CheckHx | CheckHy:						return get_next_idx_hxhy;
		case CheckHx:								return get_next_idx_hx;
		case CheckLy | CheckHy:						return get_next_idx_lyhy;
		case CheckLy:								return get_next_idx_ly;
		case CheckHy:								return get_next_idx_hy;
		default:
			std::cout << std::endl << checks << std::endl;
			throw "Bad";
		}
	}
};

#endif