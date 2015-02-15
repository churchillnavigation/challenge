#ifndef DIRK_GERRITS__RESULT_QUEUE_HPP
#define DIRK_GERRITS__RESULT_QUEUE_HPP

#include <boost/heap/pairing_heap.hpp>
#include "types.hpp"

// A priority queue for storing ranked 2-dimensional points.  Of all the points added to it over time, it will only keep
// the `max_size` points with the smallest ranks.
class result_queue {
public:
    // Create a queue capable of holding at most `max_size` points.
    explicit result_queue(std::size_t max_size);

    // Return true if the queue is full, and all its points have lesser rank than the given `point`.  In other words, 
    // return true if trying to add the given `point` would have no effect.
    bool filled_with_points_of_smaller_rank_than(ranked_2d_point const& point) const;

    // Add the given `point` to the queue.  Assumes that the queue isn't full yet, or that the `point` has a lesser rank
    // than at least some point already in the queue, so use the function above first.
    void add(ranked_2d_point const& point);

    // Empty the queue into the given output iterator, storing the points there in sorted order.
    std::size_t move_sorted_contents_to(identified_ranked_2d_point* output_iterator);

private:
    class smaller_rank {
    public:
        bool operator()(ranked_2d_point const& p1, ranked_2d_point const& p2) const {
            return p1.rank < p2.rank;
        }
    };

    // Fill the queue with "sentinel" points, which have a rank higher than any real point.  This means when we insert new
    // points we don't have to deal with the case where the queue isn't full yet.
    void fill_with_sentinel_points(std::size_t num_points);

    // Remove the sentinel points again, lest we inadvertently output them.
    void remove_sentinel_points();

private:
    using heap_t = boost::heap::pairing_heap<ranked_2d_point, boost::heap::compare<smaller_rank> >;
    heap_t m_heap;
};

#endif // DIRK_GERRITS__RESULT_QUEUE_HPP
