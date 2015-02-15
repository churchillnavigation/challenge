#include <algorithm>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <list>
#include <vector>
#include <set>

#include <assert.h>

#include <ftl_search/slice.h>
#include <ftl_search/node.h>
#include <ftl_search/point_set.h>
#include <ftl_search/point_search.h>

/**
 * Takes a two rectangles and tests if they intersect. The
 * rectangles are treated inclusive, so that if their
 * boundary values are the same they still intersect.
 *
 * @param r First rectangle.
 * @param o Other rectangle.
 *
 * @return True if they intersect, false otherwise.
 */
bool intersect(const Rect &r, const Rect &o)
{
    bool no_overlap = r.lx > o.hx ||
           r.hx < o.lx ||
           r.ly > o.hy ||
           r.hy < o.ly;
    return !no_overlap;
}

struct SearchContext
{
    Node *root;
    Point *points;
    NodePool *pool;
};

SearchContext * __stdcall create(const Point *points_begin, const Point *points_end)
{
    size_t num_elments = (size_t)(points_end - points_begin);
	if (num_elments > 0)
	{
		Point *points = (Point *) malloc( num_elments * sizeof( Point ) );

		slice_t point_slice;
		point_slice.m_start = points;
		point_slice.m_length = num_elments;

		memcpy( point_slice.m_start, points_begin, sizeof( Point ) * point_slice.m_length );

		NodePool *pool = new NodePool( num_elments );
		SearchContext *sc = new SearchContext( );
		sc->root = init_tree( point_slice, *pool );
		sc->points = points;
		sc->pool = pool;

		return sc;
	}
	else
	{
		SearchContext *sc = new SearchContext( );
		sc->root = NULL;
		sc->points = NULL;
		sc->pool = NULL;

		return sc;
	}
}

int32_t __stdcall search(SearchContext *sc, const Rect rect, const int32_t count, Point *out_points)
{
	if (sc->root == NULL)
	{
		return 0;
	}

    point_set ps( rect, count );

    std::list<Node *> nodes_to_visit;
    nodes_to_visit.push_back( sc->root );
    while( nodes_to_visit.size( ) > 0 )
    {
        Node *v = nodes_to_visit.front( );
        nodes_to_visit.pop_front( );

        ps.add( v->m_points );

        Node *right = v->m_right;
        Node *left = v->m_left;
 
        if( right != NULL && intersect( rect, right->m_rect ) && right->m_points.m_start[ 0 ].rank < ps.max( ) )
        {
            nodes_to_visit.push_front( right );
        }
        if( left != NULL && intersect( rect, left->m_rect ) && left->m_points.m_start[ 0 ].rank < ps.max( ) )
        {
            nodes_to_visit.push_front( left );
        }
    }

    size_t size = ps.write( out_points, count );
    return (int32_t) size;
}

SearchContext * __stdcall destroy(SearchContext *sc)
{
	if(sc->root != NULL)
	{
		free(sc->points);
		delete sc->pool;
		delete sc;
	}
	else
	{
		delete sc;
	}

    return NULL;
}
