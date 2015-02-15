#ifndef __POINT_SET__
#define __POINT_SET__

#include <set>

#include <ftl_search/point_search.h>
#include <ftl_search/slice.h>

/**
 * Determines whether a point is inside a rect.
 *
 * @param r The rect to test.
 * @param p The point that is tested.
 *
 * @return True if the p is inside the rect (inclusive),
 *         false otherwise.
 */
bool contains(const Rect &r, const Point &p);

/**
 * This comparator is used to sort points inside
 * a set (and also to see if they are equal). It
 * sorts first by rank followed by id, x, and y.
 * This means that we can quickly find the maximum
 * element inside a set.
 */
struct point_cmp
{
    bool operator()(const Point &a, const Point &b);
};

/**
 * A set that holds a up to a fixed number of Points.
 * The purpose is to hold the points with lowest rank
 * inside a specific rectangle.
 */
class point_set
{
public:
    /**
     * Constructor.
     *
     * @param max_size The maximum number of elements that
     *                 this set holds.
     */
    point_set(const Rect &r, size_t max_size);

    /**
     * Returns the maximum rank of the elements in this set.
     *
     * @return the maximum rank of the elements in this set.
     */
    int32_t max();

    /**
     * Adds a list set of points to the set.
     *
     * @param points The points to add.
     */
    void add(slice_t &points);

    /**
     * Returns the number of points currently inside the set.
     *
     * @return the number of points in the set.
     */
    size_t size();

    /**
     * Writes the points to the given array and returns the
     * number of points written.
     *
     * @param out The output buffer.
     * @param count Length of the output buffer.
     *
     * @return Number of items written to the output buffer.
     */
    size_t write(Point *out, size_t count);

private:
    /**
     * The rectangle all points must be in.
     */
    Rect m_rect;

    /**
     * The underlying set where all points are stored.
     */
    std::set<Point, point_cmp> m_set;

    /**
     * The maximum size of the set.
     */
    size_t m_max_size;
};

#endif /* End of __POINT_SET__ */
