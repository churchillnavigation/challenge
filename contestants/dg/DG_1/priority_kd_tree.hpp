#ifndef DIRK_GERRITS__PRIORITY_KD_TREE_HPP
#define DIRK_GERRITS__PRIORITY_KD_TREE_HPP

#include <vector>
#include "heap_structure.hpp"
#include "types.hpp"
class result_queue;

// A hybrid between a priority search tree and a kd-tree.
//
// A priority search tree would allow the 1-dimensional variant of the problem to be solved efficiently.  Its root node
// contains the point with the smallest rank, and the remainder of the points is split at the median x-coordinate and
// divided over two recursively defined priority search trees which form the node's children.  To generalize this to 
// 2-dimensional points, we instead split based on x-coordinate at even levels of the tree but on y-coordinate at odd 
// levels of the tree, as is done in a kd-tree.
class priority_kd_tree : private heap_structure<priority_kd_tree>
{
    struct node {
        ranked_2d_point point;
        coordinate_t split;
    };
public:
    // Construct a tree storing the given points.
    explicit priority_kd_tree(identified_ranked_2d_point* points_begin, identified_ranked_2d_point* points_end);

    // Add the stored points which lie in the given rectangle into the given results queue.
    void collect_points_in_rectangle(result_queue& results, bounding_box const& query_rectangle) const
    {
        collect_points_in_rectangle(results, query_rectangle, 0, X);
    }

    // Return the total number of nodes in the tree.
    std::size_t number_of_nodes() const {
        return m_nodes.size();
    }

private:
    void initialize(identified_ranked_2d_point* points_begin, identified_ranked_2d_point* points_end, 
                    std::size_t index, axis_t split_axis);
    void collect_points_in_rectangle(result_queue& results, bounding_box const& query_rectangle, 
                                     std::size_t index, axis_t split_axis) const;

private:
    // The tree is stored in an array in the same way as a binary heap would be.
    std::vector<node> m_nodes;
};

#endif // DIRK_GERRITS__PRIORITY_KD_TREE_HPP
