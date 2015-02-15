#include <algorithm>
#include "result_queue.hpp"

result_queue::result_queue(std::size_t max_size)
{
    fill_with_sentinel_points(max_size);
}

bool result_queue::filled_with_points_of_smaller_rank_than(ranked_2d_point const& point) const
{
    return m_heap.top().rank < point.rank;
}

void result_queue::add(ranked_2d_point const& point)
{
    // Replace the greatest ranked point in the queue with the given `point`, which is assumed to have a smaller rank.
    auto top = heap_t::s_handle_from_iterator(m_heap.begin());
    m_heap.decrease(top, point);
}

std::size_t result_queue::move_sorted_contents_to(identified_ranked_2d_point* output_iterator)
{
    remove_sentinel_points();

    // Since we're using a max heap we have to store the points in reverse to get a smallest-to-largest ordering.
    std::size_t count = m_heap.size();
    output_iterator += count;
    while (!m_heap.empty()) {
        --output_iterator;
        *output_iterator = m_heap.top();
        m_heap.pop();
    }
    return count;
}

void result_queue::fill_with_sentinel_points(std::size_t num_points)
{
    while (num_points-- > 0) {
        // Default constructed points function as sentinels, having NaN coordinates and the highest possible rank.
        m_heap.emplace(); 
    }
}

void result_queue::remove_sentinel_points()
{
    rank_t sentinel_rank = std::numeric_limits<rank_t>::max();
    while (!m_heap.empty() && m_heap.top().rank == sentinel_rank) {
        m_heap.pop();
    }
}