#ifndef DIRK_GERRITS__TYPES_HPP
#define DIRK_GERRITS__TYPES_HPP

#include <cstddef>
#include <cstdint>
#include <array>

typedef std::int8_t identifier_t;
typedef std::uint32_t rank_t;
typedef float coordinate_t;

enum axis_t { X, Y };
enum side_t { MIN, MAX };

struct ranked_1d_point {
    ranked_1d_point() 
    {}
    ranked_1d_point(coordinate_t y_, rank_t rank_)
        : y(y_), rank(rank_)
    {}
    coordinate_t y;
    rank_t rank;
};

struct identified_ranked_2d_point;
struct ranked_2d_point
{
    ranked_2d_point();
    ranked_2d_point(coordinate_t x_, coordinate_t y_, rank_t rank_);

    // Pack 8-bit id and 24-bit rank together into a 32-bit rank.
    ranked_2d_point(identified_ranked_2d_point const& other);
    ranked_2d_point& operator=(identified_ranked_2d_point const& other);

    coordinate_t  operator[](axis_t axis) const { return (&x)[axis]; }
    coordinate_t& operator[](axis_t axis)       { return (&x)[axis]; }

    coordinate_t x, y;
    rank_t rank;
};

#pragma pack(push, 1)

struct identified_ranked_2d_point {
    // Unpack id and rank again.
    identified_ranked_2d_point(ranked_2d_point const& other);
    identified_ranked_2d_point& operator=(ranked_2d_point const& other);

    coordinate_t  operator[](axis_t axis) const { return (&x)[axis]; }
    coordinate_t& operator[](axis_t axis)       { return (&x)[axis]; }

    std::int8_t id;
    std::int32_t rank;
    coordinate_t x;
    coordinate_t y;
};

class bounding_box {
public:
    // Construct the smallest bounding box containing the given points.
    bounding_box(identified_ranked_2d_point const* points_begin, identified_ranked_2d_point const* points_end);

    // Return a new bounding box identical to this one, but with one side position changed.
    bounding_box replace(side_t side, axis_t axis, coordinate_t new_value) const;

    // Return the axis upon which the bounding box has the longest projection.
    axis_t longest_extent_axis() const;

    // Return the length of the projection of the bounding box onto the given axis.
    coordinate_t extent_along_axis(axis_t axis) const;

    // Return the middle of the projection of the bounding box onto the given axis.
    coordinate_t axis_midpoint(axis_t axis) const;

    // Return whether the bounding box contains the given point.
    bool contains(coordinate_t x, coordinate_t y) const;
    template <class Point> bool contains(Point&& p) const {
        return contains(p.x, p.y);
    }

    std::array<coordinate_t, 2> const& operator[](side_t side) const { return m_bounds[side]; }
    std::array<coordinate_t, 2>&       operator[](side_t side)       { return m_bounds[side]; }

private:
    // bounds[MIN][X], bounds[MIN][Y], bounds[MAX][X], bounds[MAX][Y]
    std::array<std::array<coordinate_t, 2>, 2> m_bounds; 
};

#pragma pack(pop)

#endif // DIRK_GERRITS__TYPES_HPP
