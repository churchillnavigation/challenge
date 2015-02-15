#ifndef __DIM_H__
#define __DIM_H__

#include <ftl_search/slice.h>
#include <ftl_search/point_search.h>

/**
 * Represents a split in some dimension.
 */
struct dim_split
{
    /**
     * The points to the left.
     */
    slice_t m_left;

    /**
     * The points to the right.
     */
    slice_t m_right;
};

/**
 * Given a set of points this function finds the
 * smallest rectangle that contains them.
 *
 * @param points A list of points.
 *
 * @return The smallest rectangle that contains
 *         all of the points.
 */
Rect find_surrounding_rect(slice_t &points);

/**
 * Given a set of points this function sorts them
 * and splits them into two sets, according to the
 * dimension in which the spread is largerst.
 *
 * @param points The points to split.
 * @param r A rect that contains all of the points.
 *
 * @return The splitted points.
 */
dim_split split_dim(slice_t &points, Rect &r);

#endif /* End of __DIM_H__ */
