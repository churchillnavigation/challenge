#pragma once

#include "HybridSearch.h"

DLL_FUNCTION SearchContext* DLL_FUNCTION_POST create(const Point* points_begin, const Point* points_end);
DLL_FUNCTION int32_t DLL_FUNCTION_POST search(SearchContext* sc, const Rect rect, const int32_t count, Point* out_points);
DLL_FUNCTION SearchContext* DLL_FUNCTION_POST destroy(SearchContext* sc);
