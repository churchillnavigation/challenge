#ifndef DIRK_GERRITS__DLL_MAIN_HPP
#define DIRK_GERRITS__DLL_MAIN_HPP

#include "priority_kd_tree.hpp"
#include "types.hpp"
using search_structure = priority_kd_tree;

extern "C" {

    // Load the provided points into an internal data structure.  The pointers follow the STL iterator convention, where
    // `points_begin` points to the first element, and `points_end` points to one past the last element.  The input points
    // are only guaranteed to be valid for the duration of the call.  Return a pointer to the data structure that can be
    // used for consecutive searches on the data.
    search_structure* __stdcall create(identified_ranked_2d_point* points_begin, identified_ranked_2d_point* points_end);

    // Search the data structure for the (at most) `count` points with the smallest ranks inside the `query_rectangle`, and
    // copy them in order of smallest to largest rank into `out_points`, which must point to a memory buffer capable of 
    // storing `count` points.  Return the number of points copied.
    int32_t __stdcall search(search_structure* data_structure, bounding_box query_rectangle, std::int32_t count,
                             identified_ranked_2d_point* out_points);

    // Release the resources associated with the data structure.  Returns a null pointer to indicate success.
    search_structure* __stdcall destroy(search_structure* data_structure);
}

#endif // DIRK_GERRITS__DLL_MAIN_HPP
