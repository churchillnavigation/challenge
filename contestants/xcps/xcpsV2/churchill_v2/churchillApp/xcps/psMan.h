#pragma once

#include "xcps.h"
#include "xcps2.h"

#define NLAYERS             7
#define MAX_SEARCH_POINTS   32

// search manager class
struct SearchContext {
    cPointSearch_t<NLAYERS, MAX_SEARCH_POINTS>* pPs;
    cPointSearch_t<10, MAX_SEARCH_POINTS>* pPsL10;
    cPointSearch_t<9, MAX_SEARCH_POINTS>* pPsL9;
    cPointSearch_t<8, MAX_SEARCH_POINTS>* pPsL8;
    cPointSearch_t<7, MAX_SEARCH_POINTS>* pPsL7;

    cPointSearchSimple_t* pSimple;

    SearchContext(int dataCount, const Point* points_begin);
    ~SearchContext();

    int32_t find2(const Rect& r, const int32_t count, Point* out_points);

    int32_t (SearchContext::*find)(const Rect& r, const int32_t count, Point* out_points);

    int32_t useL10(const Rect& r, const int32_t count, Point* out_points);
    int32_t useL9(const Rect& r, const int32_t count, Point* out_points);
    int32_t useL8(const Rect& r, const int32_t count, Point* out_points);
    int32_t useL7(const Rect& r, const int32_t count, Point* out_points);
    int32_t useSimple(const Rect& r, const int32_t count, Point* out_points);
};