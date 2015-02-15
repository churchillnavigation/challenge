#include <limits.h>

#include <ftl_search/point_search.h>
#include <ftl_search/point_set.h>

bool
contains(const Rect &r, const Point &p)
{
    return r.lx <= p.x &&
           r.hx >= p.x &&
           r.ly <= p.y &&
           r.hy >= p.y;
}

bool
point_cmp::operator()(const Point &a, const Point &b)
{
    if( a.rank != b.rank )
    {
        return a.rank < b.rank;
    }
    else
    {
        if( a.id != b.id )
        {
            return a.id < b.id;
        }
        else
        {
            if( a.x != b.x)
            {
                return a.x < b.x;
            }
            else
            {
                return a.y < b.y;
            }
        }
    }
}

point_set::point_set(const Rect &r, size_t max_size)
    : m_rect( r ), m_max_size( max_size )
{
}

int32_t
point_set::max()
{
    if( m_set.size( ) < m_max_size )
    {
        return INT_MAX;
    }
    else
    {
        return m_set.rbegin( )->rank;
    }
}


void
point_set::add(slice_t &points)
{
    for(unsigned int i = 0; i < points.m_length; i++)
    {
        if( points.m_start[ i ].rank >= max( ) && m_set.size( ) >= m_max_size )
        {
            break;
        }
        if( contains( m_rect, points.m_start[ i ] ) )
        {
            m_set.insert( points.m_start[ i ] ); 
            if( m_set.size( ) > m_max_size )
            {
                m_set.erase( *m_set.rbegin( ) );
            }
        }
    }
}

size_t
point_set::size()
{
    return m_set.size( );
}

size_t
point_set::write(Point *out, size_t count)
{
    unsigned int i = 0;
    for(std::set<Point, point_cmp>::iterator it = m_set.begin( ); it != m_set.end( ); ++it)
    {
        if( i >= count )
        {
            break;
        }
        
        out[ i ] = *it;
        i++;
    }

    return i;
}
