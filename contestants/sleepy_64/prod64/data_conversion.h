#pragma once
#pragma once
#include "extern/point_search.h"

extern "C"
{
    __declspec(dllexport) void* __stdcall create(const Point* points_begin, const Point* points_end);
    __declspec(dllexport) int32_t __stdcall search(void* sc, const Rect rect, const int32_t count, Point* out_points);
    __declspec(dllexport) void * __stdcall destroy(void* sc);
}
