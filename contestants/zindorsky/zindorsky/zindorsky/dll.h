#pragma once
#include "point_search.h"

#ifdef ZINDORSKY_EXPORTS
	#define ZINDORSKY_API __declspec(dllexport)
#else
	#define ZINDORSKY_API __declspec(dllimport)
#endif

extern "C" {

ZINDORSKY_API SearchContext* create(const Point* points_begin, const Point* points_end);
ZINDORSKY_API int32_t search(SearchContext* sc, const Rect rect, const int32_t count, Point* out_points);
ZINDORSKY_API SearchContext* destroy(SearchContext* sc);

}	//extern "C"

