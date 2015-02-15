#ifndef __NODE_H__
#define __NODE_H__

#include <ftl_search/slice.h>
#include <ftl_search/point_search.h>

/**
 * The maximum number of points stored at a leaf.
 */
const size_t MAX_POINTS_AT_LEAF = 256;

/**
 * Represents a split in one dimension of the space.
 */
struct Node
{   
    /**
     * The rect of this node.
     */
    Rect m_rect;

    /**
     * The left node.
     */
    Node *m_left;

    /**
     * The right node.
     */
    Node *m_right;

    /**
     * The best points.
     */
    slice_t m_points;
};


/**
 * A data structure that allows Nodes to be allocated close
 * in memory.
 */
class NodePool
{
public:
    /**
     * Holds the given number of objects.
     *
     * @param size Number of objects to allocate.
     */
    NodePool(size_t size);

	/**
	 * Desutructor, frees allocations.
	 */
	~NodePool();

    /**
     * Allocates a Node object.
     *
     * @param r The rect of the node.
     * @param left The left node.
     * @param right The right node.
     * @param points The points that this node owns.
     */
    Node *allocate(Rect &r, Node *left, Node *right, slice_t &points);

	/**
	 * Allocates points for an internal node.
	 *
	 * @return An array of points of size MAX_POINTS_AT_LEAF.
	 */
	Point *allocate_points(size_t num_points);

    /**
     * Free the memory occupied by the node pool.
     */
    void clear();
private:
    /**
     * Number of allocations.
     */
    size_t m_num_allocations;

    /**
     * Lsit of allocated objects.
     */
	Node *m_allocated;

	/**
	 * Number of allocated points.
	 */
	size_t m_num_allocated_points;

	/**
	 * The allocated points.
	 */
	Point *m_allocated_points;
};

/**
 * Initializes the tree from the points, allocating
 * nodes using the given node pool.
 *
 * The space is sequentially split in the dimension
 * with the largest range of values. For each split
 * the best points of the two children is stored, to
 * allow for fast access.
 *
 * @param point_slice The points to split.
 * @param pool The nodes are allocated here.
 *
 * @return The root node.
 */
Node *init_tree(slice_t &point_slice, NodePool &pool);

/**
 * Merges the points of two slices, by creating a new
 * slice that contains the best points of the two
 * given slices.
 *
 * @param left The left slice.
 * @param right The right slice.
 * @param pool Used for allocation of points.
 *
 * @return The best points from the given slices.
 */
slice_t merge_points(slice_t &left, slice_t &right, NodePool &pool);

#endif /* End of __NODE_H__ */
