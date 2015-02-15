#ifndef DIRK_GERRITS__HEAP_STRUCTURE_HPP
#define DIRK_GERRITS__HEAP_STRUCTURE_HPP

#include <cmath>

// A helper base-class for representing complete binary trees using an array, in the same way as a binary heap
// would be represented.
//
// Derived = Class inheriting this class (curiously recurring template pattern).
//           Must define a `number_of_nodes() const` function whose return value is convertible to `std::size_t`.
template<class Derived>
class heap_structure {
    static std::size_t floored_log2(std::size_t x) {
        static double const log2 = std::log(2);
        return (std::size_t)std::floor(std::log(x) / log2);
    }
    // Return the total number of nodes.
    std::size_t number_of_nodes() const {
        return ((Derived const*)this)->number_of_nodes();
    }
protected:
    // Return the number of internal nodes.
    std::size_t number_of_internal_nodes() const {
        return number_of_internal_nodes_for_leaves(number_of_leaf_nodes());
    }
    // Return the number of leaf nodes.
    std::size_t number_of_leaf_nodes() const {
        return leaves_in_subtree_with_root(0);
    }

    // Return the index of the left child of `(*this)[parent_index]`.
    std::size_t left_child(std::size_t parent_index) const {
        return 2 * parent_index + 1;
    }
    // Return the index of the right child of `(*this)[parent_index]`.
    std::size_t right_child(std::size_t parent_index) const {
        return 2 * parent_index + 2;
    }
    // Return whether `(*this)[parent_index]` has a left child.
    bool has_left_child(std::size_t parent_index) const {
        return left_child(parent_index) < number_of_nodes();
    }
    // Return whether `(*this)[parent_index]` has a right child.
    bool has_right_child(std::size_t parent_index) const {
        return right_child(parent_index) < number_of_nodes();
    }

    // Return whether `(*this)[index]` is an internal node.
    bool is_internal_node(std::size_t index) const {
        return has_left_child(index);
    }
    // Return whether `(*this)[index]` is a leaf node.
    bool is_leaf(std::size_t index) const {
        return !has_left_child(index);
    }

    // Return the level of the tree at which `(*this)[index]` lies (the root has level 0).
    std::size_t level_of(std::size_t index) const {
        // if there are i = 2^h - 1 nodes upto level h, then h = log2(i + 1)
        return floored_log2(index + 1);
    }

    // Return the total number of levels in the tree.
    std::size_t num_levels() const {
        return number_of_nodes() == 0 ? 0 : level_of(number_of_nodes() - 1) + 1;
    }

    // Return the number of internal nodes that a binary heap has if it has `num_leaves` leaves.
    static std::size_t number_of_internal_nodes_for_leaves(std::size_t num_leaves) {
        return num_leaves - 1;
    }
    // Return the total number of nodes that a binary heap has if it has `num_leaves` leaves.
    static std::size_t number_of_nodes_for_leaves(std::size_t num_leaves) {
        return 2 * num_leaves - 1;
    }

    // Return the number of elements in the subtree rooted at `(*this)[index]`.
    std::size_t size_of_subtree_with_root(std::size_t index) const;

    // Return the number of leaves in the subtree rooted at `(*this)[index]`.
    std::size_t leaves_in_subtree_with_root(std::size_t index) const;

    // Return the number of elements on the lowest level of the tree which are in the subtree rooted at `(*this)[index]`.
    std::size_t nodes_at_lowest_level_under(std::size_t index) const;
};

template<class Derived>
std::size_t heap_structure<Derived>::size_of_subtree_with_root(std::size_t index) const {
    if (index >= number_of_nodes()) {
        return 0;
    } else {
        std::size_t lowest_level = num_levels() - 1;
        std::size_t steps_to_lowest_level = lowest_level - level_of(index);
        std::size_t nodes_before_lowest_level = // 2^i nodes on level i, so this is the sum of a geometric series: 
            (1 << steps_to_lowest_level) - 1; // sum_{i=0}^{k} 2^i = 2^(k-1) - 1
        std::size_t nodes_on_lowest_level = nodes_at_lowest_level_under(index);
        return nodes_before_lowest_level + nodes_on_lowest_level;
    }
}

template<class Derived>
std::size_t heap_structure<Derived>::leaves_in_subtree_with_root(std::size_t index) const {
    if (index >= number_of_nodes()) {
        return 0; // subtree doesn't exist
    } else if (is_leaf(index)) {
        return 1; // subtree is just a leaf
    } else {
        std::size_t lowest_level = num_levels() - 1;
        std::size_t steps_to_lowest_level = lowest_level - level_of(index);
        std::size_t lowest_level_leaves = nodes_at_lowest_level_under(index);
        std::size_t parents_of_lowest_level_leaves = (lowest_level_leaves + 1) / 2;
        std::size_t second_lowest_level_nodes = 1 << (steps_to_lowest_level - 1);
        std::size_t second_lowest_level_leaves = second_lowest_level_nodes - parents_of_lowest_level_leaves;
        return lowest_level_leaves + second_lowest_level_leaves;
    }
}

template<class Derived>
std::size_t heap_structure<Derived>::nodes_at_lowest_level_under(std::size_t index) const {
    std::size_t lowest_level = num_levels() - 1;
    std::size_t steps_to_lowest_level = lowest_level - level_of(index);
    std::size_t leftmost_index_on_lowest_level = index;
    std::size_t rightmost_index_on_lowest_level = index;
    while (steps_to_lowest_level-- > 0) {
        leftmost_index_on_lowest_level = left_child(leftmost_index_on_lowest_level);
        rightmost_index_on_lowest_level = right_child(rightmost_index_on_lowest_level);
    }

    if (leftmost_index_on_lowest_level >= number_of_nodes()) {
        return 0;
    }
    else if (rightmost_index_on_lowest_level >= number_of_nodes()) {
        return number_of_nodes() - leftmost_index_on_lowest_level;
    } else {
        return (rightmost_index_on_lowest_level + 1) - leftmost_index_on_lowest_level;
    }
}

#endif // DIRK_GERRITS__HEAP_STRUCTURE_HPP
