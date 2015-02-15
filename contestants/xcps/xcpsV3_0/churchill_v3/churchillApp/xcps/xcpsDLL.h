#include "../../point_search.h"

#define DLLEXPORT extern "C" __declspec(dllexport) 

DLLEXPORT SearchContext* __stdcall create(const Point* points_begin, const Point* points_end);
DLLEXPORT int32_t __stdcall search(SearchContext* sc, const Rect rect, const int32_t count, Point* out_points);
DLLEXPORT SearchContext* __stdcall destroy(SearchContext* sc);
