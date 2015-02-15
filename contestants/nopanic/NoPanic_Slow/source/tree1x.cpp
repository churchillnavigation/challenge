#include "point_search.h"
#include <algorithm>
#include <vector>
#include <assert.h>
#include "Tree.h"
#include "PointRange.h"
#include "Profiler.h"

using namespace std;

struct SearchContext
{
	Tree *tree;
#ifdef USE_GRID
	Grid *grid;
#endif
};

#ifdef _WIN32
#define DLL(ret,name) __declspec(dllexport) ret __stdcall name
#else
#define DLL(ret, name) ret name
#endif

extern "C" {

DLL(SearchContext*,create)(const Point* points_begin, const Point* points_end)
{
	const int OUTSIDE_LIMIT = 16;

	auto ret = new SearchContext;
	
	PointRange points(points_begin, points_end);
	auto span = points.outside() < OUTSIDE_LIMIT // XXX
	          ? points.small_span()
	          : points.span();
	
	ret->tree = new Tree(span);
	for (size_t i = 0; i < points.size(); i++)
	{
		ret->tree->add_point(points[i]);
	}

#ifdef USE_GRID
	ret->grid = new Grid(points);
#endif
	
	return ret;
}

DLL(int32_t,search)(SearchContext* sc, const Rect rect, const int32_t count, Point* out_points)
{
	Query q;
	q.rect = rect;
	q.capacity = count;
	q.count = 0;
	q.points = out_points;

	if (rect.lx <= rect.hx && rect.ly <= rect.hy)
	{
#ifdef USE_GRID
		if (sc->grid->query(q))
			return q.count;
#endif

		sc->tree->query(q);
	}

#ifdef TREE_NEW_INSERT
	sort(out_points, out_points + q.count, [](const Point &a, const Point &b){return a.rank < b.rank;});
#endif

	return q.count;
}

DLL(SearchContext*,destroy)(SearchContext* sc)
{
	//sc->tree->print_stats();
	PROFILE(ctx->print())
	delete sc->tree;
#ifdef USE_GRID
	delete sc->grid;
#endif
	delete sc;
	return NULL;
}

}
