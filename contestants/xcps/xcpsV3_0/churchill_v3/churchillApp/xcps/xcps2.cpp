#pragma once

#include "stdafx.h"
#include "xcps2.h"

cPointSearchSimple_t::cPointSearchSimple_t(int32_t count, const Point* points_begin) {
    maxSrcCount = count;

    pMyPoint = new myPoint_t[maxSrcCount];
    pIndexRankBase = new indexRank_t[maxSrcCount];

    XC::verify(
        !!pMyPoint &&
        !!pIndexRankBase
        , "no enough memory");

    if (maxSrcCount > 0)
        add(points_begin);
}

cPointSearchSimple_t::~cPointSearchSimple_t() {
    // TODO: use smart pointer or shared_array to delete them automatically.
    if (pMyPoint) {
        delete[] pMyPoint;
        pMyPoint = 0;
    }
    if (pIndexRankBase) {
        delete[] pIndexRankBase;
        pIndexRankBase = 0;
    }
}

void cPointSearchSimple_t::bubbleDown(__int64* pHeapBase, int32_t heapSize, int32_t pos) {
    register __int64* pTempHeap = pHeapBase;
    register __int64 curIndex = pos;
    while (1) {
        register __int64 child1 = curIndex << 1;
        register __int64 child2 = (curIndex << 1) + 1;

        if (child1 > heapSize)
            break;

        register __int64* pParent = pTempHeap + curIndex;
        register __int64* pChild1 = pTempHeap + child1;

        if (child1 == heapSize) {
            register __int64 vParent = *pParent;
            register __int64 vChild1 = *pChild1;

            if (vChild1 > vParent) {    // swap
                *pParent = vChild1;
                *pChild1 = vParent;
            }
            break;
        }

        // bubble down
        register __int64 vParent = *pParent;
        register __int64 vChild1 = *pChild1;

        register __int64* pChild2 = pTempHeap + child2;
        register __int64 vChild2 = *pChild2;

        if (vParent > vChild1) {
            if (vParent < vChild2) {
                // Swap parent/child2
                *pParent = vChild2;
                *pChild2 = vParent;

                curIndex = child2;
            }
            else {
                break;
            }
        }
        else {
            if (vParent > vChild2) {
                // Swap parent/child1
                *pParent = vChild1;
                *pChild1 = vParent;

                curIndex = child1;
            }
            else {
                if (vChild1 > vChild2) {
                    // Swap parent/child1
                    *pParent = vChild1;
                    *pChild1 = vParent;

                    curIndex = child1;
                }
                else {
                    // Swap parent/child2
                    *pParent = vChild2;
                    *pChild2 = vParent;

                    curIndex = child2;
                }
            }
        }
    } // end of while
}

// pHeapBase -> (zero element is not used) and we cannot overflow.
// sorting from pHeapBase+1 to pHeapBase+heapSize;
void cPointSearchSimple_t::heapSort(__int64* pHeapBase, int32_t heapSize) {
    // construct the heap now.
    for (int32_t ii = heapSize / 2; ii > 0; ii--) {
        bubbleDown(pHeapBase, heapSize, ii);
    }

    __int64* pLast = pHeapBase + heapSize;
    __int64* pTop = pHeapBase + 1;
    for (int32_t ii = 1; ii < heapSize - 1; ii++) {
        __int64 vLast = *pLast;
        *pLast = *pTop;
        *pTop = vLast;
        bubbleDown(pHeapBase, heapSize - ii, 1);
        pLast--;
    }

    // for last one, we does not need bubble down.
    __int64 vLast = *pLast;
    *pLast = *pTop;
    *pTop = vLast;
}

void cPointSearchSimple_t::add(const Point* pt) {
    const Point* pTempPt = pt;
    for (int32_t ii = 0; ii < maxSrcCount; ii++) {
        myPoint_t* p = pMyPoint + ii;
        p->x = pTempPt->x;
        p->y = pTempPt->y;
        p->id = pTempPt->id;

        indexRank_t* pIndexRank = pIndexRankBase + ii;
        pIndexRank->index = ii;
        pIndexRank->rank = pTempPt->rank;

        pTempPt++;
    }

    if ( maxSrcCount > 1 ) {
        heapSort((__int64*)(pIndexRankBase - 1), maxSrcCount);
    }
}

int32_t cPointSearchSimple_t::find(const Rect& r, const int32_t count, Point* out_points) {
    if (maxSrcCount == 0)
        return 0;

    register float lx = r.lx;
    register float ly = r.ly;
    register float hx = r.hx;
    register float hy = r.hy;

    indexRank_t* pIndexRank = pIndexRankBase;

    int32_t ii = 0;
    for (int32_t kk = 0; kk < maxSrcCount; kk ++) {
        int32_t idx = pIndexRank->index;
        myPoint_t* p = pMyPoint + idx;
        
        register float x = p->x;
        register float y = p->y;
        if (x >= lx && x <= hx && y >= ly && y <= hy) {
            out_points[ii].rank = pIndexRank->rank;
            out_points[ii].x = x;
            out_points[ii].y = y;
            out_points[ii].id = p->id;
            ii++;
            if (ii >= count)
                break;
        }
        pIndexRank++;
    }
    return ii;
}

