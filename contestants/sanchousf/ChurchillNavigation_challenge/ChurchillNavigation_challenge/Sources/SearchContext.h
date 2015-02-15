#pragma once

#include <vector>

#include "Grid.h"

//--------------------------------------------------------------------------------------------------------
extern "C" __declspec(dllexport) SearchContext*    __stdcall create(const Point* points_begin, const Point* points_end);
extern "C" __declspec(dllexport) int32_t           __stdcall search(SearchContext* sc, const Rect rect, const int32_t count, Point* out_points);
extern "C" __declspec(dllexport) SearchContext*    __stdcall destroy(SearchContext* sc);

//--------------------------------------------------------------------------------------------------------
struct SearchContext
{
private:
    typedef std::vector<Point> type_points;

public:
    SearchContext(const Point* points_begin, const Point* points_end);
    ~SearchContext();

    int32_t search(const Rect& rect, const int32_t count, Point* out_points) const;

protected:

private:
    type_points m_points;

    int32_t searchInPoints(const type_points& _tree, const Rect& rect, const int32_t count, Point* out_points) const;
    bool isInRect(const Rect& _rect, const Point& _point) const;

    TGrid m_grid;
};