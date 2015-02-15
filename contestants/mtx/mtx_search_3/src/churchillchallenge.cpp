// churchillchallenge.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"

#include "point_search.h"
#include "tree.h"

/* Load the provided points into an internal data structure. The pointers follow the STL iterator convention, where
"points_begin" points to the first element, and "points_end" points to one past the last element. The input points are
only guaranteed to be valid for the duration of the call. Return a pointer to the context that can be used for
consecutive searches on the data. */
extern "C" __declspec(dllexport) SearchContext* __stdcall create(const Point* points_begin, const Point* points_end)
{
	try
	{
		std::vector<Point> pts(points_begin, points_end);

		if (pts.size() > 4)
		{
			pts.pop_back();
			pts.pop_back();
			pts.pop_back();
			pts.pop_back();
		}

		return (SearchContext*)new cc::CompoundSearchContext(pts);
	}
	catch (const std::exception & e)
	{
		printf("exception: %s", e.what());
	}
	catch (...)
	{
		printf("exception ...");
	}
	return (SearchContext*)new cc::CompoundSearchContext(std::vector<Point>());
}

/* Search for "count" points with the smallest ranks inside "rect" and copy them ordered by smallest rank first in
"out_points". Return the number of points copied. "out_points" points to a buffer owned by the caller that
can hold "count" number of Points. */
extern "C" __declspec(dllexport) int32_t __stdcall search(SearchContext* sc, const Rect rect, const int32_t count, Point* out_points)
{
	if (!sc) {
		return 0;
	}

//	return reinterpret_cast<cc::SearchContext*>(sc)->Search(rect.lx, rect.ly, std::nextafterf(rect.hx, std::numeric_limits<float>::infinity()), std::nextafterf(rect.hy, std::numeric_limits<float>::infinity()), count, out_points);
	return reinterpret_cast<cc::CompoundSearchContext*>(sc)->Search(rect.lx, rect.ly, rect.hx, rect.hy, count, out_points);
}

/* Release the resources associated with the context. Return nullptr if successful, "sc" otherwise. */
extern "C" __declspec(dllexport) SearchContext* __stdcall destroy(SearchContext* sc)
{
	delete ((cc::CompoundSearchContext*)sc);

	return nullptr;
}

