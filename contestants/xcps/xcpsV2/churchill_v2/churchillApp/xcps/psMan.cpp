#pragma once

#include "stdafx.h"
#include "psMan.h"

//#define USE_NLAYERS

// search manager class
SearchContext::SearchContext(int dataCount, const Point* points_begin) {
    pPs = 0;
    pPsL10 = 0;
    pPsL9 = 0;
    pPsL8 = 0;
    pPsL7 = 0;
    pSimple = 0;

#ifdef USE_NLAYERS
    pPs = new cPointSearch_t<NLAYERS, MAX_SEARCH_POINTS>(dataCount, points_begin);
    find = &SearchContext::find2;
#else
    // The threshold is tuned based testing on my machine. In theory, it should
    // be tested on the target machine.
    if (dataCount > 105000) {
        pPsL10 = new cPointSearch_t<10, MAX_SEARCH_POINTS>(dataCount, points_begin);
        find = &SearchContext::useL10;
    }
    else if (dataCount > 20500) {
        pPsL9 = new cPointSearch_t<9, MAX_SEARCH_POINTS>(dataCount, points_begin);
        find = &SearchContext::useL9;
    }
    else if (dataCount > 1000) {
        pPsL8 = new cPointSearch_t<8, MAX_SEARCH_POINTS>(dataCount, points_begin);
        find = &SearchContext::useL8;
    }
    else if( dataCount > 30 ) {
        pPsL7 = new cPointSearch_t<7, MAX_SEARCH_POINTS>(dataCount, points_begin);
        find = &SearchContext::useL7;
    }
    else {
        pSimple = new cPointSearchSimple_t(dataCount, points_begin);
        find = &SearchContext::useSimple;
    }
#endif
}

SearchContext::~SearchContext() {
#ifdef MYPROFILE
    std::cout << "!!!! PROFILE: countMergeSort = " << pPs->countMergeSort << std::endl;
    std::cout << "!!!! PROFILE: countResursive = " << pPs->countResursive << std::endl;
#endif

    if ( pPs )
        delete pPs;
    if (pPsL10)
        delete pPsL10;
    if (pPsL9)
        delete pPsL9;
    if (pPsL8)
        delete pPsL8;
    if (pPsL7)
        delete pPsL7;
    if (pSimple)
        delete pSimple;
}

int32_t SearchContext::find2(const Rect& r, const int32_t count, Point* out_points) {
    if (count == 0) {
        return 0;
    }
    else if (count <= MAX_SEARCH_POINTS) {
        return pPs->find(r, count, out_points);
    }
    else {
        pPs->alloc( count );
        int32_t copyCount = pPs->find(r, count, out_points);
        pPs->deAlloc();
        
        return copyCount;
    }
}

int32_t SearchContext::useL10(const Rect& r, const int32_t count, Point* out_points) {
    if (count == 0) {
        return 0;
    }
    else if (count <= MAX_SEARCH_POINTS) {
        return pPsL10->find(r, count, out_points);
    }
    else {
        pPsL10->alloc(count);
        int32_t copyCount = pPsL10->find(r, count, out_points);
        pPsL10->deAlloc();

        return copyCount;
    }
    return 0;
}
int32_t SearchContext::useL9(const Rect& r, const int32_t count, Point* out_points) {
    if (count == 0) {
        return 0;
    }
    else if (count <= MAX_SEARCH_POINTS) {
        return pPsL9->find(r, count, out_points);
    }
    else {
        pPsL9->alloc(count);
        int32_t copyCount = pPsL9->find(r, count, out_points);
        pPsL9->deAlloc();

        return copyCount;
    }
}
int32_t SearchContext::useL8(const Rect& r, const int32_t count, Point* out_points) {
    if (count == 0) {
        return 0;
    }
    else if (count <= MAX_SEARCH_POINTS) {
        return pPsL8->find(r, count, out_points);
    }
    else {
        pPsL8->alloc(count);
        int32_t copyCount = pPsL8->find(r, count, out_points);
        pPsL8->deAlloc();

        return copyCount;
    }
}
int32_t SearchContext::useL7(const Rect& r, const int32_t count, Point* out_points) {
    if (count == 0) {
        return 0;
    }
    else if (count <= MAX_SEARCH_POINTS) {
        return pPsL7->find(r, count, out_points);
    }
    else {
        pPsL7->alloc(count);
        int32_t copyCount = pPsL7->find(r, count, out_points);
        pPsL7->deAlloc();

        return copyCount;
    }
}

int32_t SearchContext::useSimple(const Rect& r, const int32_t count, Point* out_points) {
    if (count == 0) {
        return 0;
    }
   return pSimple->find(r, count, out_points);
}