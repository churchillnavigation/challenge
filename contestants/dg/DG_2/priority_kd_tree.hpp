#ifndef DIRK_GERRITS__PRIORITY_KD_TREE_HPP
#define DIRK_GERRITS__PRIORITY_KD_TREE_HPP

#include <vector>
#include "heap_structure.hpp"
#include "types.hpp"
class result_queue;

// A mixture of a priority search tree and a kd-tree, with a hint of range tree.
//
// A priority search tree would allow the 1-dimensional variant of the problem to be solved efficiently.  Its root node
// contains the point with the smallest rank, and the remainder of the points is split at the median x-coordinate and
// divided over two recursively defined priority search trees, which then form the node's children.  To generalize this
// to 2-dimensional points, we instead split based on x-coordinate at even levels of the tree but on y-coordinate at odd 
// levels of the tree, as is done in a kd-tree.
//
// To speed up the queries (at least for count <= 20), we cache the 20 lowest ranked points that lie in certain 
// subtrees.  As soon as we reach a node whose bounding box is completely contained in the query rectangle, and which
// has an associated cache, we can just get the results from the cache without traversing the tree further.  This is 
// similar to the way range trees work.  We build caches only for the nodes which are not too far down the tree; nodes
// near the leaves are too numerous, and each one will occur in only few queries.  With the available RAM we can build 
// 20-point caches for about a million nodes, so the first 20 levels of the tree (2^20 - 1 = 1,048,575 nodes).
class priority_kd_tree : private heap_structure<priority_kd_tree>
{
    struct node {
        ranked_2d_point point;
        coordinate_t split;
    };
    enum { levels_with_caches = 20 }; // the number of levels of the tree for which we build caches
    enum { cache_size = 20 }; // number of sorted points to store in each subtree cache
public:
    // Construct a tree storing the given points.
    explicit priority_kd_tree(identified_ranked_2d_point* points_begin, identified_ranked_2d_point* points_end);

    // Add the stored points which lie in the given rectangle into the given results queue.
    void collect_points_in_rectangle(result_queue& results, bounding_box const& query_rectangle) const
    {
        collect_points_in_rectangle(results, query_rectangle, 0, X, m_bbox);
    }

    // Return the total number of nodes in the tree.
    std::size_t number_of_nodes() const {
        return m_nodes.size();
    }

private:
    void initialize(identified_ranked_2d_point* points_begin, identified_ranked_2d_point* points_end, 
                    std::size_t index, axis_t split_axis);
    void collect_points_in_rectangle(
        result_queue& results, bounding_box const& query_rectangle, std::size_t index,
        axis_t split_axis, bounding_box const& subtree_bbox) const;

private:
    // The tree is stored in an array in the same way as a binary heap would be.
    std::vector<node> m_nodes;
    // The subtree caches associated with the nodes on the first few levels, also stored as a binary heap.
    std::vector< std::array<ranked_2d_point, cache_size> > m_caches;
    // To determine when we can use the above cache, we need to know the bounding box of the stored points.
    bounding_box m_bbox;
};

#endif // DIRK_GERRITS__PRIORITY_KD_TREE_HPP
