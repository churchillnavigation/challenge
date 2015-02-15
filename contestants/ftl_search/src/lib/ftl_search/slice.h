#ifndef __SLICE_H__
#define __SLICE_H__

#include <stddef.h>

#include <ftl_search/point_search.h>

/**
 * Represents a slice of a larger array.
 */
struct slice_t
{
    /**
     * Constructor.
     */
    slice_t();

    /**
     * Copy constructor.
     */
    slice_t(const slice_t &other);
    slice_t &operator=(const slice_t &other);

    /**
     * Constructor.
     *
     * @param start Start of the slice.
     * @param length Length of the slice.
     */
    slice_t(Point *start, size_t length);

    /**
     * Start of the slice.
     */
    Point *m_start;

    /**
     * Length of the slice.
     */
    size_t m_length;
};

#endif /* End of __SLICE_H__ */
