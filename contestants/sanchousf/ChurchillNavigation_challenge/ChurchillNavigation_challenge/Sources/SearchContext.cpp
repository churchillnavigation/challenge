#include "SearchContext.h"

#include <algorithm>

//--------------------------------------------------------------------------------------------------------
SearchContext* __stdcall create(const Point* points_begin, const Point* points_end)
{
    return new SearchContext(points_begin, points_end);
}

int32_t __stdcall search(SearchContext* sc, const Rect rect, const int32_t count, Point* out_points)
{
    return sc->search(rect, count, out_points);
}

SearchContext* __stdcall destroy(SearchContext* sc)
{
    delete sc;
    return nullptr;
}

//--------------------------------------------------------------------------------------------------------

SearchContext::SearchContext(const Point* points_begin, const Point* points_end) : m_grid(points_begin, points_end)// m_points(points_begin, points_end)
{
    //std::sort(m_points.begin(), m_points.end(), [](const Point& _left, const Point& _right) { return _left.rank < _right.rank; });
}

SearchContext::~SearchContext()
{

}

int32_t SearchContext::search(const Rect& rect, const int32_t count, Point* out_points) const
{
    //return searchInPoints(m_points, rect, count, out_points);
    return m_grid.search(rect, count, out_points);
}

bool SearchContext::isInRect(const Rect& _rect, const Point& _point) const
{
    return (_point.x >= _rect.lx) && (_point.y >= _rect.ly) && (_point.x <= _rect.hx) && (_point.y <= _rect.hy);
}

int32_t SearchContext::searchInPoints(const type_points& _tree, const Rect& rect, const int32_t count, Point* out_points) const
{
    int32_t found_n = 0;

    for (const auto& i : _tree)
    {
        if (found_n >= count) break;

        if (isInRect(rect, i))
            out_points[found_n++] = i;
    }

    return found_n;
}