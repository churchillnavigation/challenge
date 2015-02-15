#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <assert.h>
#include <math.h>

#include <algorithm>

#include <ftl_search/node.h>
#include <ftl_search/dim.h>

int
point_rank_cmp(const void *a, const void *b)
{
    Point *ap = (Point *) a;
    Point *bp = (Point *) b;

    return (bp->rank < ap->rank) - (ap->rank < bp->rank);
}

Node *
init_tree(slice_t &point_slice, NodePool &pool)
{
    if( point_slice.m_length <= MAX_POINTS_AT_LEAF )
    {
        Rect r = find_surrounding_rect( point_slice );
        qsort( point_slice.m_start, point_slice.m_length, sizeof( Point ), point_rank_cmp );
        return pool.allocate( r, NULL, NULL, point_slice );
    }

    Rect r = find_surrounding_rect( point_slice );
    dim_split split = split_dim( point_slice, r );

    Node *left = init_tree( split.m_left, pool );
    Node *right = init_tree( split.m_right, pool );

    slice_t root_points = merge_points( left->m_points, right->m_points, pool );
    return pool.allocate( r, left, right, root_points );
}

slice_t
merge_points(slice_t &left, slice_t &right, NodePool &pool)
{
    slice_t out;
    out.m_length = std::min( left.m_length + right.m_length, MAX_POINTS_AT_LEAF );
    out.m_start = pool.allocate_points( out.m_length );

    unsigned int il = 0;
    unsigned int ir = 0;
    for(unsigned int i = 0; i < out.m_length; i++)
    {
        int32_t left_rank = INT_MAX;
        int32_t right_rank = INT_MAX;

        if( il < left.m_length )
        {
            left_rank = left.m_start[ il ].rank;
        }
        if( ir < right.m_length )
        {
            right_rank = right.m_start[ ir ].rank;
        }

        if( left_rank < right_rank )
        {
            out.m_start[ i ] = left.m_start[ il ];
            il++;
        }
        else
        {
            out.m_start[ i ] = right.m_start[ ir ];
            ir++;
        }
    }

    return out;
}

NodePool::NodePool(size_t size)
{
    double h = ceil( log2( ( (float)size / MAX_POINTS_AT_LEAF ) ) );
    size_t n = pow( 2, ceil( h ) );

	m_num_allocations = 2 * n;
	m_num_allocated_points = n * MAX_POINTS_AT_LEAF;

	m_allocated = (Node *) malloc( sizeof( Node ) * m_num_allocations );
	m_allocated_points = (Point *) malloc( sizeof( Point ) * m_num_allocated_points );
}

NodePool::~NodePool()
{
	free( m_allocated );
	free( m_allocated_points );
}

Node *
NodePool::allocate(Rect &r, Node *left, Node *right, slice_t &points)
{
	m_num_allocations--;
	Node *n = &m_allocated[ m_num_allocations ];
	n->m_rect = r;
	n->m_left = left;
	n->m_right = right;
	n->m_points = points;
	
	return n;
}

Point *
NodePool::allocate_points(size_t num_points)
{
	m_num_allocated_points -= num_points;
	return &m_allocated_points[ m_num_allocated_points ];
}
