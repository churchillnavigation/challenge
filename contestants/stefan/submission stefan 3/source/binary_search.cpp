#include "binary_search.h"

#include <algorithm>

point_index bin_search::lower_bound(float value,
                                    point_index first, point_index last) const
{
    auto it = std::lower_bound(begin(m_values) + first,
                               begin(m_values) + last,
                               value,
                               [](float a, float b){return a < b;});
    return (point_index)std::distance(begin(m_values), it);
}

point_index bin_search::upper_bound(float value,
                                    point_index first, point_index last) const
{
    auto it = std::upper_bound(begin(m_values) + first,
                               begin(m_values) + last,
                               value,
                               [](float a, float b){return a < b;});
    return (point_index)std::distance(begin(m_values), it);
}
