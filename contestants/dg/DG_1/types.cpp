#include <limits>
#include "types.hpp"

namespace {
    // Make the rank and id take up a single 32-bit integer, with the rank in the high bits.
    // Since the original ranks were unique, the result has the same ordering as the original ranks.
    rank_t pack_id_and_rank(std::int8_t id, std::int32_t rank) {
        std::uint8_t id_bits = reinterpret_cast<std::int8_t const&>(id);
        std::uint32_t rank_bits = static_cast<std::uint32_t>(rank);
        std::uint32_t packed_rank = (rank_bits << 8) + id_bits;
        return packed_rank;
    }

    std::int8_t unpack_id(rank_t packed_rank) {
        std::uint8_t id_bits = packed_rank & 0xFF;
        std::int8_t id = reinterpret_cast<std::int8_t const&>(id_bits);
        return id;
    }

    std::int32_t unpack_rank(rank_t packed_rank) {
        std::uint32_t rank_bits = packed_rank >> 8;
        std::int32_t rank = static_cast<std::int32_t>(rank_bits);
        return rank;
    }
} // unnamed namespace

ranked_2d_point::ranked_2d_point()
    : x(std::numeric_limits<coordinate_t>::quiet_NaN()),
      y(std::numeric_limits<coordinate_t>::quiet_NaN()),
      rank(std::numeric_limits<rank_t>::max())
{}
ranked_2d_point::ranked_2d_point(coordinate_t x_, coordinate_t y_, rank_t rank_)
    : x(x_), y(y_), rank(rank_)
{}

ranked_2d_point::ranked_2d_point(identified_ranked_2d_point const& other)
    : x(other.x), y(other.y), rank(pack_id_and_rank(other.id, other.rank))
{

}
ranked_2d_point& ranked_2d_point::operator=(identified_ranked_2d_point const& other)
{
    x = other.x;
    y = other.y;
    rank = pack_id_and_rank(other.id, other.rank);
    return *this;
}

identified_ranked_2d_point::identified_ranked_2d_point(ranked_2d_point const& other)
    : id(unpack_id(other.rank)), rank(unpack_rank(other.rank)), x(other.x), y(other.y)
{

}
identified_ranked_2d_point& identified_ranked_2d_point::operator=(ranked_2d_point const& other)
{
    id = unpack_id(other.rank); 
    rank = unpack_rank(other.rank); 
    x = other.x; 
    y = other.y;
    return *this;
}

bounding_box::bounding_box(identified_ranked_2d_point const* points_begin, identified_ranked_2d_point const* points_end)
{
    if (points_begin == points_end) { return; }
    m_bounds[MIN][X] = m_bounds[MAX][X] = points_begin->x;
    m_bounds[MIN][Y] = m_bounds[MAX][Y] = points_begin->y;
    for (auto it = std::next(points_begin); it != points_end; ++it) {
        if (it->x < m_bounds[MIN][X]) { m_bounds[MIN][X] = it->x; }
        if (it->x > m_bounds[MAX][X]) { m_bounds[MAX][X] = it->x; }
        if (it->y < m_bounds[MIN][Y]) { m_bounds[MIN][Y] = it->y; }
        if (it->y > m_bounds[MAX][Y]) { m_bounds[MAX][Y] = it->y; }
    }
}

bounding_box bounding_box::replace(side_t side, axis_t axis, coordinate_t new_value) const
{
    bounding_box result(*this);
    result[side][axis] = new_value;
    return result;
}

axis_t bounding_box::longest_extent_axis() const
{
    if (extent_along_axis(X) < extent_along_axis(Y)) {
        return Y;
    } else {
        return X;
    }
}

coordinate_t bounding_box::extent_along_axis(axis_t axis) const
{
    return m_bounds[MAX][axis] - m_bounds[MIN][axis];
}

coordinate_t bounding_box::axis_midpoint(axis_t axis) const
{
    return m_bounds[MIN][axis] + (m_bounds[MAX][axis] - m_bounds[MIN][axis]) / 2;
}

bool bounding_box::contains(coordinate_t x, coordinate_t y) const
{
    return m_bounds[MIN][X] <= x && x <= m_bounds[MAX][X] && m_bounds[MIN][Y] <= y && y <= m_bounds[MAX][Y];
}
