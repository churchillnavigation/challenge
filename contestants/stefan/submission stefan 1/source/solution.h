#ifndef SOLUTION_H
#define SOLUTION_H

#include "point_search.h"

#include <vector>

typedef int32_t point_index;

/**
 * An simple struct that stored an single coordinate (x or y) and one index
 */
struct pair_pos_index {
    pair_pos_index(float pos, point_index index)
        :   pos(pos)
        ,   index(index)
    {}

    friend bool operator<(const pair_pos_index& a, const pair_pos_index& b)
    {
        return a.pos < b.pos;
    }

    float pos;
    point_index index;
};

class Solution {
public:
    Solution(const Point *points_begin, const Point *points_end);
    ~Solution() {}

    int32_t search(const Rect rect, const int32_t count, Point *out_points);

private:
    std::vector<Point> m_points;
    std::vector<float> m_x_coord;
    std::vector<float> m_y_coord;
};



#endif // SOLUTION_H
