#include "dll.h"

#include "solution.h"

SearchContext* __stdcall create(const Point *points_begin, const Point *points_end)
{
    return (SearchContext*)new Solution(points_begin, points_end);
}

int32_t search(SearchContext *sc, const Rect rect, const int32_t count, Point *out_points)
{
    Solution* solution = (Solution*)sc;
    return solution->search(rect, count, out_points);
}


SearchContext *destroy(SearchContext *sc)
{
    delete (Solution*)sc;
    return nullptr;
}
