#pragma once

#include "xcpsDLL.h"
#include "../../point_search.h"
#include "xcError.h"
#include <list>
#include <iostream>
#include <fstream>
#include <sstream>
#include <limits>
#include "smmintrin.h"
#include "mmintrin.h"
#include "xcpsComm.h"

struct myPoint_t {
    float x;
    float y;
    int8_t id;
};

class cPointSearchSimple_t {
public:

    int32_t maxSrcCount;    // maximum available source data count;
    
    myPoint_t* pMyPoint;
    indexRank_t* pIndexRankBase;
    
    //--------------------------------------------------------------------------------
    cPointSearchSimple_t(int32_t count, const Point* points_begin);
    ~cPointSearchSimple_t();

    void bubbleDown(__int64* pHeapBase, int32_t heapSize, int32_t pos);
    void heapSort(__int64* pHeapBase, int32_t heapSize);

    void searchPoints();

    int32_t find(const Rect& r, const int32_t count, Point* out_points);
    void add(const Point* pt );
};

