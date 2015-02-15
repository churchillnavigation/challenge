#pragma once

#include "point_search.h"

#include <vector>
#include <array>

class TGrid
{
public:
    struct GridPoint
    {
        size_t x;
        size_t y;
    };

private:
    enum { grid_width = 10, grid_height = 10, grid_count = grid_width*grid_height };

    typedef std::vector<Point> type_points;
    typedef std::vector<float> type_cell_size;

    typedef std::array<type_points, grid_width> type_grid_column;
    typedef std::array<type_grid_column, grid_height> type_grid;

public:
    TGrid(const Point* _first, const Point* _last);

    int32_t search(const Rect& rect, const int32_t count, Point* out_points) const;

protected:

private:
    Rect m_array_size;
    float m_array_width;
    float m_array_height;

    type_grid m_grid;
    std::array<float, grid_width + 1> m_col_pos;
    std::array<float, grid_height + 1> m_row_pos;

    void searchInPoints(const type_points& _points, const Rect& rect, const int32_t count, type_points& out_points, const bool check_isInRect) const;
    bool isInRect(const Rect& _rect, const Point& _point) const;

    GridPoint FoundCell(const Point& _point) const;
    GridPoint FoundCell(const float _x, const float _y) const;
};