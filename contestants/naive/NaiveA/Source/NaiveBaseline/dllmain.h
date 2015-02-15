#define DLL_EXPORT extern "C" __declspec(dllexport) 

#include "point_search.h"
#include "PointContainer.h"

struct SearchContext
{
	PointContainer * point_container;
};

/* Load the provided points into an internal data structure. The pointers follow the STL iterator convention, where
"points_begin" points to the first element, and "points_end" points to one past the last element. The input points are
only guaranteed to be valid for the duration of the call. Return a pointer to the context that can be used for
consecutive searches on the data. */
DLL_EXPORT SearchContext* create (const Point* points_begin, const Point* points_end);

/* Search for "count" points with the smallest ranks inside "rect" and copy them ordered by smallest rank first in
"out_points". Return the number of points copied. "out_points" points to a buffer owned by the caller that
can hold "count" number of Points. */
DLL_EXPORT int32_t search (SearchContext* sc, const Rect rect, const int32_t count, Point* out_points);

/* Release the resources associated with the context. Return nullptr if successful, "sc" otherwise. */
DLL_EXPORT SearchContext* destroy (SearchContext* sc);