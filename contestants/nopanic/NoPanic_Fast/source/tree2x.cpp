#include "point_search.h"
#include <algorithm>
#include <iostream>
#include <limits>
#include <vector>
#include <queue>
#include "Tree2.h"
#include "PointRange.h"
#include "Profiler.h"

using namespace std;

struct SearchContext
{
	Tree2 *tree;
};

#ifdef _WIN32
#define DLL(ret,name) __declspec(dllexport) ret __stdcall name
#else
#define DLL(ret, name) ret name
#endif


extern "C" {

DLL(SearchContext*, create)(const Point* points_begin, const Point* points_end)
{
	const int OUTSIDE_LIMIT = 16;
	PROFILE(ctx->Version = 2)
	auto ret = new SearchContext;
	
	PointRange points(points_begin, points_end);
	auto span = points.outside() < OUTSIDE_LIMIT // XXX
	          ? points.small_span()
	          : points.span();
	
	ret->tree = new Tree2(span);
	
	for (size_t i = 0; i < points.size(); i++)
	{
		/*
		 * TODO: since rank == INT32_MAX case is reserved for our internal purpoeses
		 *       we should to an extra check here to deal with this if necessary.
		 *
		 *       Ditto for rank == INT32_MIN.
		 */
		ret->tree->add_point(points[i]);
	}

	ret->tree->prepare();
	return ret;
}

DLL(int32_t, search)(SearchContext* sc, const Rect rect, const int32_t count, Point* out_points)
{
	Query2 q;
	q.rect    = rect;
	q.request = count;
	sc->tree->query2(q);

	int32_t ret = 0;
	int32_t min_rank = numeric_limits<int32_t>::min();

	PROFILE(ctx->tmp0 = 0)
	const unsigned size = q.points.size();
	for (unsigned i = 0; i < size; i++)
	{
#ifdef Q2_INDIRECT
		Point &point = *q.points[i];
#else
		Point &point = q.points[i];
#endif

#if defined(USE_MORE_SIMD) || defined(Q2_INDIRECT)
		point.rank   = q.ranks[i];
#endif

		if (point.rank == numeric_limits<int32_t>::max())
			continue;
		if (point.rank == numeric_limits<int32_t>::min())
			continue;
		
		if (ret < count)
		{
			out_points[ret++] = point;
			min_rank = max(min_rank, point.rank);
			PROFILE(ctx->tmp0 = i)
			continue;
		}

		if (min_rank < point.rank)
			continue;
		
		auto min_rank_ = min_rank;
		min_rank = numeric_limits<int32_t>::min();
		for (int j = count - 1; j >= 0; j--)
		{
			if (min_rank_ == out_points[j].rank)
			{
				out_points[j] = point;
				min_rank_ = numeric_limits<int32_t>::max();
			}
		
			min_rank = max(min_rank, out_points[j].rank);
		}
		PROFILE(ctx->ResultsOverwritten++)
		PROFILE(ctx->tmp0 = i)
	}
	PROFILE(ctx->PointsUsed += ctx->tmp0)
	PROFILE(if (ctx->tmp0 > 70) ctx->SlowQueries2++)

	sort(out_points, out_points + ret, [](const Point &a, const Point &b)
	{
		return a.rank < b.rank;
	});
	
	return ret;
}

DLL(SearchContext*, destroy)(SearchContext* sc)
{
	PROFILE(ctx->print())
	//sc->tree->print_stats();
	delete sc->tree;
	delete sc;
	return NULL;
}

}
