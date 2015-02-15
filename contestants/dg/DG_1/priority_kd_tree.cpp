#include "priority_kd_tree.hpp"
#include "result_queue.hpp"

namespace {
    // X -> Y, Y -> X
    inline axis_t next_axis(axis_t axis) {
        return static_cast<axis_t>(1 - axis);
    }

} // unnamed namespace

priority_kd_tree::priority_kd_tree(identified_ranked_2d_point* points_begin, identified_ranked_2d_point* points_end)
    : m_nodes(std::distance(points_begin, points_end))
{
    if (m_nodes.empty()) {
        // Use a single (dummy) node to represent an empty point set.  This avoids the need to check whether the root of
        // the tree exists when we initiate a query.
        m_nodes.emplace_back();
        auto& root = m_nodes.back();
        root.split = std::numeric_limits<coordinate_t>::quiet_NaN();
    } else {
        initialize(points_begin, points_end, 0, X);
    }
}

void priority_kd_tree::initialize(
    identified_ranked_2d_point* points_begin, identified_ranked_2d_point* points_end, 
    std::size_t index, axis_t split_axis)
{
    if (points_begin == points_end) { return; }

    // Find the remaining point with minimum rank, store it in the node, and remove it from the point set.
    auto& node = m_nodes[index];
    auto minimum_rank_point = std::min_element(points_begin, points_end, 
        [](identified_ranked_2d_point const& p1, identified_ranked_2d_point const& p2) {
            return p1.rank < p2.rank;
    });
    node.point = *minimum_rank_point;
    std::rotate(minimum_rank_point, std::next(minimum_rank_point), points_end);
    --points_end;

    // Initialize the node's splitting coordinate, and partition the remaining points over its children (assuming there
    // are two children; for one child or less there is no need to partition).
    std::size_t num_points = std::distance(points_begin, points_end);
    std::size_t num_left_subtree_points = size_of_subtree_with_root(left_child(index));
    std::size_t num_right_subtree_points = size_of_subtree_with_root(right_child(index));
    std::size_t num_subtree_points = size_of_subtree_with_root(index);
    if (num_left_subtree_points == 0) {
        // No children: pick NaN as a splitting coordinate, so the search will not go deeper.
        node.split = std::numeric_limits<coordinate_t>::quiet_NaN();
    } else if (num_left_subtree_points == num_points) {
        // Only a left child: pick +infinity as a splitting coordinate, so the search will not go to the right.
        node.split = std::numeric_limits<coordinate_t>::infinity();
    } else {
        // Both a left and a right child: pick the splitting coordinate to be the smallest coordinate in the
        // right subtree.  The nodes in the left subtree then have lesser or equal coordinates to those in
        // the right subtree, as required.
        std::nth_element(points_begin, std::next(points_begin, num_left_subtree_points), points_end, 
            [=](identified_ranked_2d_point const& p1, identified_ranked_2d_point const& p2) {
                return p1[split_axis] < p2[split_axis];
        });
        auto& first_point_of_right_subtree = *std::next(points_begin, num_left_subtree_points);
        node.split = first_point_of_right_subtree[split_axis];
    }

    // Initialize the child subtrees recursively.
    auto points_middle = std::next(points_begin, num_left_subtree_points);
    initialize(points_begin, points_middle, left_child(index), next_axis(split_axis));
    initialize(points_middle, points_end, right_child(index), next_axis(split_axis));
}

void priority_kd_tree::collect_points_in_rectangle(
    result_queue& results, bounding_box const& query_rectangle, std::size_t index, axis_t split_axis) const
{
    // If the current node's point lies in the query rectangle try to add it to the result queue.  If the result queue
    // was already full with points of lesser rank then we can stop, because all points deeper in the tree will have an
    // even greater rank than the current node.
    auto& node = m_nodes[index];
    if (query_rectangle.contains(node.point)) {
        if (results.filled_with_points_of_smaller_rank_than(node.point)) {
            return;
        } else {
            results.add(node.point);
        }
    }
    
    // Recurse on the child(ren) that could have points in the query rectangle.  Note that the split coordinates
    // have been initialized in such a way that this will never recurse into a non-existent child node.
    if (query_rectangle[MIN][split_axis] <= node.split) {
        collect_points_in_rectangle(results, query_rectangle, left_child(index), next_axis(split_axis));
    }
    if (node.split <= query_rectangle[MAX][split_axis]) {
        collect_points_in_rectangle(results, query_rectangle, right_child(index), next_axis(split_axis));
    }
}
