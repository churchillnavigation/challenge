#include "Grid.h"

#include <algorithm>

TGrid::TGrid(const Point* _first, const Point* _last)
{
    if (!_first) return;

    size_t count = _last - _first;
    size_t points_per_col = count / grid_width;
    size_t points_per_row = count / grid_height;
    size_t points_per_cell = count / grid_count;

    type_points points(_first, _last);
    std::sort(points.begin(), points.end(), [](const Point& _left, const Point& _right) { return _left.x < _right.x; });
    for (size_t i = 0; i < grid_width; ++i)
        m_col_pos[i] = points[i*points_per_col].x;
    m_col_pos[grid_width] = (--points.end())->x;

    std::sort(points.begin(), points.end(), [](const Point& _left, const Point& _right) { return _left.y < _right.y; });
    for (size_t i = 0; i < grid_height; ++i)
        m_row_pos[i] = points[i*points_per_row].y;
    m_row_pos[grid_height] = (--points.end())->y;

    /*
    m_array_size.lx = m_array_size.hx = _first->x;
    m_array_size.ly = m_array_size.hy = _first->y;
    for (const Point* i = (_first+1); i < _last; ++i)
    {
        if (i->x < m_array_size.lx)
            m_array_size.lx = i->x;
        else if (i->x > m_array_size.hx)
            m_array_size.hx = i->x;

        if (i->y < m_array_size.ly)
            m_array_size.ly = i->y;
        else if (i->y > m_array_size.hy)
            m_array_size.hy = i->y;
    }

    m_array_width  = m_array_size.hx - m_array_size.lx;
    m_array_height = m_array_size.hy - m_array_size.ly;
    */
    for (const Point* i = _first; i < _last; ++i)
    {
        GridPoint gridPoint = FoundCell(*i);

        m_grid[gridPoint.x][gridPoint.y].push_back(*i);
    }

    for (auto& col_i : m_grid)
        for (auto& cell_i : col_i)
            std::sort(cell_i.begin(), cell_i.end(), [](const Point& _left, const Point& _right) { return _left.rank < _right.rank; });

}

TGrid::GridPoint TGrid::FoundCell(const Point& _point) const
{
    return FoundCell(_point.x, _point.y);
}

TGrid::GridPoint TGrid::FoundCell(const float _x, const float _y) const
{
    auto x_i = std::lower_bound(m_col_pos.cbegin(), m_col_pos.cend(), _x, [](const float _left, const float _right) { return _left < _right; });
    auto y_i = std::lower_bound(m_row_pos.cbegin(), m_row_pos.cend(), _y, [](const float _left, const float _right) { return _left < _right; });

    GridPoint result;
    result.x = x_i - m_col_pos.cbegin();
    result.y = y_i - m_row_pos.cbegin();

    if (result.x >= grid_width) result.x = grid_width - 1;
    if (result.y >= grid_height) result.y = grid_height - 1;

    return result;
}

int32_t TGrid::search(const Rect& rect, const int32_t count, Point* out_points) const
{
    GridPoint l_point = FoundCell(rect.lx, rect.ly);
    GridPoint h_point = FoundCell(rect.hx, rect.hy);;

    size_t x_count = h_point.x - l_point.x + 1;
    size_t y_count = h_point.y - l_point.y + 1;

    const size_t grid_rect_count = x_count * y_count;
    std::vector<type_points> cell_result(grid_rect_count);

    for (size_t x = 0; x < x_count; ++x)
        for (size_t y = 0; y < y_count; ++y)
        {
            const bool is_edge = (x == 0) || (y == 0) || (x == (x_count - 1)) || (y == (y_count - 1));
            searchInPoints(m_grid[l_point.x + x][l_point.y + y], rect, count, cell_result[y + x*y_count], is_edge);
        }

    type_points result;
    for (const auto& i : cell_result)
        result.insert(result.end(), i.begin(), i.end());
    std::sort(result.begin(), result.end(), [](const Point& _left, const Point& _right) { return _left.rank < _right.rank; });

    int32_t count_result = 0;
    for (; (count_result < count) && (count_result<result.size()); ++count_result)
        out_points[count_result] = result[count_result];

    return count_result;
}

void TGrid::searchInPoints(const type_points& _points, const Rect& rect, const int32_t count, type_points& out_points, const bool check_isInRect) const
{
    if (check_isInRect)
    {
        int32_t found_n = 0;

        for (const auto& i : _points)
        {
            if (found_n >= count) break;

            if (isInRect(rect, i))
            {
                out_points.push_back(i);
                ++found_n;
            }
        }
    }
    else
    {
        if (count <= _points.size())
            out_points.assign(_points.begin(), _points.begin() + count);
        else
            out_points = _points;
    }
}

bool TGrid::isInRect(const Rect& _rect, const Point& _point) const
{
    return (_point.x >= _rect.lx) && (_point.y >= _rect.ly) && (_point.x <= _rect.hx) && (_point.y <= _rect.hy);
}