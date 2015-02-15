#ifndef FASTSEARCH_HPP_
#define FASTSEARCH_HPP_

#include "point_search.h"

extern "C" {
struct SearchContext;

__declspec(dllexport) SearchContext *__stdcall create(const Point* points_begin, const Point* points_end);
__declspec(dllexport) int32_t __stdcall search(SearchContext *sc, const Rect rect, const int32_t count, Point* out_points);
__declspec(dllexport) SearchContext *__stdcall destroy(SearchContext* sc);
}


#endif /* FASTSEARCH_HPP_ */
