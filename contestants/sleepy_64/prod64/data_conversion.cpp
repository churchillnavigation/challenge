#include "data_conversion.h"
#include "data_conversion.h"
#include "tree64.h"
#include <algorithm>

extern "C" __declspec(dllexport) void* __stdcall create(const Point* points_begin, const Point* points_end)
{
    std::vector<search2d::FoundPoint> points;
    points.reserve(points_end - points_begin);
    for (auto p = points_begin; p != points_end; ++p)
    {
        points.emplace_back(search2d::FoundPoint{ geom2d::Point{ p->x, p->y }, p->rank, p->id });
    }
    auto p = search2d::create_context(points);

    return p;
}

extern "C" __declspec(dllexport) int32_t __stdcall search(void* sc, const Rect rect, const int32_t count, Point* out_points)
{
    if (count <= 0) return 0;
    auto p = (search2d::SearchContext*)sc;
    auto r = geom2d::Rectangle{ geom2d::Point{ rect.lx, rect.ly }, geom2d::Point{ rect.hx, rect.hy } };
    std::vector<search2d::FoundPoint> result;
    search2d::search(*p, r, count, result);
    size_t len = std::min((size_t)count, result.size());
    for (size_t i = 0; i < len; ++i)
    {
        const auto& src = result[i];
        Point pt = Point{ src.payload, src.rank, src.p.x, src.p.y };
        out_points[i] = pt;
    }
    return (int32_t)len;
}

extern "C" __declspec(dllexport) void * __stdcall destroy(void* sc)
{
    auto p = (search2d::SearchContext*)sc;
    delete p;
    return nullptr;
}

