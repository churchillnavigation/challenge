// Copyright 2015 Paul Lange
#include "point_search.h"
#include "kdtree.h"

extern "C" {
	__declspec(dllexport)
	SearchContext* __stdcall
	create(const Point* points_begin, const Point* points_end)
	{
		if (points_begin == points_end) return 0;
		void* ptr;
		ptr = new PointTree(points_begin, points_end);
		return (SearchContext*)ptr;
	}

	__declspec(dllexport)
	int32_t __stdcall
	search(SearchContext* sc, const Rect rect, const int32_t count, Point* out_points)
	{
		if(sc==0) return 0;
		return reinterpret_cast<PointTree*>(sc)->search(rect, count, out_points);
	}

	__declspec(dllexport)
	SearchContext* __stdcall
	destroy(SearchContext* sc)
	{
		delete reinterpret_cast<PointTree*>(sc);
		return nullptr;
	}
}