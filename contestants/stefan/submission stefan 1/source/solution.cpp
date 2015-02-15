#include "solution.h"

#include <algorithm>
#include <iterator>

#include <iostream>

namespace util {

    struct is_inside {
        is_inside(const Rect rect)
            :   m_rect(rect)
        {}

        bool operator()(const Point p) const {
            return (m_rect.lx <= p.x && m_rect.hx >= p.x) &&
                    (m_rect.ly <= p.y && m_rect.hy >= p.y);
        }
    private:
        Rect m_rect;
    };

    bool point_rank_less(const Point a, const Point b) {
        return a.rank < b.rank;
    }

    float extract_x(const Point& p) {
        return p.x;
    }

    float extract_y(const Point& p) {
        return p.y;
    }
}

Solution::Solution(const Point *points_begin, const Point *points_end)
    :   m_points(points_begin, points_end)
{
    std::sort(begin(m_points), end(m_points), util::point_rank_less);

    m_x_coord.resize(m_points.size());
    std::transform(begin(m_points), end(m_points), begin(m_x_coord), util::extract_x);
    m_y_coord.resize(m_points.size());
    std::transform(begin(m_points), end(m_points), begin(m_y_coord), util::extract_y);
}

float extract_float(__m256 m, const int i) {
    float f[8];
    _mm256_store_ps(f, m);
    return f[i];
}

int32_t Solution::search(const Rect rect, const int32_t count, Point *out_points)
{
    if (count == 0) {
        return 0;
    }

    __m256 rect_lx = _mm256_broadcast_ss(&rect.lx);
    __m256 rect_hx = _mm256_broadcast_ss(&rect.hx);
    __m256 rect_ly = _mm256_broadcast_ss(&rect.ly);
    __m256 rect_hy = _mm256_broadcast_ss(&rect.hy);

    int32_t n = 0;
    for(int32_t i = 0; i < m_points.size(); i+=16) {
        __m256 x = _mm256_load_ps(m_x_coord.data() + i);
        __m256 y = _mm256_load_ps(m_y_coord.data() + i);

        __m256 x2 = _mm256_load_ps(m_x_coord.data() + i + 8);
        __m256 y2 = _mm256_load_ps(m_y_coord.data() + i + 8);

        __m256 x_in = _mm256_and_ps(_mm256_cmp_ps(rect_lx, x, _CMP_LE_OQ), _mm256_cmp_ps(rect_hx, x, _CMP_GE_OQ));
        __m256 y_in = _mm256_and_ps(_mm256_cmp_ps(rect_ly, y, _CMP_LE_OQ), _mm256_cmp_ps(rect_hy, y, _CMP_GE_OQ));

        __m256 x_in2 = _mm256_and_ps(_mm256_cmp_ps(rect_lx, x2, _CMP_LE_OQ), _mm256_cmp_ps(rect_hx, x2, _CMP_GE_OQ));
        __m256 y_in2 = _mm256_and_ps(_mm256_cmp_ps(rect_ly, y2, _CMP_LE_OQ), _mm256_cmp_ps(rect_hy, y2, _CMP_GE_OQ));

        bool according_to_sse = !_mm256_testz_ps(x_in, y_in) || !_mm256_testz_ps(x_in2, y_in2);

        if (according_to_sse)
        {
            for(int q = 0; q < 16; q++)
            {
                Point p = m_points[i+q];

                if (util::is_inside(rect)(p))
                {
                    out_points[n++] = p;
                    if (n == count) {
                        return n;
                    }
                }
            }
        }
    }
    return n;
}
