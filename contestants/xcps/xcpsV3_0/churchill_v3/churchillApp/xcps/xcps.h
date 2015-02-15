#pragma once

#include "xcpsDLL.h"
#include "../../point_search.h"
#include "xcError.h"
#include <list>
#include <iostream>
#include <fstream>
#include <sstream>
#include <memory>
#include <limits>
#include "smmintrin.h"
#include "mmintrin.h"
#include "xcpsComm.h"

//#define USEDLINK        // TODO: mergeA may still need use old method???
#define USEIDX2
#define USELINEAR

#define IDX_TABLE_SIZE     1000000
#define IDX_TABLE_SIZEP2    (IDX_TABLE_SIZE+2)

#define XYSTEP1BITS  3
#define XYSTEP2BITS  5

#define XYSTEP1     (1<<XYSTEP1BITS)    // 4
#define XYSTEP2     (1<<XYSTEP2BITS)    // 16
#define XYSTEPS     2

#define XYSTEP21_DBITS  (XYSTEP2BITS-XYSTEP1BITS)
#define XYSTEP10_DBITS  (XYSTEP1BITS)

#define XYSTEP1M1   (XYSTEP1-1)         // 3
#define XYSTEP2M1   (XYSTEP2-1)         // 15

#define XYSTEP1MASK (~(XYSTEP1-1))
#define XYSTEP2MASK (~(XYSTEP2-1))

//#define MYPROFILE
//#define TRACING

struct ptIndex_t {
    int32_t index;  // index of the raw array;
    int32_t next;   // offset of the ptIndex_t*
};

struct cellNode_t {
    int32_t ptCount;    // how many points inside the cell.
    int32_t startIndex; // start index of the re-arrange raw data array. --> pIndexRankBase
};

struct aggCellNode_t {  // aggregated cells
    //int32_t factor;
    int32_t sizeNN;
    cellNode_t* pAggCellNode;   // size of the array is: sizeNN x sizeNN
    indexRank_t* pAggIndexRank;
};

struct linearAggCellNode_t {  // aggregated cells
    int32_t sizeNN;
    cellNode_t* pAggCellNode;   // size of the array is: sizeNN x sizeNN
    indexRank_t* pAggIndexRank;
};

struct sortedNode_t;

struct sortedNode_t {
    sortedNode_t* next;
    sortedNode_t* previous;
    union {
        indexRank_t indexRank;
        __int64     indexRank64;
    };
};

template <int NL, int MC> class cPointSearch_t {
#define LOG_NN_P1   (NL)    // NL=10, NN=512;    NL=9, NN=256; ...
#define NN          (1<<((LOG_NN_P1)-1))
#define NNP2        ((NN)+2)

    int32_t maxSrcCount;    // maximum available source data count;
    
    int32_t maxReqCount;    // maximum interesting points to be returned.
    int32_t maxReqCountM1;  // maximum interesting points to be returned minus 1;

    float xMin;             // min value of the input data in x-coordinate
    float xMax;             // max value of the input data in x-coordinate
    float yMin;             // min value of the input data in y-coordinate
    float yMax;             // max value of the input data in y-coordinate

    float xStep;            // cell steps in x direction
    float yStep;            // cell steps in y direction

    float* pXBase;          // All x coordinates
    float* pYBase;          // All y coordinates
    indexRank_t* pIndexRankBase;    // All rank and index to the points array
    int8_t* pIdBase;        // All Id 
    
    indexRank_t* pTempSortedBase;   // temporary point save, this is only used for interesting points greater than 256 (MC)
    indexRank_t* pSortedBase[2];    // double buffering.
    int32_t sortedSet;              // 0 - answer is in even set, 1 - answer is in odd set
    
    int32_t* pCellNext;             // point to pPtIndexBase, indexed using cell index.
    ptIndex_t* pPtIndexBase;        // This is for connecting the points during the load time.
        

    typedef bool (cPointSearch_t::*cond_t)(int32_t index);  
    cond_t cond;                                            // The variable is not hooked it yet. TODO: delete it and clean it up. 

    Rect rect;

    int32_t* log2Table;             // Build log2 table to avoid using log floating point computation.

    __int64 lastBiggest;
#ifdef USEDLINK
    sortedNode_t* pSnHead;
    sortedNode_t* pSnTail;
    sortedNode_t sortedNode[MC + 1];
#endif
    aggCellNode_t aggCells[LOG_NN_P1];
    cellNode_t cell[NN + 2][NN + 2];

#ifdef USELINEAR
    linearAggCellNode_t xAggCells[XYSTEPS];
    linearAggCellNode_t yAggCells[XYSTEPS];
#endif

    int16_t xIdxTable[IDX_TABLE_SIZEP2];
    int16_t yIdxTable[IDX_TABLE_SIZEP2];

    void bubbleDown(__int64* pHeapBase, int32_t heapSize, int32_t pos);
    void heapSort(__int64* pHeapBase, int32_t heapSize);

    void mergeSort(__int64* pIndexRank, int32_t ptCount);
    void mergeSort2(__int64* pIndexRank, int32_t ptCount);
    void mergeSort3(__int64* pIndexRank, int32_t ptCount);
    
    template <class T> void mergeSortCondTemp(__int64* pIndexRank, int32_t ptCount);
    template <class T> void mergeSortCond2Temp(__int64* pIndexRank, int32_t ptCount);
    template <class T> void mergeSortCond3Temp(__int64* pIndexRank, int32_t ptCount);

    /*  +---+---+---+       +---+---+---+       +---+
        | F | D | G |       | K | J | L |       | N |
        +---+---+---+       +---+---+---+       +---+   +---+
        | B | A | C |                           | M |   | P |
        +---+---+---+                           +---+   +---+
        | I | E | H |                           | O |
        +---+---+---+                           +---+
    */   
    void mergeA(int32_t xil, int32_t xih, int32_t yil, int32_t yih);
    void mergeARecursive(int32_t xl, int32_t xh, int32_t yl, int32_t yh);
    
    template<class T> void mergeBCM_temp(int32_t xi, int32_t yil, int32_t yih);
    template<class T> void mergeDEJ_temp(int32_t xil, int32_t xih, int32_t yi);
    template<class T> void mergeCorner_temp(int32_t xi, int32_t yi);
    template<class T> void mergeP_temp(int32_t xi, int32_t yi);

    void findMinMax(const Point* pt);
    void genIdxTable(const Point* pt);
    void computeCellSt(const Point* pt, int* pSum);
    void outputCellSt(const Point* pt, int* pSum);
    void bundleUp();
    void linearBundleUp();
    void xyFirstAggregation(int32_t direction, int32_t step);
    void xyNextAggregation(int32_t direction, int32_t step, int32_t t0, int32_t t1);
    void mergeTwoCells(__int64* pSrc0, int32_t c0, __int64* pSrc1, int32_t c1, __int64* pDst);
    void mergeCells(cellNode_t* pSrcNode, __int64* pSrcData, int32_t idx0, int32_t idx1, cellNode_t* pDstNode, int32_t idx2, __int64* pDstData, int32_t& pos);
    void aggCellMerge(cellNode_t* pSrcNode, indexRank_t* pIndexRank);
    void mergeOneCell(int32_t ii, int32_t jj);
    void mergeColCell(int32_t ii, int32_t yil, int32_t yih, int32_t step);
    void mergeRowCell(int32_t xil, int32_t xih, int32_t jj, int32_t step);

    template<class T> void mergeColCell_temp(int32_t ii, int32_t yil, int32_t yih, int32_t step);
    template<class T> void mergeRowCell_temp(int32_t xil, int32_t xih, int32_t jj, int32_t step);

    void searchPoints();

public:
    //--------------------------------------------------------------------------------
    cPointSearch_t(int32_t count, const Point* points_begin);
    ~cPointSearch_t();

    int32_t find(const Rect& r, const int32_t count, Point* out_points);
    void add(const Point* pt );

    void alloc(int32_t count);
    void deAlloc();

#ifdef TRACING
    bool enableTrace;
#endif

#ifdef MYPROFILE
    int countMerge[PROFILE_COUNTERS];
    int countResursive;
#endif

};

class cMergeCond_lx_t {
public:
    bool operator()(float* pXBase, float* pYBase, int32_t index, Rect& rect) {
        return (*(pXBase + index) >= rect.lx);
    }
};
class cMergeCond_hx_t {
public:
    bool operator()(float* pXBase, float* pYBase, int32_t index, Rect& rect) {
        return (*(pXBase + index) <= rect.hx);
    }
};
class cMergeCond_ly_t {
public:
    bool operator()(float* pXBase, float* pYBase, int32_t index, Rect& rect) {
        return (*(pYBase + index) >= rect.ly);
    }
};
class cMergeCond_hy_t {
public:
    bool operator()(float* pXBase, float* pYBase, int32_t index, Rect& rect) {
        return (*(pYBase + index) <= rect.hy);
    }
};
class cMergeCond_lx_ly_t {
public:
    bool operator()(float* pXBase, float* pYBase, int32_t index, Rect& rect) {
        return ((*(pXBase + index) >= rect.lx) && (*(pYBase + index) >= rect.ly));
    }
};
class cMergeCond_lx_hy_t {
public:
    bool operator()(float* pXBase, float* pYBase, int32_t index, Rect& rect) {
        return ((*(pXBase + index) >= rect.lx) && (*(pYBase + index) <= rect.hy));
    }
};
class cMergeCond_hx_ly_t {
public:
    bool operator()(float* pXBase, float* pYBase, int32_t index, Rect& rect) {
        return ((*(pXBase + index) <= rect.hx) && (*(pYBase + index) >= rect.ly));
    }
};
class cMergeCond_hx_hy_t {
public:
    bool operator()(float* pXBase, float* pYBase, int32_t index, Rect& rect) {
        return ((*(pXBase + index) <= rect.hx) && (*(pYBase + index) <= rect.hy));
    }
};
class cMergeCond_lx_hx_t {
public:
    bool operator()(float* pXBase, float* pYBase, int32_t index, Rect& rect) {
        return ((*(pXBase + index) >= rect.lx) && (*(pXBase + index) <= rect.hx));
    }
};
class cMergeCond_ly_hy_t {
public:
    bool operator()(float* pXBase, float* pYBase, int32_t index, Rect& rect) {
        return ((*(pYBase + index) >= rect.ly) && (*(pYBase + index) <= rect.hy));
    }
};
class cMergeCond_lx_ly_hy_t {
public:
    bool operator()(float* pXBase, float* pYBase, int32_t index, Rect& rect) {
        return ((*(pXBase + index) >= rect.lx) &&
            (*(pYBase + index) >= rect.ly) &&
            (*(pYBase + index) <= rect.hy));
    }
};
class cMergeCond_hx_ly_hy_t {
public:
    bool operator()(float* pXBase, float* pYBase, int32_t index, Rect& rect) {
        return ((*(pXBase + index) <= rect.hx) &&
            (*(pYBase + index) >= rect.ly) &&
            (*(pYBase + index) <= rect.hy));
    }
};
class cMergeCond_ly_lx_hx_t {
public:
    bool operator()(float* pXBase, float* pYBase, int32_t index, Rect& rect) {
        return ((*(pYBase + index) >= rect.ly) &&
            (*(pXBase + index) >= rect.lx) &&
            (*(pXBase + index) <= rect.hx));
    }
};
class cMergeCond_hy_lx_hx_t {
public:
    bool operator()(float* pXBase, float* pYBase, int32_t index, Rect& rect) {
        return ((*(pYBase + index) <= rect.hy) &&
            (*(pXBase + index) >= rect.lx) &&
            (*(pXBase + index) <= rect.hx));
    }
};
class cMergeCond_lx_hx_ly_hy_t {
public:
    bool operator()(float* pXBase, float* pYBase, int32_t index, Rect& rect) {
        return ((*(pYBase + index) >= rect.ly) &&
            (*(pYBase + index) <= rect.hy) &&
            (*(pXBase + index) >= rect.lx) &&
            (*(pXBase + index) <= rect.hx));
    }
};

template <int NL, int MC> cPointSearch_t<NL, MC>::cPointSearch_t(int32_t count, const Point* points_begin) : pTempSortedBase(0) {
#ifdef MYPROFILE
    for (int32_t ii = 0; ii < PROFILE_COUNTERS; ii++) {
        countMerge[ii] = 0;
    }
    countResursive = 0;
#endif
    maxSrcCount = count;

    // TODO: we could use one "new" and manage the memory ourselves.
    pPtIndexBase = new ptIndex_t[maxSrcCount];
    pCellNext = new int32_t[(NN + 2)*(NN + 2)];

    pXBase = new ALIGN32 float[maxSrcCount];
    pYBase = new ALIGN32 float[maxSrcCount];
    pIndexRankBase = new ALIGN32 indexRank_t[maxSrcCount];
    pIdBase = new ALIGN8 int8_t[maxSrcCount];

    for (int32_t ii = 0; ii < LOG_NN_P1; ii++) {
        aggCells[ii].pAggCellNode = 0;
        aggCells[ii].pAggIndexRank = 0;
    }

#ifdef USELINEAR
    for (int32_t ii = 0; ii < XYSTEPS; ii++) {
        xAggCells[ii].pAggCellNode = 0;
        xAggCells[ii].pAggIndexRank = 0;
        yAggCells[ii].pAggCellNode = 0;
        yAggCells[ii].pAggIndexRank = 0;
    }
#endif

    pSortedBase[0] = new indexRank_t[2*(MC + 3)];
    pSortedBase[1] = pSortedBase[0] + (MC+3);

    log2Table = new ALIGN4 int32_t[NN + 4];

    XC::verify(
        !!pPtIndexBase &&
        !!pCellNext &&
        !!pXBase &&
        !!pYBase &&
        !!pIndexRankBase &&
        !!pIdBase &&
        !!log2Table &&
        !!pSortedBase[0],
        "no enough memory");

    // build log2Table
    for (int32_t ii = 1; ii < NN + 4; ii++) {
        log2Table[ii] = int32_t(log2(ii));
    }

    if (maxSrcCount > 0)
        add(points_begin);
}

template <int NL, int MC> cPointSearch_t<NL, MC>::~cPointSearch_t() {
    // TODO: use smart pointer or shared_array to delete them automatically.
    if (pPtIndexBase) {
        delete[] pPtIndexBase;
        pPtIndexBase = 0;
    }
    if (pCellNext) {
        delete[] pCellNext;
        pCellNext = 0;
    }
    if (pXBase) {
        delete[] pXBase;
        pXBase = 0;
    }
    if (pYBase) {
        delete[] pYBase;
        pYBase = 0;
    }
    if (pIndexRankBase) {
        delete[] pIndexRankBase;
        pIndexRankBase = 0;
    }
    if (pIdBase) {
        delete[] pIdBase;
        pIdBase = 0;
    }
    if (pSortedBase[0]) {
        delete[] pSortedBase[0];
        pSortedBase[0] = 0;
    }
    if (pTempSortedBase) {
        delete[] pTempSortedBase;
        pTempSortedBase = 0;
    }
    if (log2Table) {
        delete[] log2Table;
        log2Table = 0;
    }

    // TODO: this is a hack here, ii starts from 1, as we did not allocate for them here.
    if (aggCells[0].pAggCellNode) {
        delete[] aggCells[0].pAggCellNode;
        aggCells[0].pAggCellNode = 0;
    }
    for (int32_t ii = 1; ii < LOG_NN_P1; ii++) {
        if (aggCells[ii].pAggCellNode) {
            delete[] aggCells[ii].pAggCellNode;
            aggCells[ii].pAggCellNode = 0;
        }
        if (aggCells[ii].pAggIndexRank) {
            delete[] aggCells[ii].pAggIndexRank;
            aggCells[ii].pAggIndexRank = 0;
        }
    }
#ifdef USELINEAR
    for (int32_t ii = 0; ii < XYSTEPS; ii++) {
        if (xAggCells[ii].pAggCellNode) {
            delete[] xAggCells[ii].pAggCellNode;
            xAggCells[ii].pAggCellNode = 0;
        }
        if (xAggCells[ii].pAggIndexRank) {
            delete[] xAggCells[ii].pAggIndexRank;
            xAggCells[ii].pAggIndexRank = 0;
        }
        if (yAggCells[ii].pAggCellNode) {
            delete[] yAggCells[ii].pAggCellNode;
            yAggCells[ii].pAggCellNode = 0;
        }
        if (yAggCells[ii].pAggIndexRank) {
            delete[] yAggCells[ii].pAggIndexRank;
            yAggCells[ii].pAggIndexRank = 0;
        }
    }
#endif
}

template <int NL, int MC> void cPointSearch_t<NL, MC>::alloc(int32_t count) {
    if (count > maxSrcCount)
        count = maxSrcCount;

    // store the pointer
    pTempSortedBase = pSortedBase[0];

    pSortedBase[0] = new indexRank_t[2 * (count + 3)];
    if (pSortedBase[0] == 0) {
        pSortedBase[0] = pTempSortedBase;
        pTempSortedBase = 0;
        throw XC::errorExcept_t("out of memory in search");
    }
    pSortedBase[1] = pSortedBase[0] + (count + 3);

}

template <int NL, int MC> void cPointSearch_t<NL, MC>::deAlloc() {
    delete[] pSortedBase[0];
    // restore the pointer
    pSortedBase[0] = pTempSortedBase;
    pSortedBase[1] = pTempSortedBase + (MC + 3);
    pTempSortedBase = 0;
}

template <int NL, int MC> void cPointSearch_t<NL, MC>::bubbleDown(__int64* pHeapBase, int32_t heapSize, int32_t pos) {
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
template <int NL, int MC> void cPointSearch_t<NL, MC>::heapSort(__int64* pHeapBase, int32_t heapSize) {
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

template <int NL, int MC> void cPointSearch_t<NL, MC>::findMinMax(const Point* pt) {
    // find max and min
    /*xMin = FLT_MAX;
    xMax = FLT_MIN;
    yMin = FLT_MAX;
    yMax = FLT_MIN;

    const Point* pTempPt = pt;
    for (int32_t ii = 0; ii < maxSrcCount; ii++) {
    if (pTempPt->x <= PT_LB_FLOAT || pTempPt->x >= PT_UB_FLOAT || pTempPt->y <= PT_LB_FLOAT || pTempPt->y >= PT_UB_FLOAT) {
    continue;
    }
    xMin = myMin(xMin, pTempPt->x);
    xMax = myMax(xMax, pTempPt->x);
    yMin = myMin(yMin, pTempPt->y);
    yMax = myMax(yMax, pTempPt->y);

    pTempPt++;
    }*/

    {
        xMin = FLT_MAX;
        xMax = FLT_MIN;
        yMin = FLT_MAX;
        yMax = FLT_MIN;

        __m128 mxMin;
        __m128 mxMax;
        __m128 myMin;
        __m128 myMax;
        __m128 mx;
        __m128 my;
        mxMin = _mm_set1_ps(FLT_MAX);
        mxMax = _mm_set1_ps(FLT_MIN);
        myMin = _mm_set1_ps(FLT_MAX);
        myMax = _mm_set1_ps(FLT_MIN);
        const Point* p0 = pt;
        int32_t leftover = 0;
        int32_t ii = 0;
        float x0, x1, x2, x3;
        float y0, y1, y2, y3;
        while (ii < maxSrcCount) {
            while (p0->x <= PT_LB_FLOAT || p0->x >= PT_UB_FLOAT || p0->y <= PT_LB_FLOAT || p0->y >= PT_UB_FLOAT) {
                ii++;
                p0++;
                if (ii >= maxSrcCount) {
                    leftover = 0;
                    break;
                }
            }
            x0 = p0->x; y0 = p0->y; p0++; ii++;

            while (p0->x <= PT_LB_FLOAT || p0->x >= PT_UB_FLOAT || p0->y <= PT_LB_FLOAT || p0->y >= PT_UB_FLOAT) {
                ii++;
                p0++;
                if (ii >= maxSrcCount) {
                    leftover = 1;
                    break;
                }
            }
            x1 = p0->x; y1 = p0->y; p0++; ii++;

            while (p0->x <= PT_LB_FLOAT || p0->x >= PT_UB_FLOAT || p0->y <= PT_LB_FLOAT || p0->y >= PT_UB_FLOAT) {
                ii++;
                p0++;
                if (ii >= maxSrcCount) {
                    leftover = 2;
                    break;
                }
            }
            x2 = p0->x; y2 = p0->y; p0++; ii++;

            while (p0->x <= PT_LB_FLOAT || p0->x >= PT_UB_FLOAT || p0->y <= PT_LB_FLOAT || p0->y >= PT_UB_FLOAT) {
                ii++;
                p0++;
                if (ii >= maxSrcCount) {
                    leftover = 3;
                    break;
                }
            }
            x3 = p0->x; y3 = p0->y; p0++; ii++;

            mx = _mm_set_ps(x0, x1, x2, x3);
            my = _mm_set_ps(y0, y1, y2, y3);

            mxMax = _mm_max_ps(mx, mxMax);
            mxMin = _mm_min_ps(mx, mxMin);

            myMax = _mm_max_ps(my, myMax);
            myMin = _mm_min_ps(my, myMin);
        }
        switch (leftover) {
        case 3:
            xMin = min(xMin, x2); yMin = min(yMin, y2);
            xMax = max(xMax, x2); yMax = max(yMax, y2);
        case 2:
            xMin = min(xMin, x1); yMin = min(yMin, y1);
            xMax = max(xMax, x1); yMax = max(yMax, y1);
        case 1:
            xMin = min(xMin, x0); yMin = min(yMin, y0);
            xMax = max(xMax, x0); yMax = max(yMax, y0);
        default:
            break;
        }

        union u
        {
            __m128 m;
            float f[4];
        } a0, b0, a1, b1;

        a0.m = mxMin;
        a1.m = mxMax;
        b0.m = myMin;
        b1.m = myMax;

        for (int jj = 0; jj < 4; jj++) {
            xMin = min(xMin, a0.f[jj]);
            xMax = max(xMax, a1.f[jj]);
            yMin = min(yMin, b0.f[jj]);
            yMax = max(yMax, b1.f[jj]);
        }
    }
}

template <int NL, int MC> void cPointSearch_t<NL, MC>::genIdxTable(const Point* pt) {
#ifdef USEIDX2
    xStep = (xMax - xMin) / IDX_TABLE_SIZE;
    yStep = (yMax - yMin) / IDX_TABLE_SIZE;

    std::unique_ptr<int32_t[]> pXIdxCount(new int32_t[IDX_TABLE_SIZEP2]);
    std::unique_ptr<int32_t[]> pYIdxCount(new int32_t[IDX_TABLE_SIZEP2]);
    XC::verify(pXIdxCount && pYIdxCount, "no enough memory for pX/YIdxCount");

    // TODO: improve the loading time by using SIMD 
    {
        __m128i m1;
        m1 = _mm_set1_epi32(0);

        int32_t* px = pXIdxCount.get();
        int32_t* py = pYIdxCount.get();
        int32_t loopCount = IDX_TABLE_SIZEP2 >> 2;
        for (int32_t ii = 0; ii < loopCount; ii++) {
            _mm_storeu_si128((__m128i*)px, m1); // set to zero four once
            _mm_storeu_si128((__m128i*)py, m1); // set to zero four once
            px += 4;
            py += 4;
        }
        // clear the rest
        for (int32_t ii = 0; ii < (IDX_TABLE_SIZEP2 & 3); ii++) {
            *px = 0;
            *py = 0;
            px++;
            py++;
        }
    }

    /*const Point* pTempPt3 = pt;
    for (int32_t ii = 0; ii < maxSrcCount; ii++) {
    int32_t xCellIndex;
    int32_t yCellIndex;
    float xIdx = (pTempPt3->x - xMin) / xStep;
    float yIdx = (pTempPt3->y - yMin) / yStep;
    if (xIdx > IDX_TABLE_SIZE)
    xIdx = IDX_TABLE_SIZE;
    if (yIdx > IDX_TABLE_SIZE)
    yIdx = IDX_TABLE_SIZE;

    xCellIndex = int32_t(xIdx);
    yCellIndex = int32_t(yIdx);

    xCellIndex = myMax(xCellIndex, 0);
    yCellIndex = myMax(yCellIndex, 0);

    pXIdxCount[xCellIndex] ++;
    pYIdxCount[yCellIndex] ++;
    pTempPt3++;
    }*/
    {
        __m128 mxMin = _mm_set1_ps(xMin);
        __m128 myMin = _mm_set1_ps(yMin);
        __m128 mxStep = _mm_set1_ps(xStep);
        __m128 myStep = _mm_set1_ps(yStep);
        __m128 mTableSize = _mm_set1_ps(IDX_TABLE_SIZE);
        __m128 mZero = _mm_set1_ps(0);

        const Point* p0 = pt;
        int32_t loopCount = maxSrcCount >> 2;
        for (int32_t ii = 0; ii < loopCount; ii++) {
            float x0, x1, x2, x3;
            float y0, y1, y2, y3;
            x0 = p0->x; y0 = p0->y; p0++;
            x1 = p0->x; y1 = p0->y; p0++;
            x2 = p0->x; y2 = p0->y; p0++;
            x3 = p0->x; y3 = p0->y; p0++;

            __m128 mxIdx;
            __m128 myIdx;

            mxIdx = _mm_set_ps(x0, x1, x2, x3);
            myIdx = _mm_set_ps(y0, y1, y2, y3);

            mxIdx = _mm_sub_ps(mxIdx, mxMin);
            myIdx = _mm_sub_ps(myIdx, myMin);

            mxIdx = _mm_div_ps(mxIdx, mxStep);
            myIdx = _mm_div_ps(myIdx, myStep);

            mxIdx = _mm_min_ps(mxIdx, mTableSize);
            myIdx = _mm_min_ps(myIdx, mTableSize);

            mxIdx = _mm_max_ps(mxIdx, mZero);
            myIdx = _mm_max_ps(myIdx, mZero);

            __m128i mxIdxInt = _mm_cvtps_epi32(mxIdx);
            __m128i myIdxInt = _mm_cvtps_epi32(myIdx);

            union {
                __m128i m;
                int32_t e[4];
            } xCellIndex, yCellIndex;
            xCellIndex.m = mxIdxInt;
            yCellIndex.m = myIdxInt;

            for (int jj = 0; jj < 4; jj++) {
                pXIdxCount[xCellIndex.e[jj]] ++;
                pYIdxCount[yCellIndex.e[jj]] ++;
            }
        }
        for (int32_t ii = 0; ii < (maxSrcCount & 3); ii++) {
            int32_t xCellIndex;
            int32_t yCellIndex;
            float xIdx = (p0->x - xMin) / xStep;
            float yIdx = (p0->y - yMin) / yStep;
            if (xIdx > IDX_TABLE_SIZE)
                xIdx = IDX_TABLE_SIZE;
            if (yIdx > IDX_TABLE_SIZE)
                yIdx = IDX_TABLE_SIZE;

            xCellIndex = int32_t(xIdx);
            yCellIndex = int32_t(yIdx);

            xCellIndex = myMax(xCellIndex, 0);
            yCellIndex = myMax(yCellIndex, 0);

            pXIdxCount[xCellIndex] ++;
            pYIdxCount[yCellIndex] ++;
            p0++;
        }
    }

    int32_t k0 = 0;
    int64_t xCount = 0;
    for (int32_t ii = 0; ii < NNP2; ii++) {
        __int64 xThresh = ((__int64)maxSrcCount * (ii + 1) / NNP2) + 1;

        while (xCount <= xThresh) {
            xIdxTable[k0] = ii;
            xCount += pXIdxCount[k0];
            k0++;
            if (k0 >= IDX_TABLE_SIZEP2) {
                k0 = IDX_TABLE_SIZE + 1;
                break;
            }
        }
    }

    k0 = 0;
    int64_t yCount = 0;
    for (int32_t ii = 0; ii < NNP2; ii++) {
        __int64 yThresh = ((__int64)maxSrcCount * (ii + 1) / NNP2) + 1;
        while (yCount <= yThresh) {
            yIdxTable[k0] = ii;
            yCount += pYIdxCount[k0];
            k0++;
            if (k0 >= IDX_TABLE_SIZEP2) {
                k0 = IDX_TABLE_SIZE + 1;
                break;
            }
        }
    }
#else
    xStep = (xMax - xMin) / NN;
    yStep = (yMax - yMin) / NN;
#endif
}

template <int NL, int MC> void cPointSearch_t<NL, MC>::computeCellSt(const Point* pt, int* pSum) {
    const Point* pTempPt2 = pt;
    for (int ii = 0; ii < NNP2*NNP2; ii++) {
        pSum[ii] = 0;
    }
    for (int32_t ii = 0; ii < maxSrcCount; ii++) {
        int32_t xCellIndex;
        int32_t yCellIndex;
        float xIdx = (pTempPt2->x - xMin) / xStep;
        float yIdx = (pTempPt2->y - yMin) / yStep;
        if (xIdx > IDX_TABLE_SIZE)
            xIdx = IDX_TABLE_SIZE;
        if (yIdx > IDX_TABLE_SIZE)
            yIdx = IDX_TABLE_SIZE;
        xCellIndex = int32_t(xIdx);
        yCellIndex = int32_t(yIdx);

        xCellIndex = myMax(xCellIndex, 0);
        yCellIndex = myMax(yCellIndex, 0);

        xCellIndex = xIdxTable[xCellIndex]; // idx translation
        yCellIndex = yIdxTable[yCellIndex]; // idx translation

        pSum[xCellIndex + yCellIndex * NNP2] ++;

        pTempPt2++;
    }
}

template <int NL, int MC> void cPointSearch_t<NL, MC>::outputCellSt(const Point* pt, int* pSum) {
    const Point* pTempPt2 = pt;
    // write the point and rank into a text file;
    std::ofstream wfx("datax.csv");
    wfx << "maxSrcCount=," << maxSrcCount << ", NNP2=," << NNP2 << std::endl;
    int32_t maxValue = 0;
    for (int ii = 0; ii < NNP2; ii++) { // row
        for (int jj = 0; jj < NNP2; jj++) { // col
            wfx << pSum[ii*NNP2+jj] << ", ";
            if (maxValue < pSum[ii*NNP2 + jj])
                maxValue = pSum[ii*NNP2 + jj];
        }
        wfx << std::endl;
    }
    std::cout << "max value: " << maxValue << std::endl;
    wfx.close();
}

template <int NL, int MC> void cPointSearch_t<NL, MC>::add(const Point* pt) {
#if 0
    {
        const Point* pPtBase = pt;
        // write the point and rank into a text file;
        std::ofstream wfx("datax3.bin", std::ios::binary);
        std::ofstream wfy("datay3.bin", std::ios::binary);
        std::ofstream wfr("datar3.bin", std::ios::binary);
#define WN  1000
        double* mx = new double[WN];
        double* my = new double[WN];
        double* mr = new double[WN];
        for (int ii = 0; ii < 10000; ii++) {
            for (int jj = 0; jj < WN; jj++) {
                mx[jj] = pPtBase->x;
                my[jj] = pPtBase->y;
                mr[jj] = pPtBase->rank;
                pPtBase++;
                //std::cout << "x=" << pPtBase->x << ", y=" << pPtBase->y << ", rank=" << pPtBase->rank << std::endl;
            }
            wfx.write((char*)(mx), sizeof(double)*WN);
            wfy.write((char*)(my), sizeof(double)*WN);
            wfr.write((char*)(mr), sizeof(double)*WN);
        }
        delete[] mx;
        delete[] my;
        delete[] mr;
    }
#endif

    const Point* pPtBase = pt;

    // clear pCell array;
    for (int32_t ii = 0; ii < NN + 2; ii++) {
        for (int32_t jj = 0; jj < NN + 2; jj++) {
            cell[ii][jj].ptCount = 0;
        }
    }

    {   // clear pCellNext
        int32_t* p0 = pCellNext;
        int32_t count0 = (NNP2 * NNP2) >> 2;
        static_assert((NNP2 & 0x1) == 0, "NNP2 must be an even number to make the following code work");
        __m128i m1;
        m1 = _mm_set1_epi32(0);
        for (int32_t ii = 0; ii < count0; ii++) {
            _mm_storeu_si128((__m128i*)p0, m1); // set to zero four once
            p0 += 4;
        }
    }

    findMinMax(pt);

    genIdxTable(pt);


#if 0
    if (1){
        int sum[NNP2][NNP2];
        computeCellSt(pt, &sum[0][0]);
        outputCellSt(pt, &sum[0][0]);
    }
    exit(0);
#endif

    const Point* pTempPt2 = pt;
    for (int32_t ii = 0; ii < maxSrcCount; ii++) {
        int32_t xCellIndex;
        int32_t yCellIndex;
        float xIdx = (pTempPt2->x - xMin) / xStep;
        float yIdx = (pTempPt2->y - yMin) / yStep;
#ifdef USEIDX2
        if (xIdx > IDX_TABLE_SIZE)
            xIdx = IDX_TABLE_SIZE;
        if (yIdx > IDX_TABLE_SIZE)
            yIdx = IDX_TABLE_SIZE;
#else
        if (xIdx > NN + 1)
            xIdx = NN + 1;
        if (yIdx > NN + 1)
            yIdx = NN + 1;
#endif

        xCellIndex = int32_t(xIdx);
        yCellIndex = int32_t(yIdx);

        xCellIndex = myMax(xCellIndex, 0);
        yCellIndex = myMax(yCellIndex, 0);

        pPtIndexBase[ii].index = ii;
        pPtIndexBase[ii].next = 0;

#ifdef USEIDX2
        xCellIndex = xIdxTable[xCellIndex]; // idx translation
        yCellIndex = yIdxTable[yCellIndex]; // idx translation
#endif
        //--------------------------------------------------------
        // insert the point into the list
        cellNode_t* pCellNode = &cell[yCellIndex][xCellIndex];
        pCellNode->ptCount++;

        ptIndex_t* pTemp = pPtIndexBase + ii;
        int32_t tempIndex = yCellIndex*(NN + 2) + xCellIndex;
        pTemp->next = pCellNext[tempIndex];
        pCellNext[tempIndex] = ii;
        //--------------------------------------------------------

        pTempPt2++;
    }

    // re-arrange the data
    int32_t kk = 0;
    int32_t startIndex = 0;
    for (int32_t ii = 0; ii < NN + 2; ii++) {   // per row
        for (int32_t jj = 0; jj < NN + 2; jj++) {   // per col
            int32_t tempIndex = ii*(NN + 2) + jj;
            ptIndex_t* pTempCell = pPtIndexBase + pCellNext[tempIndex];
            cell[ii][jj].startIndex = startIndex;
            int32_t ptCount = cell[ii][jj].ptCount;
            startIndex += ptCount;
            for (int32_t mm = 0; mm < ptCount; mm++) {
                const Point* pTempPt3 = pPtBase + pTempCell->index;

                float* pX = pXBase + kk;
                *pX = pTempPt3->x;

                float* pY = pYBase + kk;
                *pY = pTempPt3->y;

                int8_t* pId = pIdBase + kk;
                *pId = pTempPt3->id;

                indexRank_t* pIndexRank = pIndexRankBase + kk;
                pIndexRank->index = kk;
                pIndexRank->rank = pTempPt3->rank;

                pTempCell = pPtIndexBase + pTempCell->next;
                kk++;
            }
        }
    }

    // sort the rank for each cell;
    for (int32_t ii = 0; ii < NN + 2; ii++) {   // per row
        for (int32_t jj = 0; jj < NN + 2; jj++) {   // per col
            int32_t si = cell[ii][jj].startIndex;
            int32_t ptCount = cell[ii][jj].ptCount;

            indexRank_t* pIndexRankCellBase = pIndexRankBase + si;
            if (ptCount > 1) {
                heapSort((__int64*)(pIndexRankCellBase - 1), ptCount);
            }
        }
    }

    // release unused memory now.
    delete[] pPtIndexBase;
    pPtIndexBase = 0;

    delete[] pCellNext;
    pCellNext = 0;

    bundleUp();
#ifdef USELINEAR
    linearBundleUp();
#endif
}

template <int NL, int MC> void cPointSearch_t<NL, MC>::linearBundleUp() {
    static_assert(XYSTEPS == 2, "initialize the following properly");
    int32_t xySteps[XYSTEPS] = { XYSTEP1, XYSTEP2 };

    // allocate the proper memory space now.
    for (int32_t ii = 0; ii < XYSTEPS; ii++) {
        int sizeNN = (NN+2)/xySteps[ii];
        XC::verify(sizeNN > 0, "no need for supporting linearBundleUp");

        xAggCells[ii].sizeNN = sizeNN;
        xAggCells[ii].pAggCellNode = new ALIGN16 cellNode_t[(NN+2)*sizeNN];
        xAggCells[ii].pAggIndexRank = new indexRank_t[maxSrcCount + 1];

        yAggCells[ii].sizeNN = sizeNN;
        yAggCells[ii].pAggCellNode = new ALIGN16 cellNode_t[(NN + 2)*sizeNN];
        yAggCells[ii].pAggIndexRank = new indexRank_t[maxSrcCount + 1];

        XC::verify(
            !!xAggCells[ii].pAggCellNode &&
            !!xAggCells[ii].pAggIndexRank &&
            !!yAggCells[ii].pAggCellNode && 
            !!yAggCells[ii].pAggIndexRank, 
            "no enough memory");
    }

    // work out the first X/ Aggregation
    xyFirstAggregation(0, XYSTEP1);
    xyFirstAggregation(1, XYSTEP1);

    // work out the next level X/Y aggregation
    static_assert((XYSTEP2 / XYSTEP1) * XYSTEP1 == XYSTEP2, "invalid XYSTEP1 and XYSTEP2 relationships");
    xyNextAggregation(0, XYSTEP2 / XYSTEP1, 0, 1);
    xyNextAggregation(1, XYSTEP2 / XYSTEP1, 0, 1);
}

// direction: 0 - x; 1 - y
// t0 - source level; t1 - target level
template <int NL, int MC> void cPointSearch_t<NL, MC>::xyNextAggregation(int32_t direction, int32_t step, int32_t t0, int32_t t1) {
    XC::verify(direction == 0 || direction == 1, "invalid direction");

    std::unique_ptr<int32_t[]>  pCellPtCount(new int32_t[step]);         // true, data is in the cell to merge; false, no data is in the cell.
    std::unique_ptr<int32_t[]>  pCellIndex(new int32_t[step]);      // index of the each cell to merge from
    int32_t loopCount = direction ? yAggCells[t1].sizeNN : xAggCells[t1].sizeNN;
    cellNode_t* pDst = direction ? yAggCells[t1].pAggCellNode : xAggCells[t1].pAggCellNode;
    __int64* pTempIndeRank = direction ? (__int64*)yAggCells[t1].pAggIndexRank : (__int64*)xAggCells[t1].pAggIndexRank;
    __int64* pSrcIndeRank = direction ? (__int64*)yAggCells[t0].pAggIndexRank : (__int64*)xAggCells[t0].pAggIndexRank;

    cellNode_t* pSrc = direction ? yAggCells[t0].pAggCellNode : xAggCells[t0].pAggCellNode;
    
    int32_t xstride = direction ? (NN + 2) : 1;
    int32_t ystride = direction ? 1 : step;

    int32_t loopX = direction ? (loopCount) : (NN + 2);
    int32_t loopY = direction ? (NN + 2) : (loopCount);
    for (int32_t ii = 0; ii < loopX; ii++) {
        for (int32_t jj = 0; jj < loopY; jj++) {
            int mergeCount = 0;
            cellNode_t* pSrcTemp = pSrc;
            for (int32_t kk = 0; kk < step; kk++) {
                pCellPtCount[kk] = pSrcTemp->ptCount;
                pCellIndex[kk] = pSrcTemp->startIndex;
                mergeCount += pSrcTemp->ptCount;
                pSrcTemp += xstride;    // x: xstride = 1; y: yAggCells[t0].sizeNN
            }
            pSrc += ystride;   // x: step; y: 1;

            for (int32_t k0 = 0; k0 < mergeCount; k0++) {
                int32_t minIndex = -1;
                __int64 minRank = 0;
                for (int32_t k1 = 0; k1 < step; k1++) {
                    if (pCellPtCount[k1] > 0) {
                        __int64 myIndexRank = *((__int64*)(pSrcIndeRank + pCellIndex[k1]));
                        if (minIndex == -1)  {
                            minIndex = k1;
                            minRank = myIndexRank;
                        }
                        else {
                            if (myIndexRank < minRank) {
                                minRank = myIndexRank;
                                minIndex = k1;
                            }
                        }
                    }
                }
                *pTempIndeRank = minRank;
                pTempIndeRank++;
                pCellIndex[minIndex] ++;
                pCellPtCount[minIndex] --;
            }
            pDst->ptCount = mergeCount;
            pDst++;
        }
        if (direction) {
            pSrc += (step - 1)*(NN + 2);
        }
    }
    // update the startIndex
    pDst = direction ? yAggCells[t1].pAggCellNode : xAggCells[t1].pAggCellNode;
    pDst->startIndex = 0;
    for (int32_t ii = 1; ii < (NN + 2)*loopCount; ii++) {
        int32_t nextStartIndex = pDst->startIndex + pDst->ptCount;
        pDst++;
        pDst->startIndex = nextStartIndex;
    }
}

template <int NL, int MC> void cPointSearch_t<NL, MC>::xyFirstAggregation(int32_t direction, int32_t step) {
    XC::verify(direction == 0 || direction == 1, "invalid direction");

    std::unique_ptr<int32_t[]>  pCellPtCount(new int32_t[step]);         // true, data is in the cell to merge; false, no data is in the cell.
    std::unique_ptr<int32_t[]>  pCellIndex(new int32_t[step]);      // index of the each cell to merge from
    int32_t loopCount = direction ? yAggCells[0].sizeNN : xAggCells[0].sizeNN;
    cellNode_t* pDst = direction ? yAggCells[0].pAggCellNode : xAggCells[0].pAggCellNode;
    __int64* pTempIndeRank = direction ? (__int64*)yAggCells[0].pAggIndexRank : (__int64*)xAggCells[0].pAggIndexRank;

    int32_t loopX = direction ? (loopCount) : (NN + 2);
    int32_t loopY = direction ? (NN + 2) : (loopCount);
    for (int32_t ii = 0; ii < loopX; ii++) {
        int cellIdx1 = direction ? (ii*step) : ii;
        for (int32_t jj = 0; jj < loopY; jj++) {
            int cellIdx2 = direction ? jj : (jj * step);
            int mergeCount = 0;
            for (int32_t kk = 0; kk < step; kk++) {
                int32_t n0 = cellIdx1 + kk*direction;
                int32_t n1 = cellIdx2 + kk*(!direction);
                cellNode_t& rCell = cell[n0][n1];
                pCellPtCount[kk] = rCell.ptCount;
                pCellIndex[kk] = rCell.startIndex;
                mergeCount += rCell.ptCount;
            }
            for (int32_t k0 = 0; k0 < mergeCount; k0++) {
                int32_t minIndex = -1;
                __int64 minRank = 0;
                for (int32_t k1 = 0; k1 < step; k1++) {
                    if (pCellPtCount[k1] > 0) {
                        __int64 myIndexRank = *((__int64*)(pIndexRankBase + pCellIndex[k1]));
                        if (minIndex == -1)  {
                            minIndex = k1;
                            minRank = myIndexRank;
                        }
                        else {
                            if (myIndexRank < minRank) {
                                minRank = myIndexRank;
                                minIndex = k1;
                            }
                        }
                    }
                }
                *pTempIndeRank = minRank;
                pTempIndeRank++;
                pCellIndex[minIndex] ++;
                pCellPtCount[minIndex] --;
            }
            pDst->ptCount = mergeCount;
            pDst++;
        }
    }
    // update the startIndex
    pDst = direction ? yAggCells[0].pAggCellNode : xAggCells[0].pAggCellNode;
    pDst->startIndex = 0;
    for (int32_t ii = 1; ii < (NN + 2)*loopCount; ii++) {
        int32_t nextStartIndex = pDst->startIndex + pDst->ptCount;
        pDst++;
        pDst->startIndex = nextStartIndex;
    }
}

template <int NL, int MC> void cPointSearch_t<NL, MC>::bundleUp() {
    // allocate the proper memory space now.
    for (int32_t ii = 0; ii < LOG_NN_P1; ii++) {
        int sizeNN = (NN >> ii);
        XC::verify(sizeNN > 0, "LOG_NN_P1 setting wrong");
        //aggCells[ii].factor = ii;
        aggCells[ii].sizeNN = sizeNN;
        aggCells[ii].pAggCellNode = new ALIGN16 cellNode_t[sizeNN*sizeNN];

        if (ii > 0) {
            int32_t requiredMemory = sizeNN*sizeNN*(MC + 3);
            if (requiredMemory > (maxSrcCount + 1)) {   // one extra because when we merge the cells, we are using the SIMD to copy
                requiredMemory = maxSrcCount + 1;       // as a result, it may spill over one element (each element is 64-bit)
            }
            aggCells[ii].pAggIndexRank = new indexRank_t[requiredMemory];
        }
        else {
            aggCells[0].pAggIndexRank = pIndexRankBase;
        }
        XC::verify(!!aggCells[ii].pAggCellNode && !!aggCells[ii].pAggIndexRank, "no enough memory");
    }

    // assign the first cell arrays with existing
    cellNode_t* pCellNode = aggCells[0].pAggCellNode;
    for (int32_t ii = 0; ii < NN; ii++) {
        for (int32_t jj = 0; jj < NN; jj++) {
            pCellNode->ptCount = cell[ii + 1][jj + 1].ptCount;
            pCellNode->startIndex = cell[ii + 1][jj + 1].startIndex;
            pCellNode++;
        }
    }

    // bundle up
    //#define MEASURE_WASTE
#ifdef MEASURE_WASTE
    // measure wasted memory
    int32_t waste = 0;
#endif
    for (int32_t ii = 1; ii < LOG_NN_P1; ii++) { // TODO: the first needs to be treated in a special way.
        int32_t jj = ii - 1;
        int32_t loops = (NN >> ii);
        int32_t jjSizeNN = aggCells[jj].sizeNN;

        int32_t idx0 = 0;
        int32_t idx1 = idx0 + jjSizeNN;
        int32_t pos = 0;
        int32_t idx2 = 0;
        for (int32_t k0 = 0; k0 < loops; k0++) {        // row
            for (int32_t k1 = 0; k1 < loops; k1++) {    // col
                mergeCells(aggCells[jj].pAggCellNode, (__int64*)aggCells[jj].pAggIndexRank, idx0, idx1,
                    aggCells[ii].pAggCellNode, idx2, (__int64*)aggCells[ii].pAggIndexRank, pos);
#ifdef MEASURE_WASTE
                waste += MC - (aggCells[ii].pAggCellNode->ptCount);
#endif
                idx0 += 2;
                idx1 += 2;
                idx2++;
            }
            idx0 += jjSizeNN;
            idx1 += jjSizeNN;
        }
    }

#ifdef MEASURE_WASTE
    std::cout << "---> waste = " << waste << std::endl;
#endif
}

template <int NL, int MC> void cPointSearch_t<NL, MC>::mergeCells(cellNode_t* pSrcNode, __int64* pSrcData, int32_t idx0, int32_t idx1, cellNode_t* pDstNode, int32_t idx2, __int64* pDstData, int32_t& pos) {
    // merge 01
    __int64* p0 = pSrcData + ((pSrcNode + idx0)->startIndex);
    __int64* p1 = pSrcData + ((pSrcNode + idx0 + 1)->startIndex);
    int32_t ptCount0 = (pSrcNode + idx0)->ptCount;
    int32_t ptCount1 = (pSrcNode + idx0 + 1)->ptCount;
    int32_t p01Count = ptCount0 + ptCount1;
    if (p01Count > MC)
        p01Count = MC;

    __int64* p01 = p0;
    if (ptCount0 > 0) {
        if (ptCount1 > 0) {
            p01 = (__int64*)pSortedBase[0];
            mergeTwoCells(p0, ptCount0, p1, ptCount1, p01);
        }
        else {
            p01 = p0;
        }
    }
    else {
        if (ptCount1 > 0) {
            p01 = p1;
        }
    }

    // merge 23
    __int64* p2 = pSrcData + ((pSrcNode + idx1)->startIndex);
    __int64* p3 = pSrcData + ((pSrcNode + idx1 + 1)->startIndex);
    int32_t ptCount2 = (pSrcNode + idx1)->ptCount;
    int32_t ptCount3 = (pSrcNode + idx1 + 1)->ptCount;
    int32_t p23Count = ptCount2 + ptCount3;
    if (p23Count > MC)
        p23Count = MC;

    __int64* p23 = p2;
    if (ptCount2 > 0) {
        if (ptCount3 > 0) {
            p23 = (__int64*)pSortedBase[1];
            mergeTwoCells(p2, ptCount2, p3, ptCount3, p23);
        }
        else {
            p23 = p2;
        }
    }
    else {
        if (ptCount3 > 0) {
            p23 = p3;
        }
    }

    // merge 0123
    int ptCount = p01Count + p23Count;
    if (ptCount > MC)
        ptCount = MC;

    (pDstNode + idx2)->ptCount = ptCount;       // update the dest collected information
    (pDstNode + idx2)->startIndex = pos;
    __int64* pDst = pDstData + pos;
    pos += ptCount;

    if (p01Count > 0) {
        if (p23Count > 0) {
            mergeTwoCells(p01, p01Count, p23, p23Count, pDst);
        }
        else {
            // copy to pDst
            int32_t copyCount = (ptCount + 1) >> 1;
            for (int ii = 0; ii < copyCount; ii++) {
                __m128i m1 = _mm_loadu_si128((__m128i*)p01);
                _mm_storeu_si128((__m128i*)pDst, m1);
                p01 += 2;
                pDst += 2;
            }
        }
    }
    else {
        if (p23Count > 0) {
            // copy to pDst
            int32_t copyCount = (ptCount + 1) >> 1;
            for (int ii = 0; ii < copyCount; ii++) {
                __m128i m1 = _mm_loadu_si128((__m128i*)p23);
                _mm_storeu_si128((__m128i*)pDst, m1);
                p23 += 2;
                pDst += 2;
            }
        }
    }
}

template <int NL, int MC> void cPointSearch_t<NL, MC>::mergeTwoCells(__int64* pSrc0, int32_t c0, __int64* pSrc1, int32_t c1, __int64* pDst) {
    register __int64* p0 = pSrc0;
    register __int64* p1 = pSrc1;
    register __int64* p2 = pDst;    // merge p0 and p1 into p2

    register __int64 e0 = *p0;
    register __int64 e1 = *p1;

    for (int32_t ii = 0; ii < MC; ii++) {
        if (e0 < e1) {
            *p2 = e0;
            p0++;
            p2++;
            e0 = *p0;
            c0--;
            if (c0 == 0) { // copy the rest of p1
                int32_t copyCount = MC - ii - 1;
                if (copyCount > c1)
                    copyCount = c1;
                for (int jj = 0; jj < copyCount; jj++) {
                    *p2 = e1;
                    p2++;
                    p1++;
                    e1 = *p1;
                }
                return;
            }
        }
        else {
            *p2 = e1;
            p1++;
            p2++;
            e1 = *p1;
            c1--;
            if (c1 == 0) { // copy the rest of p0
                int32_t copyCount = MC - ii - 1;
                if (copyCount > c0)
                    copyCount = c0;
                for (int jj = 0; jj < copyCount; jj++) {
                    *p2 = e0;
                    p2++;
                    p0++;
                    e0 = *p0;
                }
                return;
            }
        }
    }
}

// NOTE: Inspect the assembly for optimization tips;
template <int NL, int MC> void cPointSearch_t<NL, MC>::mergeSort(__int64* pIndexRank, int32_t ptCount) {
#ifdef USEDLINK
    mergeSort3(pIndexRank, ptCount);
#else
    mergeSort2(pIndexRank, ptCount);
#endif
}
template <int NL, int MC> void cPointSearch_t<NL, MC>::mergeSort2(__int64* pIndexRank, int32_t ptCount) {
#ifdef MYPROFILE
    countMerge[7]++;
#endif

    register __int64* pExistSrc = (__int64*)pSortedBase[sortedSet];
    sortedSet++;
    sortedSet &= 1;
    register __int64* pDst = (__int64*)pSortedBase[sortedSet];

    register __int64 existSrc = *pExistSrc;
    register __int64 newSrc = *pIndexRank;
    int32_t tempMaxReqCount = maxReqCount;
    for (register int32_t kk = 0; kk < tempMaxReqCount;) {
        if (existSrc < newSrc) {
            *pDst = existSrc;
            pExistSrc++;
            pDst++;
            existSrc = *pExistSrc;
            kk++;
        }
        else {
            *pDst = newSrc;
            pIndexRank++;
            pDst++;
            ptCount--;
            newSrc = *pIndexRank;
            kk++;
            if (ptCount == 0) { // copy the rest
                for (; kk < tempMaxReqCount; kk++) {
                    existSrc = *pExistSrc;
                    *pDst = existSrc;
                    pExistSrc++;
                    pDst++;
                }
                lastBiggest = existSrc;
                return;
            }
        }
    }
    pDst--;
    lastBiggest = *pDst;
}
template <int NL, int MC> void cPointSearch_t<NL, MC>::mergeSort3(__int64* pIndexRank, int32_t ptCount) {
    // TODO: insert from the backwards.
    sortedNode_t* pCur = pSnTail;
    
    __int64 newSrc = *pIndexRank;

    while (pCur) {
        if (pCur->indexRank64 > newSrc) {
            pCur = pCur->previous;
        }
        else {
            sortedNode_t* pCurNext = pCur->next;
            if (pCurNext == pSnTail) {
                pSnTail->indexRank64 = newSrc;
                lastBiggest = newSrc;
                return;
            }
            sortedNode_t* pNextToTail = pSnTail->previous;

            pSnTail->indexRank64 = newSrc;
            pSnTail->next = pCurNext;
            pSnTail->previous = pCur;

            pCur->next = pSnTail;
            pCurNext->previous = pSnTail;

            pNextToTail->next = 0;
            pSnTail = pNextToTail;

            ptCount--;
            if (ptCount == 0) {
                lastBiggest = pSnTail->indexRank64;
                return;
            }

            pIndexRank++;
            newSrc = *pIndexRank;
            if (newSrc >= pSnTail->indexRank64) {
                lastBiggest = pSnTail->indexRank64;
                return;
            }
            pCur = pCurNext;
            break;
        }
    }

    while (pCur) {
        if (pCur->indexRank64 <= newSrc) {
            pCur = pCur->next;
        }
        else {
            if (pCur == pSnTail) {
                pSnTail->indexRank64 = newSrc;
                break;
            }
            sortedNode_t* pCurPrevious = pCur->previous;
            sortedNode_t* pNextToTail = pSnTail->previous;

            pSnTail->indexRank64 = newSrc;
            pSnTail->next = pCur;
            pSnTail->previous = pCurPrevious;

            pCurPrevious->next = pSnTail;
            pCur->previous = pSnTail;

            pNextToTail->next = 0;
            pSnTail = pNextToTail;

            ptCount--;
            if (ptCount == 0)
                break;

            pIndexRank++;
            newSrc = *pIndexRank;
            if (newSrc >= pSnTail->indexRank64)
                break;
        }
    }
    lastBiggest = pSnTail->indexRank64;
}
template <int NL, int MC> template <class T> void cPointSearch_t<NL, MC>::mergeSortCondTemp(__int64* pIndexRank, int32_t ptCount) {
#ifdef USEDLINK
    mergeSortCond3Temp<T>(pIndexRank, ptCount);
#else
    mergeSortCond2Temp<T>(pIndexRank, ptCount);
#endif
}

template <int NL, int MC> template <class T> void cPointSearch_t<NL, MC>::mergeSortCond2Temp(__int64* pIndexRank, int32_t ptCount) {
#ifdef MYPROFILE
    countMerge[0]++;
#endif
    
    __int64 eExisted = lastBiggest;

    bool found = false;
    int32_t TempPtCount = ptCount;
    for (int32_t ii = 0; ii < TempPtCount; ii++) {
#ifdef MYPROFILE
        countMerge[1]++;
#endif
        __int64 eNew = *pIndexRank;
        if (eNew >= eExisted)
            return;

#ifdef MYPROFILE
        countMerge[2]++;
#endif

        //int32_t index = ((indexRank_t*)pIndexRank)->index;
        int32_t index = eNew & 0xffffffff;
        if (T()(pXBase, pYBase, index, rect)) {
            found = true;
            break;
        }

        pIndexRank++;
        ptCount--;
    }
    if (!found)
        return;

#ifdef MYPROFILE
    countMerge[3]++;
#endif

    register __int64* pExistSrc = (__int64*)pSortedBase[sortedSet];
    sortedSet++;
    sortedSet &= 1;
    register __int64* pDst = (__int64*)pSortedBase[sortedSet];


    register __int64 existSrc = *pExistSrc;
    register __int64 newSrc = *pIndexRank;
    pIndexRank++;
    ptCount--;

    for (register int32_t kk = 0; kk < maxReqCount; kk++) {
        if (existSrc < newSrc) {
            *pDst = existSrc;
            pExistSrc++;
            pDst++;
            existSrc = *pExistSrc;
        }
        else {
#ifdef MYPROFILE
            countMerge[4]++;
#endif
            *pDst = newSrc;
            pDst++;

            bool found2 = false;
            int32_t TempPtCount2 = ptCount;
            for (int32_t ii = 0; ii < TempPtCount2; ii++) {
#ifdef MYPROFILE
                countMerge[5]++;
#endif
                newSrc = *pIndexRank;
                if (newSrc >= eExisted)
                    break;

                //int32_t index = ((indexRank_t*)pIndexRank)->index;
                int32_t index = newSrc & 0xffffffff;
                if (T()(pXBase, pYBase, index, rect)) {
                    found2 = true;
                    break;
                }

                pIndexRank++;
                ptCount--;
            }
            if (found2) {
                newSrc = *pIndexRank;
                pIndexRank++;
                ptCount--;
            }
            else {
#ifdef MYPROFILE
                countMerge[6]++;
#endif
                kk++;
                for (; kk < maxReqCount; kk++) {
                    existSrc = *pExistSrc;
                    *pDst = existSrc;
                    pExistSrc++;
                    pDst++;
                }
                lastBiggest = existSrc;
                return;
            }
        }
    }
    pDst--;
    lastBiggest = *pDst;
}
template <int NL, int MC> template <class T> void cPointSearch_t<NL, MC>::mergeSortCond3Temp(__int64* pIndexRank, int32_t ptCount) {
    bool found = false;
    int32_t TempPtCount = ptCount;
    for (int32_t ii = 0; ii < TempPtCount; ii++) {
        int32_t index = ((indexRank_t*)pIndexRank)->index;
        //if ((this->*cond)(index)) {
        if (T()(pXBase, pYBase, index, rect)) {
            found = true;
            break;
        }

        pIndexRank++;
        ptCount--;
    }
    if (!found)
        return;

    __int64 eExisted = lastBiggest;
    __int64 newSrc = *pIndexRank;

    if (newSrc > eExisted) {
        return;
    }
    sortedNode_t* pCur = pSnHead->next;

    while (pCur) {
        if (pCur->indexRank64 <= newSrc) {
            pCur = pCur->next;
        }
        else {
            if (pCur == pSnTail) {
                pSnTail->indexRank64 = newSrc;
                break;
            }
            sortedNode_t* pCurPrevious = pCur->previous;
            sortedNode_t* pNextToTail = pSnTail->previous;

            pSnTail->indexRank64 = newSrc;
            pSnTail->next = pCur;
            pSnTail->previous = pCurPrevious;

            pCurPrevious->next = pSnTail;
            pCur->previous = pSnTail;

            pNextToTail->next = 0;
            pSnTail = pNextToTail;

            ptCount--;
            if (ptCount == 0)
                break;

            pIndexRank++;

            bool found2 = false;
            int32_t TempPtCount2 = ptCount;
            for (int32_t ii = 0; ii < TempPtCount2; ii++) {
                int32_t index = ((indexRank_t*)pIndexRank)->index;
                if (T()(pXBase, pYBase, index, rect)) {
                    found2 = true;
                    break;
                }
                pIndexRank++;
                ptCount--;
            }
            if (found2) {
                newSrc = *pIndexRank;
                if (newSrc >= pSnTail->indexRank64)
                    break;
            }
            else {
                break;
            }
        }
    }
    lastBiggest = pSnTail->indexRank64;
}

template <int NL, int MC> void cPointSearch_t<NL, MC>::mergeOneCell(int32_t ii, int32_t jj) {
#ifdef MYPROFILE
    countMerge[15] ++;
#endif
    int32_t ptCount = cell[ii][jj].ptCount;

    if (ptCount) {
        int32_t startIndex = cell[ii][jj].startIndex;
        __int64* pTempRank = (__int64*)pIndexRankBase + startIndex;

        __int64 eExisted = lastBiggest;
        __int64 eNew = *pTempRank;
        if (eNew < eExisted) {
            mergeSort(pTempRank, ptCount);
        }
    }
}

template <int NL, int MC> void cPointSearch_t<NL, MC>::mergeColCell(int32_t jj, int32_t yil, int32_t yih, int32_t step) {
    static_assert((XYSTEP1 & XYSTEP1M1) == 0, "XYSTEP1 must be a power of 2");
    static_assert((XYSTEP2 & XYSTEP2M1) == 0, "XYSTEP2 must be a power of 2");
    
    if (step == 0) {
        for (int32_t ii = yil; ii < yih; ii++) {
            int32_t ptCount = cell[ii][jj].ptCount;
            if (ptCount) {
                int32_t startIndex = cell[ii][jj].startIndex;
                __int64* pTempRank = (__int64*)pIndexRankBase + startIndex;

                __int64 eExisted = lastBiggest;
                __int64 eNew = *pTempRank;
                if (eNew < eExisted) {
                    mergeSort(pTempRank, ptCount);
                }
            }
        }
        return;
    }

    int32_t ys;
    int32_t ye;
    int32_t ysn;
    int32_t yen;
    int32_t sizeNN = NN + 2;
    linearAggCellNode_t* pLinearAggCell;
    if (step == 2) {
        // check step2 first;
        ys = (yil + XYSTEP2M1) >> XYSTEP2BITS;
        ye = yih >> XYSTEP2BITS;
        yen = ys << XYSTEP2BITS;
        ysn = ye << XYSTEP2BITS;
        //sizeNN = yAggCells[1].sizeNN;
        pLinearAggCell = &yAggCells[1];
    }
    else {  // step = 1;
        ys = (yil + XYSTEP1M1) >> XYSTEP1BITS;
        ye = yih >> XYSTEP1BITS;
        yen = ys << XYSTEP1BITS;
        ysn = ye << XYSTEP1BITS;
        //sizeNN = yAggCells[1].sizeNN;
        pLinearAggCell = &yAggCells[0];
    }
    
    for (int32_t ii = ys; ii < ye; ii++) {
        int32_t ptCount = pLinearAggCell->pAggCellNode[ii * sizeNN + jj].ptCount;
        if (ptCount) {
            int32_t startIndex = pLinearAggCell->pAggCellNode[ii * sizeNN + jj].startIndex;
            __int64* pTempRank = (__int64*)pLinearAggCell->pAggIndexRank + startIndex;

            __int64 eExisted = lastBiggest;
            __int64 eNew = *pTempRank;
            if (eNew < eExisted) {
                mergeSort(pTempRank, ptCount);
            }
        }
    }
    if (ys >= ye) {
        mergeColCell(jj, yil, yih, step-1);
    }
    else {
        mergeColCell(jj, yil, yen, step - 1);
        mergeColCell(jj, ysn, yih, step - 1);
    }
}

template <int NL, int MC> void cPointSearch_t<NL, MC>::mergeRowCell(int32_t xil, int32_t xih, int32_t jj, int32_t step) {
    static_assert((XYSTEP1 & XYSTEP1M1) == 0, "XYSTEP1 must be a power of 2");
    static_assert((XYSTEP2 & XYSTEP2M1) == 0, "XYSTEP2 must be a power of 2");

    if (step == 0) {
        for (int32_t ii = xil; ii < xih; ii++) {
            int32_t ptCount = cell[jj][ii].ptCount;
            if (ptCount) {
                int32_t startIndex = cell[jj][ii].startIndex;
                __int64* pTempRank = (__int64*)pIndexRankBase + startIndex;

                __int64 eExisted = lastBiggest;
                __int64 eNew = *pTempRank;
                if (eNew < eExisted) {
                    mergeSort(pTempRank, ptCount);
                }
            }
        }
        return;
    }

    int32_t xs;
    int32_t xe;
    int32_t xsn;
    int32_t xen;
    int32_t sizeNN;
    linearAggCellNode_t* pLinearAggCell;
    if (step == 2) {
        // check step2 first;
        xs = (xil + XYSTEP2M1) >> XYSTEP2BITS;
        xe = xih >> XYSTEP2BITS;
        xen = xs << XYSTEP2BITS;
        xsn = xe << XYSTEP2BITS;
        sizeNN = xAggCells[1].sizeNN;
        pLinearAggCell = &xAggCells[1];
    }
    else {  // step = 1;
        xs = (xil + XYSTEP1M1) >> XYSTEP1BITS;
        xe = xih >> XYSTEP1BITS;
        xen = xs << XYSTEP1BITS;
        xsn = xe << XYSTEP1BITS;
        sizeNN = xAggCells[0].sizeNN;
        pLinearAggCell = &xAggCells[0];
    }

    for (int32_t ii = xs; ii < xe; ii++) {
        int32_t ptCount = pLinearAggCell->pAggCellNode[ii + sizeNN * jj].ptCount;
        if (ptCount) {
            int32_t startIndex = pLinearAggCell->pAggCellNode[ii + sizeNN * jj].startIndex;
            __int64* pTempRank = (__int64*)pLinearAggCell->pAggIndexRank + startIndex;

            __int64 eExisted = lastBiggest;
            __int64 eNew = *pTempRank;
            if (eNew < eExisted) {
                mergeSort(pTempRank, ptCount);
            }
        }
    }
    if (xs >= xe) {
        mergeRowCell(xil, xih, jj, step - 1);
    }
    else {
        mergeRowCell(xil, xen, jj, step - 1);
        mergeRowCell(xsn, xih, jj, step - 1);
    }
}

template <int NL, int MC> template<class T> void cPointSearch_t<NL, MC>::mergeColCell_temp(int32_t jj, int32_t yil, int32_t yih, int32_t step) {
    static_assert((XYSTEP1 & XYSTEP1M1) == 0, "XYSTEP1 must be a power of 2");
    static_assert((XYSTEP2 & XYSTEP2M1) == 0, "XYSTEP2 must be a power of 2");

    if (step == 0) {
        for (int32_t ii = yil; ii < yih; ii++) {
            int32_t ptCount = cell[ii][jj].ptCount;
            if (ptCount) {
                int32_t startIndex = cell[ii][jj].startIndex;
                __int64* pTempRank = (__int64*)pIndexRankBase + startIndex;

                __int64 eExisted = lastBiggest;
                __int64 eNew = *pTempRank;
                if (eNew < eExisted) {
                    mergeSortCondTemp<T>(pTempRank, ptCount);
                }
            }
        }
        return;
    }

    int32_t ys;
    int32_t ye;
    int32_t ysn;
    int32_t yen;
    int32_t sizeNN = NN + 2;
    linearAggCellNode_t* pLinearAggCell;
    if (step == 2) {
        // check step2 first;
        ys = (yil + XYSTEP2M1) >> XYSTEP2BITS;
        ye = yih >> XYSTEP2BITS;
        yen = ys << XYSTEP2BITS;
        ysn = ye << XYSTEP2BITS;
        //sizeNN = yAggCells[1].sizeNN;
        pLinearAggCell = &yAggCells[1];
    }
    else {  // step = 1;
        ys = (yil + XYSTEP1M1) >> XYSTEP1BITS;
        ye = yih >> XYSTEP1BITS;
        yen = ys << XYSTEP1BITS;
        ysn = ye << XYSTEP1BITS;
        //sizeNN = yAggCells[1].sizeNN;
        pLinearAggCell = &yAggCells[0];
    }

    for (int32_t ii = ys; ii < ye; ii++) {
        int32_t ptCount = pLinearAggCell->pAggCellNode[ii * sizeNN + jj].ptCount;
        if (ptCount) {
            int32_t startIndex = pLinearAggCell->pAggCellNode[ii * sizeNN + jj].startIndex;
            __int64* pTempRank = (__int64*)pLinearAggCell->pAggIndexRank + startIndex;

            __int64 eExisted = lastBiggest;
            __int64 eNew = *pTempRank;
            if (eNew < eExisted) {
                mergeSortCondTemp<T>(pTempRank, ptCount);
            }
        }
    }
    if (ys >= ye) {
        mergeColCell_temp<T>(jj, yil, yih, step - 1);
    }
    else {
        mergeColCell_temp<T>(jj, yil, yen, step - 1);
        mergeColCell_temp<T>(jj, ysn, yih, step - 1);
    }
}

template <int NL, int MC> template<class T> void cPointSearch_t<NL, MC>::mergeRowCell_temp(int32_t xil, int32_t xih, int32_t jj, int32_t step) {
    static_assert((XYSTEP1 & XYSTEP1M1) == 0, "XYSTEP1 must be a power of 2");
    static_assert((XYSTEP2 & XYSTEP2M1) == 0, "XYSTEP2 must be a power of 2");

    if (step == 0) {
        for (int32_t ii = xil; ii < xih; ii++) {
            int32_t ptCount = cell[jj][ii].ptCount;
            if (ptCount) {
                int32_t startIndex = cell[jj][ii].startIndex;
                __int64* pTempRank = (__int64*)pIndexRankBase + startIndex;

                __int64 eExisted = lastBiggest;
                __int64 eNew = *pTempRank;
                if (eNew < eExisted) {
                    mergeSortCondTemp<T>(pTempRank, ptCount);
                }
            }
        }
        return;
    }

    int32_t xs;
    int32_t xe;
    int32_t xsn;
    int32_t xen;
    int32_t sizeNN;
    linearAggCellNode_t* pLinearAggCell;
    if (step == 2) {
        // check step2 first;
        xs = (xil + XYSTEP2M1) >> XYSTEP2BITS;
        xe = xih >> XYSTEP2BITS;
        xen = xs << XYSTEP2BITS;
        xsn = xe << XYSTEP2BITS;
        sizeNN = xAggCells[1].sizeNN;
        pLinearAggCell = &xAggCells[1];
    }
    else {  // step = 1;
        xs = (xil + XYSTEP1M1) >> XYSTEP1BITS;
        xe = xih >> XYSTEP1BITS;
        xen = xs << XYSTEP1BITS;
        xsn = xe << XYSTEP1BITS;
        sizeNN = xAggCells[0].sizeNN;
        pLinearAggCell = &xAggCells[0];
    }

    for (int32_t ii = xs; ii < xe; ii++) {
        int32_t ptCount = pLinearAggCell->pAggCellNode[ii + sizeNN * jj].ptCount;
        if (ptCount) {
            int32_t startIndex = pLinearAggCell->pAggCellNode[ii + sizeNN * jj].startIndex;
            __int64* pTempRank = (__int64*)pLinearAggCell->pAggIndexRank + startIndex;

            __int64 eExisted = lastBiggest;
            __int64 eNew = *pTempRank;
            if (eNew < eExisted) {
                mergeSortCondTemp<T>(pTempRank, ptCount);
            }
        }
    }
    if (xs >= xe) {
        mergeRowCell_temp<T>(xil, xih, jj, step - 1);
    }
    else {
        mergeRowCell_temp<T>(xil, xen, jj, step - 1);
        mergeRowCell_temp<T>(xsn, xih, jj, step - 1);
    }
}

template <int NL, int MC> void cPointSearch_t<NL, MC>::aggCellMerge(cellNode_t* pSrcNode, indexRank_t* pIndexRank) {
#ifdef MYPROFILE
    countMerge[29] ++;
#endif

    __int64* pSrc = ((__int64*)pIndexRank) + pSrcNode->startIndex;
    int32_t ptCount = pSrcNode->ptCount;
    if (ptCount) {
        __int64 eExisted = lastBiggest;
        __int64 eNew = *pSrc;
        if (eNew < eExisted) {
            mergeSort(pSrc, ptCount);
        }
    }
}

// Notes: xh is exclusive, yh is exclusive.
template <int NL, int MC> void cPointSearch_t<NL, MC>::mergeARecursive(int32_t xl, int32_t xh, int32_t yl, int32_t yh) {
    if (xl + 1 == xh) {
#ifdef USELINEAR
        mergeColCell( xl, yl, yh, 2);
#else
        for (int ii = yl; ii < yh; ii++) {
            mergeOneCell(ii, xl);        // iterate each cells.
        }
#endif
        return;
    }
    else if (yl + 1 == yh) {
#ifdef USELINEAR
        mergeRowCell( xl, xh, yl, 2 );
#else
        for (int ii = xl; ii < xh; ii++) {
            mergeOneCell(yl, ii);        // iterate each cells.
        }
#endif
        return;
    }

    // 7256 times
    // TODO: More optimization here.
    int32_t xd = xh - xl;
    int32_t yd = yh - yl;
    if (xd > yd) xd = yd;
    register int32_t maxCellSizeBits = log2Table[xd];
    //int32_t maxCellSizeBits = (int32_t)(log2(xd));

    register int32_t maxCellSize = 1 << maxCellSizeBits;

    if (maxCellSize == 1) {
        // iterate each cells.
#ifdef USELINEAR
        if (xd > yd) {
            for (int jj = yl; jj < yh; jj++) {
                mergeRowCell(xl, xh, jj, 2);
            }
        }
        else {
            for (int ii = xl; ii < xh; ii++) {
                mergeColCell(ii, yl, yh, 2);
            }
        }
#else
        for (int ii = xl; ii < xh; ii++) {
            for (int jj = yl; jj < yh; jj++) {
                mergeOneCell(jj, ii);
            }
        }
#endif
        return;
    }

    register int32_t xlBase = (xl + maxCellSize - 2) >> maxCellSizeBits;
    register int32_t xhBase = (xh - 1) >> maxCellSizeBits; // xhbase - exclusive
    if (xhBase == xlBase) {
        maxCellSize >>= 1;
        if (maxCellSize == 1) {
            // iterate each cells.
#ifdef USELINEAR
            if (xd > yd) {
                for (int jj = yl; jj < yh; jj++) {
                    mergeRowCell(xl, xh, jj, 2);
                }
            }
            else {
                for (int ii = xl; ii < xh; ii++) {
                    mergeColCell(ii, yl, yh, 2);
                }
            }
#else
            for (int ii = xl; ii < xh; ii++) {
                for (int jj = yl; jj < yh; jj++) {
                    mergeOneCell(jj, ii);
                }
            }
#endif
            return;
        }
        maxCellSizeBits--;
        xlBase = (xl + maxCellSize - 2) >> maxCellSizeBits;
        xhBase = (xh - 1) >> maxCellSizeBits;
    }
    register int32_t ylBase = (yl + maxCellSize - 2) >> maxCellSizeBits;
    register int32_t yhBase = (yh - 1) >> maxCellSizeBits; // yhbase - exclusive
    if (yhBase == ylBase) {
        maxCellSize >>= 1;
        if (maxCellSize == 1) {
#ifdef USELINEAR
            // iterate each cells.
            if (xd > yd) {
                for (int jj = yl; jj < yh; jj++) {
                    mergeRowCell(xl, xh, jj, 2);
                }
            }
            else {
                for (int ii = xl; ii < xh; ii++) {
                    mergeColCell(ii, yl, yh, 2);
                }
            }
#else
            for (int ii = xl; ii < xh; ii++) {
                for (int jj = yl; jj < yh; jj++) {
                    mergeOneCell(jj, ii);
                }
            }
#endif
            return;
        }
        maxCellSizeBits--;

        xlBase = (xl + maxCellSize - 2) >> maxCellSizeBits;
        xhBase = (xh - 1) >> maxCellSizeBits;
        ylBase = (yl + maxCellSize - 2) >> maxCellSizeBits;
        yhBase = (yh - 1) >> maxCellSizeBits;
    }

#ifdef MYPROFILE
    // 7235
    countResursive++;
#endif

#if 1
    aggCellNode_t* pAggCells = &aggCells[maxCellSizeBits];
    int32_t sizeNN = pAggCells->sizeNN;
    indexRank_t* pAggIndexRank = pAggCells->pAggIndexRank;
    for (int ii = xlBase; ii < xhBase; ii++) {
        cellNode_t* pCellNode = pAggCells->pAggCellNode + ii + ylBase * sizeNN;
        for (int jj = ylBase; jj < yhBase; jj++) {
#ifdef MYPROFILE
            countMerge[15 + maxCellSizeBits] ++;
#endif
            aggCellMerge(pCellNode, pAggIndexRank);
            pCellNode += sizeNN;
        }
    }
#else
    {
        int32_t xlBaseInRaw = 1 + (xlBase << maxCellSizeBits);
        int32_t ylBaseInRaw = 1 + (ylBase << maxCellSizeBits);
        int32_t xhBaseInRaw = 1 + (xhBase << maxCellSizeBits);
        int32_t yhBaseInRaw = 1 + (yhBase << maxCellSizeBits);
        for (int ii = xlBaseInRaw; ii < xhBaseInRaw; ii++) {
            for (int jj = ylBaseInRaw; jj < yhBaseInRaw; jj++) {
                mergeOneCell(jj, ii);
            }
        }
    }
#endif

    int32_t xlBaseInRaw = 1 + (xlBase << maxCellSizeBits);
    int32_t ylBaseInRaw = 1 + (ylBase << maxCellSizeBits);
    int32_t xhBaseInRaw = 1 + (xhBase << maxCellSizeBits);
    int32_t yhBaseInRaw = 1 + (yhBase << maxCellSizeBits);

    if (xl < xh) {
        if (yl < ylBaseInRaw)
            mergeARecursive(xl, xh, yl, ylBaseInRaw);
        if (yhBaseInRaw < yh)
            mergeARecursive(xl, xh, yhBaseInRaw, yh);
    }

    if (ylBaseInRaw < yhBaseInRaw) {
        if (xl < xlBaseInRaw)
            mergeARecursive(xl, xlBaseInRaw, ylBaseInRaw, yhBaseInRaw);

        if (xhBaseInRaw < xh)
            mergeARecursive(xhBaseInRaw, xh, ylBaseInRaw, yhBaseInRaw);
    }
}

/*
+---+---+---+       +---+---+---+       +---+
| F | D | G |       | K | J | L |       | N |
+---+---+---+       +---+---+---+       +---+   +---+
| B | A | C |                           | M |   | P |
+---+---+---+                           +---+   +---+
| I | E | H |                           | O |
+---+---+---+                           +---+
*/
template <int NL, int MC> void cPointSearch_t<NL, MC>::mergeA(int32_t xil, int32_t xih, int32_t yil, int32_t yih){
    for (int32_t ii = yil; ii < yih; ii++) {
        for (int32_t jj = xil; jj < xih; jj++) {
            int32_t ptCount = cell[ii][jj].ptCount;

            if (ptCount) {
                int32_t startIndex = cell[ii][jj].startIndex;
                __int64* pTempRank = (__int64*)pIndexRankBase + startIndex;

                __int64 eExisted = lastBiggest;
                __int64 eNew = *pTempRank;
                if (eNew < eExisted) {
                    mergeSort(pTempRank, ptCount);
                }
            }
        }
    }
}

template <int NL, int MC> template<class T> void cPointSearch_t<NL, MC>::mergeBCM_temp(int32_t xi, int32_t yil, int32_t yih) {
    for (int32_t ii = yil; ii < yih; ii++) {
        int32_t jj = xi;
        int32_t ptCount = cell[ii][jj].ptCount;
        if (ptCount) {
            int32_t startIndex = cell[ii][jj].startIndex;
            __int64* pTempRank = (__int64*)pIndexRankBase + startIndex;

            __int64 eExisted = lastBiggest;
            __int64 eNew = *pTempRank;
            if (eNew < eExisted) {
#ifdef MYPROFILE
                countMerge[10] ++;
#endif
                mergeSortCondTemp<T>(pTempRank, ptCount);
            }
        }
    }
}

template <int NL, int MC> template<class T> void cPointSearch_t<NL, MC>::mergeDEJ_temp(int32_t xil, int32_t xih, int32_t yi) {
    int32_t ii = yi;
    for (int32_t jj = xil; jj < xih; jj++) {
        int32_t ptCount = cell[ii][jj].ptCount;
        if (ptCount) {
            int32_t startIndex = cell[ii][jj].startIndex;
            __int64* pTempRank = (__int64*)pIndexRankBase + startIndex;

            __int64 eExisted = lastBiggest;
            __int64 eNew = *pTempRank;
            if (eNew < eExisted) {
#ifdef MYPROFILE
                countMerge[11] ++;
#endif
                mergeSortCondTemp<T>(pTempRank, ptCount);
            }
        }
    }
}

template <int NL, int MC> template<class T> void cPointSearch_t<NL, MC>::mergeCorner_temp(int32_t xi, int32_t yi) {
    int32_t ptCount = cell[yi][xi].ptCount;

    if (ptCount) {
        int32_t startIndex = cell[yi][xi].startIndex;
        __int64* pTempRank = (__int64*)pIndexRankBase + startIndex;

        __int64 eExisted = lastBiggest;
        __int64 eNew = *pTempRank;
        if (eNew < eExisted) {
#ifdef MYPROFILE
            countMerge[12] ++;
#endif
            mergeSortCondTemp<T>(pTempRank, ptCount);
        }
    }
}
template <int NL, int MC> template<class T> void cPointSearch_t<NL, MC>::mergeP_temp(int32_t xi, int32_t yi) {
    int32_t ptCount = cell[yi][xi].ptCount;
    if (ptCount) {
        int32_t startIndex = cell[yi][xi].startIndex;
        __int64* pTempRank = (__int64*)pIndexRankBase + startIndex;
#ifdef MYPROFILE
        countMerge[13] ++;
#endif
        mergeSortCondTemp<T>(pTempRank, ptCount);
    }
}

template <int NL, int MC> void cPointSearch_t<NL, MC>::searchPoints() {
    int32_t xCellIndexL;
    int32_t yCellIndexL;
    int32_t xCellIndexH;
    int32_t yCellIndexH;

    float xIdx = (rect.lx - xMin) / xStep;
    float yIdx = (rect.ly - yMin) / yStep;
#ifdef USEIDX2
    if (xIdx > IDX_TABLE_SIZE)
        xIdx = IDX_TABLE_SIZE;
    if (yIdx > IDX_TABLE_SIZE)
        yIdx = IDX_TABLE_SIZE;
#else
    if (xIdx > NN + 1)
        xIdx = NN + 1;
    if (yIdx > NN + 1)
        yIdx = NN + 1;
#endif

    xCellIndexL = int32_t(xIdx);
    yCellIndexL = int32_t(yIdx);

    xCellIndexL = myMax(xCellIndexL, 0);
    yCellIndexL = myMax(yCellIndexL, 0);
#ifdef USEIDX2
    xCellIndexL = xIdxTable[xCellIndexL]; // idx translation
    yCellIndexL = yIdxTable[yCellIndexL]; // idx translation
#endif

    if (rect.hx < xMin) return;
    if (rect.hy < yMin) return;

    xIdx = (rect.hx - xMin) / xStep;
    yIdx = (rect.hy - yMin) / yStep;
#ifdef USEIDX2
    if (xIdx > IDX_TABLE_SIZE)
        xIdx = IDX_TABLE_SIZE;
    if (yIdx > IDX_TABLE_SIZE)
        yIdx = IDX_TABLE_SIZE;
#else
    if (xIdx > NN + 1)
        xIdx = NN + 1;
    if (yIdx > NN + 1)
        yIdx = NN + 1;
#endif
    xIdx = myMax(xIdx, 0);
    yIdx = myMax(yIdx, 0);

    xCellIndexH = int32_t(xIdx);
    yCellIndexH = int32_t(yIdx);
#ifdef USEIDX2
    xCellIndexH = xIdxTable[xCellIndexH]; // idx translation
    yCellIndexH = yIdxTable[yCellIndexH]; // idx translation
#endif

    /*
    +---+---+---+       +---+---+---+       +---+
    | F | D | G |       | K | J | L |       | N |
    +---+---+---+       +---+---+---+       +---+   +---+
    | B | A | C |                           | M |   | P |
    +---+---+---+                           +---+   +---+
    | I | E | H |                           | O |
    +---+---+---+                           +---+
    */

#ifdef TRACING
    if (enableTrace) {
        std::cout << "x,y xCellIndexL:" << xCellIndexL << ", " << xCellIndexH << ", " << yCellIndexL << ", " << yCellIndexH << std::endl;
    }
#endif

    if (yCellIndexL < yCellIndexH) {
        if (xCellIndexL < xCellIndexH) {
            //mergeA(xCellIndexL+1, xCellIndexH, yCellIndexL+1, yCellIndexH); // A
            int32_t xl = xCellIndexL + 1;
            int32_t yl = yCellIndexL + 1;
            if ((xl < xCellIndexH) && (yl < yCellIndexH)) {
                if (maxReqCount <= MC) {
                    mergeARecursive(xl, xCellIndexH, yl, yCellIndexH); // A
                }
                else {
                    mergeA(xl, xCellIndexH, yl, yCellIndexH); // A
                }
            }
#ifndef USELINEAR
            mergeBCM_temp<cMergeCond_lx_t>(xCellIndexL, yl, yCellIndexH);   // B
            mergeBCM_temp<cMergeCond_hx_t>(xCellIndexH, yl, yCellIndexH);   // C

            mergeDEJ_temp<cMergeCond_ly_t>(xl, xCellIndexH, yCellIndexL);   // E
            mergeDEJ_temp<cMergeCond_hy_t>(xl, xCellIndexH, yCellIndexH);   // D
#else
            mergeColCell_temp<cMergeCond_lx_t>(xCellIndexL, yl, yCellIndexH, 2);    // B
            mergeColCell_temp<cMergeCond_hx_t>(xCellIndexH, yl, yCellIndexH, 2);    // C

            mergeRowCell_temp<cMergeCond_ly_t>(xl, xCellIndexH, yCellIndexL, 2);   // E
            mergeRowCell_temp<cMergeCond_hy_t>(xl, xCellIndexH, yCellIndexH, 2);   // D
#endif
            mergeCorner_temp<cMergeCond_lx_hy_t>(xCellIndexL, yCellIndexH);  // F
            mergeCorner_temp<cMergeCond_hx_hy_t>(xCellIndexH, yCellIndexH);  // G
            mergeCorner_temp<cMergeCond_lx_ly_t>(xCellIndexL, yCellIndexL);  // I
            mergeCorner_temp<cMergeCond_hx_ly_t>(xCellIndexH, yCellIndexL);  // H
        }
        else { // xCellIndexL = xCellIndexH
#ifndef USELINEAR
            mergeBCM_temp<cMergeCond_lx_hx_t>(xCellIndexL, yCellIndexL + 1, yCellIndexH);    // M
#else
            mergeColCell_temp<cMergeCond_lx_hx_t>(xCellIndexL, yCellIndexL + 1, yCellIndexH, 2);    // M
#endif
            mergeCorner_temp<cMergeCond_hy_lx_hx_t>(xCellIndexL, yCellIndexH);   // N
            mergeCorner_temp<cMergeCond_ly_lx_hx_t>(xCellIndexL, yCellIndexL);   // O
        }
    }
    else {  // yCellIndexL = yCellIndexH
        if (xCellIndexL < xCellIndexH) {
#ifndef USELINEAR
            mergeDEJ_temp<cMergeCond_ly_hy_t>(xCellIndexL + 1, xCellIndexH, yCellIndexH);  // J
#else
            mergeRowCell_temp<cMergeCond_ly_hy_t>(xCellIndexL + 1, xCellIndexH, yCellIndexH, 2);   // E
#endif
            mergeCorner_temp<cMergeCond_lx_ly_hy_t>(xCellIndexL, yCellIndexL);   // K
            mergeCorner_temp<cMergeCond_hx_ly_hy_t>(xCellIndexH, yCellIndexL);   // L
        }
        else { // xCellIndexL = xCellIndexH
            mergeP_temp<cMergeCond_lx_hx_ly_hy_t>(xCellIndexL, yCellIndexL);
        }
    }
}

template <int NL, int MC> int32_t cPointSearch_t<NL, MC>::find(const Rect& r, const int32_t count, Point* out_points) {
    if (maxSrcCount == 0)
        return 0;

    maxReqCount = count;
    if (maxReqCount > maxSrcCount)
        maxReqCount = maxSrcCount;

    maxReqCountM1 = maxReqCount - 1;
    lastBiggest = 0x7fffffff7fffffffull;

#ifdef USEDLINK
    
    // TODO: Many operations can be initialized in the constructor, we just need to init rankIndex
    for (int ii = 1; ii < MC; ii++) {
        sortedNode[ii].next = &sortedNode[ii + 1];
        sortedNode[ii].previous = &sortedNode[ii - 1];
        sortedNode[ii].indexRank64 = 0x7fffffff7fffffffull;
    }
    sortedNode[0].previous = 0;
    sortedNode[0].next = &sortedNode[1];
    sortedNode[MC].previous = &sortedNode[MC - 1];
    sortedNode[MC].next = 0;
    sortedNode[MC].indexRank64 = 0x7fffffff7fffffffull;

    pSnHead = &sortedNode[0];
    pSnTail = &sortedNode[MC];

    rect = r;
    searchPoints();

    sortedNode_t* pCur = pSnHead->next;
    for (int32_t ii = 0; ii < maxReqCount; ii++) {  // TODO: use while????
        int32_t index = pCur->indexRank.index;
        if (index == 0x7fffffff) {
            return ii;
        }
        out_points[ii].id = *(pIdBase + index);
        out_points[ii].rank = pCur->indexRank.rank;
        out_points[ii].x = *(pXBase + index);
        out_points[ii].y = *(pYBase + index);
        pCur = pCur->next;
    }
#else
    sortedSet = 0;

    __m128i m1;
    m1 = _mm_set1_epi32(0x7fffffff);    // TODO: need unique 
    indexRank_t* p = pSortedBase[sortedSet];
    for (int32_t ii = 0; ii <= (maxReqCount >> 1); ii++) {
        _mm_store_si128((__m128i*)p, m1);
        p += 2;
    }

    rect = r;
    searchPoints();

    indexRank_t* pFound = pSortedBase[sortedSet];
    for (int32_t ii = 0; ii < maxReqCount; ii++) {
        int32_t index = pFound[ii].index;
        if (index == 0x7fffffff) {
            return ii;
        }
        out_points[ii].id = *(pIdBase + index);
        out_points[ii].rank = pFound[ii].rank;
        out_points[ii].x = *(pXBase + index);
        out_points[ii].y = *(pYBase + index);
    }
#endif
    return maxReqCount;
}

