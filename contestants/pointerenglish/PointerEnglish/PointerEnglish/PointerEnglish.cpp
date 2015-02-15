// PointerEnglish.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"

// This header contains the logic for searching.
#include "QueryPoints.h"

// Use the standard library when convenient.
#include <vector>
#include <algorithm>

/* Load the provided points into an internal data structure. The pointers follow the STL iterator convention, where
"points_begin" points to the first element, and "points_end" points to one past the last element. The input points are
only guaranteed to be valid for the duration of the call. Return a pointer to the context that can be used for
consecutive searches on the data. */
extern "C" __declspec(dllexport) SearchContext* __stdcall create(const Point* points_begin, const Point* points_end)
{
    // Create the search context
    auto qp = new QueryPoints(points_begin, points_end);
    // Create the search structure.
    qp->MakeQueryStructure();
    // Return the handle to the query structure.
    return qp;
}

/* Search for "count" points with the smallest ranks inside "rect" and copy them ordered by smallest rank first in
"out_points". Return the number of points copied. "out_points" points to a buffer owned by the caller that
can hold "count" number of Points. */
extern "C" __declspec(dllexport) size_t __stdcall search(SearchContext* sc, const Rect rect, const int32_t count, Point* out_points)
{
    // Use query structure to find contained points.
    auto qp = static_cast<QueryPoints*>(sc);
    qp->Search(rect);
    // Access inside point locally
    std::vector<Point>& results = qp->GetResults();
    // Determine number of points to return to caller
    size_t out_count = min((size_t)count, results.size());
    // Get the out_count smallest ranks
    std::partial_sort_copy(results.begin(), results.end(),
        out_points, out_points + out_count,
        [](const Point& lhs, const Point& rhs){return lhs.rank < rhs.rank; });

    return out_count;
}

/* Release the resources associated with the context. Return nullptr if successful, "sc" otherwise. */
extern "C" __declspec(dllexport) SearchContext* __stdcall destroy(SearchContext* sc)
{
    auto qp = static_cast<QueryPoints*>(sc);

    delete qp;
    return 0;
}
