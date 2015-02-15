#include <algorithm>

#include <ftl_search/dim.h>

/**
 * Compares points on the x-coordinate.
 *
 * @param a A point.
 * @param b A point.
 *
 * @return true if a.x < b.x, false otherwise.
 */
bool point_x_cmp(const Point &a, const Point &b)
{
    return a.x < b.x;
}

/**
 * Compares points on the y-coordinate.
 *
 * @param a A point.
 * @param b A point.
 *
 * @return true if a.y < b.y, false otherwise.
 */
bool point_y_cmp(const Point &a, const Point &b)
{
    return a.y < b.y;
}


Rect
find_surrounding_rect(slice_t &points)
{
    Rect r;
    if(points.m_length <= 0)
    {
        r.lx = r.hx = r.ly = r.hy = 0;
        return r;
    }

    r.lx = points.m_start[ 0 ].x;
    r.hx = points.m_start[ 0 ].x;
    r.ly = points.m_start[ 0 ].y;
    r.hy = points.m_start[ 0 ].y;

    for(unsigned int i = 0; i < points.m_length; i++)
    {
        r.lx = std::min( r.lx, points.m_start[ i ].x );
        r.hx = std::max( r.hx, points.m_start[ i ].x );
        r.ly = std::min( r.ly, points.m_start[ i ].y );
        r.hy = std::max( r.hy, points.m_start[ i ].y );
    }

    return r;
}

dim_split
split_dim(slice_t &points, Rect &r)
{
    if( (r.hx - r.lx ) > (r.hy - r.ly) )
    {
        std::sort( points.m_start, points.m_start + points.m_length, point_x_cmp );
    }
    else
    {
        std::sort( points.m_start, points.m_start + points.m_length, point_y_cmp );
    }
    size_t middle = points.m_length / 2;
    dim_split split;
    split.m_left.m_start = points.m_start;
    split.m_left.m_length = middle;

    split.m_right.m_start = points.m_start + middle;
    split.m_right.m_length = points.m_length - middle;

    return split;
}

