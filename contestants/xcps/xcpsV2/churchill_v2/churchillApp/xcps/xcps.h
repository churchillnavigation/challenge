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
    int32_t factor;
    int32_t sizeNN;
    cellNode_t* pAggCellNode;   // size of the array is: sizeNN x sizeNN
    indexRank_t* pAggIndexRank;
};


template <int NL, int MC> class cPointSearch_t {
#define LOG_NN_P1   (NL)    // NL=10, NN=512;    NL=9, NN=256; ...
#define NN          (1<<((LOG_NN_P1)-1))

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
    aggCellNode_t aggCells[LOG_NN_P1];
    cellNode_t cell[NN + 2][NN + 2];

#ifdef TRACING
    bool enableTrace;
#endif


#ifdef MYPROFILE
    int countMergeSort;
    int countResursive;
#endif

    void bubbleDown(__int64* pHeapBase, int32_t heapSize, int32_t pos);
    void heapSort(__int64* pHeapBase, int32_t heapSize);

    void mergeSort(__int64* pIndexRank, int32_t ptCount);
    void mergeSort2(__int64* pIndexRank, int32_t ptCount);
    void mergeSortCond(__int64* pIndexRank, int32_t ptCount);
    void mergeSortCond2(__int64* pIndexRank, int32_t ptCount);
    
    template <class T> void mergeSortCondTemp(__int64* pIndexRank, int32_t ptCount);
    template <class T> void mergeSortCond2Temp(__int64* pIndexRank, int32_t ptCount);

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

    void bundleUp();
    void mergeTwoCells(__int64* pSrc0, int32_t c0, __int64* pSrc1, int32_t c1, __int64* pDst);
    void mergeCells(cellNode_t* pSrcNode, __int64* pSrcData, int32_t idx0, int32_t idx1, cellNode_t* pDstNode, int32_t idx2, __int64* pDstData, int32_t& pos);
    void aggCellMerge(cellNode_t* pSrcNode, indexRank_t* pIndexRank);
    void mergeOneCell(int32_t ii, int32_t jj);

    void searchPoints();

public:
    //--------------------------------------------------------------------------------
    cPointSearch_t(int32_t count, const Point* points_begin);
    ~cPointSearch_t();

    int32_t find(const Rect& r, const int32_t count, Point* out_points);
    void add(const Point* pt );

    void alloc(int32_t count);
    void deAlloc();
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
    countMergeSort = 0;
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

template <int NL, int MC> void cPointSearch_t<NL, MC>::add(const Point* pt) {
#if 0
    {
        pPtBase = pt;
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
    int32_t mm = 0;
    for (int32_t ii = 0; ii < NN + 2; ii++) {
        for (int32_t jj = 0; jj < NN + 2; jj++) {
            cell[ii][jj].ptCount = 0;
            pCellNext[mm] = 0;
            mm++;
        }
    }
    // find max and min
    xMin = FLT_MAX;
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
    }

    xStep = (xMax - xMin) / NN;
    yStep = (yMax - yMin) / NN;

    const Point* pTempPt2 = pt;
    for (int32_t ii = 0; ii < maxSrcCount; ii++) {
        int32_t xCellIndex;
        int32_t yCellIndex;
        float xIdx = (pTempPt2->x - xMin) / xStep;
        float yIdx = (pTempPt2->y - yMin) / yStep;
        if (xIdx > NN + 1)
            xIdx = NN + 1;
        if (yIdx > NN + 1)
            yIdx = NN + 1;

        xCellIndex = int32_t(xIdx);
        yCellIndex = int32_t(yIdx);

        xCellIndex = myMax(xCellIndex, 0);
        yCellIndex = myMax(yCellIndex, 0);

        pPtIndexBase[ii].index = ii;
        pPtIndexBase[ii].next = 0;

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
}

template <int NL, int MC> void cPointSearch_t<NL, MC>::bundleUp() {
    // allocate the proper memory space now.
    for (int32_t ii = 0; ii < LOG_NN_P1; ii++) {
        int sizeNN = (NN >> ii);
        XC::verify(sizeNN > 0, "LOG_NN_P1 setting wrong");
        aggCells[ii].factor = ii;
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
    register __int64* pExistSrc = (__int64*)pSortedBase[sortedSet];
    sortedSet++;
    sortedSet &= 1;
    register __int64* pDst = (__int64*)pSortedBase[sortedSet];

    register __int64 existSrc = *pExistSrc;
    register __int64 newSrc = *pIndexRank;
    for (register int32_t kk = 0; kk < maxReqCount; kk++) {
        if (existSrc < newSrc) {
            *pDst = existSrc;
            pExistSrc++;
            pDst++;
            existSrc = *pExistSrc;
        }
        else {
            *pDst = newSrc;
            pIndexRank++;
            pDst++;
            ptCount--;
            newSrc = *pIndexRank;

            if (ptCount == 0) { // copy the rest
#if 0
                kk++;
                for (; kk < maxReqCount; kk += 2) {
                    __m128i m1;
                    m1 = _mm_loadu_si128((__m128i*)pExistSrc);
                    pExistSrc += 2;

                    _mm_storeu_si128((__m128i*)pDst, m1);
                    pDst += 2;
                }
#else
                kk++;
                for (; kk < maxReqCount; kk++) {
                    existSrc = *pExistSrc;
                    *pDst = existSrc;
                    pExistSrc++;
                    pDst++;
                }
#endif
                return;
            }
        }
    }
}
template <int NL, int MC> void cPointSearch_t<NL, MC>::mergeSort2(__int64* pIndexRank, int32_t ptCount) {
    register __int64* pExistSrc = (__int64*)pSortedBase[0];
    register __int64 newSrc = *pIndexRank;
    register __int64 existSrc = *pExistSrc;

    pExistSrc++;
    register int32_t kk = 0;
    for (; kk < maxReqCount; kk++) {
        if (existSrc < newSrc) {
            existSrc = *pExistSrc;
            pExistSrc++;
        }
        else {
            break;
        }
    }

    int32_t loopCount = (maxReqCount - kk) >> 1;
    __int64* pSrc = --pExistSrc;
    __int64* pDst = (__int64*)pSortedBase[1];
    for (register int32_t mm = 0; mm < loopCount; mm++) {
        __m128i m1;
        m1 = _mm_loadu_si128((__m128i*) pSrc);
        pSrc += 2;
        _mm_storeu_si128((__m128i*)pDst, m1);
        pDst += 2;
    }

    //pDst = pExistSrc;
    register __int64* pNewSrc = (__int64*)pSortedBase[1];

    // merge over 
    for (; kk < maxReqCount; kk++) {
        if (existSrc < newSrc) {
            *pExistSrc = existSrc;
            pNewSrc++;
            pExistSrc++;
            existSrc = *pNewSrc;
        }
        else {
            *pExistSrc = newSrc;
            pIndexRank++;
            pExistSrc++;
            ptCount--;
            newSrc = *pIndexRank;

            if (ptCount == 0) { // copy the rest
                for (; kk < maxReqCountM1; kk++) {
                    existSrc = *pNewSrc;
                    *pExistSrc = existSrc;
                    pNewSrc++;
                    pExistSrc++;
                }
                return;
            }
        }
    }
}
template <int NL, int MC> template <class T> void cPointSearch_t<NL, MC>::mergeSortCondTemp(__int64* pIndexRank, int32_t ptCount) {
    __int64 eExisted = *(((__int64*)pSortedBase[sortedSet]) + maxReqCountM1);

    bool found = false;
    int32_t TempPtCount = ptCount;
    for (int32_t ii = 0; ii < TempPtCount; ii++) {
        __int64 eNew = *pIndexRank;
        if (eNew >= eExisted)
            return;

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
    countMergeSort++;
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
            *pDst = newSrc;
            pDst++;

            bool found2 = false;
            int32_t TempPtCount2 = ptCount;
            for (int32_t ii = 0; ii < TempPtCount2; ii++) {
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
                kk++;
                for (; kk < maxReqCount; kk++) {
                    existSrc = *pExistSrc;
                    *pDst = existSrc;
                    pExistSrc++;
                    pDst++;
                }
                return;
            }
        }
    }
}
template <int NL, int MC> template <class T> void cPointSearch_t<NL, MC>::mergeSortCond2Temp(__int64* pIndexRank, int32_t ptCount) {
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

    __int64 eExisted = *(((__int64*)pSortedBase[0]) + maxReqCountM1);
    __int64 eNew = *pIndexRank;
    pIndexRank++;
    ptCount--;

    if (eNew > eExisted) {
        return;
    }

    register __int64* pExistSrc = (__int64*)pSortedBase[0];
    register __int64 existSrc = *pExistSrc;
    pExistSrc++;
    register int32_t kk = 0;
    for (; kk < maxReqCount; kk++) {
        if (existSrc < eNew) {
            existSrc = *pExistSrc;
            pExistSrc++;
        }
        else {
            break;
        }
    }

    int32_t loopCount = (maxReqCount - kk) >> 1;
    __int64* pSrc = --pExistSrc;
    __int64* pDst = (__int64*)pSortedBase[1];
    for (register int32_t mm = 0; mm < loopCount; mm++) {
        __m128i m1;
        m1 = _mm_loadu_si128((__m128i*) pSrc);
        pSrc += 2;
        _mm_storeu_si128((__m128i*)pDst, m1);
        pDst += 2;
    }

    register __int64* pNewSrc = (__int64*)pSortedBase[1];
    //register __int64* pDst = pExistSrc;

    for (; kk < maxReqCount; kk++) {
        if (existSrc < eNew) {
            *pExistSrc = existSrc;
            pNewSrc++;
            pExistSrc++;
            existSrc = *pNewSrc;
        }
        else {
            *pExistSrc = eNew;
            pExistSrc++;

            bool found2 = false;
            int32_t TempPtCount2 = ptCount;
            for (int32_t ii = 0; ii < TempPtCount2; ii++) {
                int32_t index = ((indexRank_t*)pIndexRank)->index;
                //if ((this->*cond)(index)) {
                if (T()(pXBase, pYBase, index, rect)) {
                    found2 = true;
                    break;
                }
                pIndexRank++;
                ptCount--;
            }
            if (found2) {
                eNew = *pIndexRank;
                pIndexRank++;
                ptCount--;
            }
            else {
                for (; kk < maxReqCountM1; kk++) {
                    existSrc = *pNewSrc;
                    *pExistSrc = existSrc;
                    pNewSrc++;
                    pExistSrc++;
                }
                return;
            }
        }
    }
}
template <int NL, int MC> void cPointSearch_t<NL, MC>::mergeSortCond(__int64* pIndexRank, int32_t ptCount) {
    bool found = false;
    int32_t TempPtCount = ptCount;
    for (int32_t ii = 0; ii < TempPtCount; ii++) {
        int32_t index = ((indexRank_t*)pIndexRank)->index;
        if ((this->*cond)(index)) {
            found = true;
            break;
        }

        pIndexRank++;
        ptCount--;
    }
    if (!found)
        return;

    __int64 eExisted = *(((__int64*)pSortedBase[sortedSet]) + maxReqCountM1);
    __int64 eNew = *pIndexRank;
    if (eNew > eExisted) {
        return;
    }

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
            *pDst = newSrc;
            pDst++;

            bool found2 = false;
            int32_t TempPtCount2 = ptCount;
            for (int32_t ii = 0; ii < TempPtCount2; ii++) {
                int32_t index = ((indexRank_t*)pIndexRank)->index;
                if ((this->*cond)(index)) {
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
                for (register int32_t mm = kk; mm < maxReqCountM1; mm++) {
                    existSrc = *pExistSrc;
                    *pDst = existSrc;
                    pExistSrc++;
                    pDst++;
                }
                return;
            }
        }
    }
}
template <int NL, int MC> void cPointSearch_t<NL, MC>::mergeSortCond2(__int64* pIndexRank, int32_t ptCount) {
    bool found = false;
    int32_t TempPtCount = ptCount;
    for (int32_t ii = 0; ii < TempPtCount; ii++) {
        int32_t index = ((indexRank_t*)pIndexRank)->index;
        if ((this->*cond)(index)) {
            found = true;
            break;
        }

        pIndexRank++;
        ptCount--;
    }
    if (!found)
        return;

    __int64 eExisted = *(((__int64*)pSortedBase[0]) + maxReqCountM1);
    __int64 eNew = *pIndexRank;
    pIndexRank++;
    ptCount--;

    if (eNew > eExisted) {
        return;
    }

    register __int64* pExistSrc = (__int64*)pSortedBase[0];
    register __int64 existSrc = *pExistSrc;

    pExistSrc++;
    register int32_t kk = 0;
    for (; kk < maxReqCount; kk++) {
        if (existSrc < eNew) {
            existSrc = *pExistSrc;
            pExistSrc++;
        }
        else {
            break;
        }
    }

    int32_t loopCount = (maxReqCount - kk) >> 1;
    __int64* pSrc = --pExistSrc;
    __int64* pDst = (__int64*)pSortedBase[1];
    for (register int32_t mm = 0; mm < loopCount; mm++) {
        __m128i m1;
        m1 = _mm_loadu_si128((__m128i*) pSrc);
        pSrc += 2;
        _mm_storeu_si128((__m128i*)pDst, m1);
        pDst += 2;
    }

    register __int64* pNewSrc = (__int64*)pSortedBase[1];
    //register __int64* pDst = pExistSrc;

    for (; kk < maxReqCount; kk++) {
        if (existSrc < eNew) {
            *pExistSrc = existSrc;
            pNewSrc++;
            pExistSrc++;
            existSrc = *pNewSrc;
        }
        else {
            *pExistSrc = eNew;
            pExistSrc++;

            bool found2 = false;
            int32_t TempPtCount2 = ptCount;
            for (int32_t ii = 0; ii < TempPtCount2; ii++) {
                int32_t index = ((indexRank_t*)pIndexRank)->index;
                if ((this->*cond)(index)) {
                    found2 = true;
                    break;
                }
                pIndexRank++;
                ptCount--;
            }
            if (found2) {
                eNew = *pIndexRank;
                pIndexRank++;
                ptCount--;
            }
            else {
                for (; kk < maxReqCountM1; kk++) {
                    existSrc = *pNewSrc;
                    *pExistSrc = existSrc;
                    pNewSrc++;
                    pExistSrc++;
                }
                return;
            }
        }
    }
}

template <int NL, int MC> void cPointSearch_t<NL, MC>::mergeOneCell(int32_t ii, int32_t jj) {
    int32_t ptCount = cell[ii][jj].ptCount;

    if (ptCount) {
        int32_t startIndex = cell[ii][jj].startIndex;
        __int64* pTempRank = (__int64*)pIndexRankBase + startIndex;

        __int64 eExisted = *(((__int64*)pSortedBase[sortedSet]) + maxReqCountM1);
        __int64 eNew = *pTempRank;
        if (eNew < eExisted) {
            mergeSort(pTempRank, ptCount);
        }
    }
}

template <int NL, int MC> void cPointSearch_t<NL, MC>::aggCellMerge(cellNode_t* pSrcNode, indexRank_t* pIndexRank) {
    __int64* pSrc = ((__int64*)pIndexRank) + pSrcNode->startIndex;
    int32_t ptCount = pSrcNode->ptCount;
    if (ptCount) {
        __int64 eExisted = *(((__int64*)pSortedBase[sortedSet]) + maxReqCountM1);
        __int64 eNew = *pSrc;
        if (eNew < eExisted) {
            mergeSort(pSrc, ptCount);
        }
    }
}

// Notes: xh is exclusive, yh is exclusive.
template <int NL, int MC> void cPointSearch_t<NL, MC>::mergeARecursive(int32_t xl, int32_t xh, int32_t yl, int32_t yh) {
    if (xl + 1 == xh) {
        for (int ii = yl; ii < yh; ii++) {
            mergeOneCell(ii, xl);        // iterate each cells.
        }
        return;
    }
    else if (yl + 1 == yh) {
        for (int ii = xl; ii < xh; ii++) {
            mergeOneCell(yl, ii);        // iterate each cells.
        }
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
        for (int ii = xl; ii < xh; ii++) {
            for (int jj = yl; jj < yh; jj++) {
                mergeOneCell(jj, ii);
            }
        }
        return;
    }

    register int32_t xlBase = (xl + maxCellSize - 2) >> maxCellSizeBits;
    register int32_t xhBase = (xh - 1) >> maxCellSizeBits; // xhbase - exclusive
    if (xhBase == xlBase) {
        maxCellSize >>= 1;
        if (maxCellSize == 1) {
            // iterate each cells.
            for (int ii = xl; ii < xh; ii++) {
                for (int jj = yl; jj < yh; jj++) {
                    mergeOneCell(jj, ii);
                }
            }
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
            // iterate each cells.
            for (int ii = xl; ii < xh; ii++) {
                for (int jj = yl; jj < yh; jj++) {
                    mergeOneCell(jj, ii);
                }
            }
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

    // TODO: optimize the all of the shifts, with one operations using simd
#if 0
    __m128i m1, m2, m3;
    m1 = _mm_set_epi32( xlBase, xhBase, ylBase, yhBase );
    m2 = _mm_slli_epi32( m1, maxCellSizeBits);
    m3 = _mm_set1_epi32(1);
    m1 = _mm_add_epi32(m2, m3);

    int32_t xlBaseInRaw = _mm_extract_epi32(m1, 3);
    int32_t ylBaseInRaw = _mm_extract_epi32(m1, 1);
    int32_t xhBaseInRaw = _mm_extract_epi32(m1, 2);
    int32_t yhBaseInRaw = _mm_cvtsi128_si32(m1);
#else
    int32_t xlBaseInRaw = 1 + (xlBase << maxCellSizeBits);
    int32_t ylBaseInRaw = 1 + (ylBase << maxCellSizeBits);
    int32_t xhBaseInRaw = 1 + (xhBase << maxCellSizeBits);
    int32_t yhBaseInRaw = 1 + (yhBase << maxCellSizeBits);
#endif

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

                __int64 eExisted = *(((__int64*)pSortedBase[sortedSet]) + maxReqCountM1);
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

            __int64 eExisted = *(((__int64*)pSortedBase[sortedSet]) + maxReqCountM1);
            __int64 eNew = *pTempRank;
            if (eNew < eExisted) {
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

            __int64 eExisted = *(((__int64*)pSortedBase[sortedSet]) + maxReqCountM1);
            __int64 eNew = *pTempRank;
            if (eNew < eExisted) {
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

        __int64 eExisted = *(((__int64*)pSortedBase[sortedSet]) + maxReqCountM1);
        __int64 eNew = *pTempRank;
        if (eNew < eExisted) {
            mergeSortCondTemp<T>(pTempRank, ptCount);
        }
    }
}
template <int NL, int MC> template<class T> void cPointSearch_t<NL, MC>::mergeP_temp(int32_t xi, int32_t yi) {
    int32_t ptCount = cell[yi][xi].ptCount;
    if (ptCount) {
        int32_t startIndex = cell[yi][xi].startIndex;
        __int64* pTempRank = (__int64*)pIndexRankBase + startIndex;
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
    if (xIdx > NN + 1)
        xIdx = NN + 1;
    if (yIdx > NN + 1)
        yIdx = NN + 1;

    xCellIndexL = int32_t(xIdx);
    yCellIndexL = int32_t(yIdx);

    xCellIndexL = myMax(xCellIndexL, 0);
    yCellIndexL = myMax(yCellIndexL, 0);

    if (rect.hx < xMin) return;
    if (rect.hy < yMin) return;

    xIdx = (rect.hx - xMin) / xStep;
    yIdx = (rect.hy - yMin) / yStep;
    if (xIdx > NN + 1)
        xIdx = NN + 1;
    if (yIdx > NN + 1)
        yIdx = NN + 1;

    xCellIndexH = int32_t(xIdx);
    yCellIndexH = int32_t(yIdx);

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
            mergeBCM_temp<cMergeCond_lx_t>(xCellIndexL, yl, yCellIndexH);   // B
            mergeBCM_temp<cMergeCond_hx_t>(xCellIndexH, yl, yCellIndexH);   // C

            mergeDEJ_temp<cMergeCond_ly_t>(xl, xCellIndexH, yCellIndexL);   // E
            mergeDEJ_temp<cMergeCond_hy_t>(xl, xCellIndexH, yCellIndexH);   // D
            mergeCorner_temp<cMergeCond_lx_hy_t>(xCellIndexL, yCellIndexH);  // F
            mergeCorner_temp<cMergeCond_hx_hy_t>(xCellIndexH, yCellIndexH);  // G
            mergeCorner_temp<cMergeCond_lx_ly_t>(xCellIndexL, yCellIndexL);  // I
            mergeCorner_temp<cMergeCond_hx_ly_t>(xCellIndexH, yCellIndexL);  // H
        }
        else { // xCellIndexL = xCellIndexH
            mergeBCM_temp<cMergeCond_lx_hx_t>(xCellIndexL, yCellIndexL + 1, yCellIndexH);    // M
            mergeCorner_temp<cMergeCond_hy_lx_hx_t>(xCellIndexL, yCellIndexH);   // N
            mergeCorner_temp<cMergeCond_ly_lx_hx_t>(xCellIndexL, yCellIndexL);   // O
        }
    }
    else {  // yCellIndexL = yCellIndexH
        if (xCellIndexL < xCellIndexH) {
            mergeDEJ_temp<cMergeCond_ly_hy_t>(xCellIndexL + 1, xCellIndexH, yCellIndexH);  // J
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
    return maxReqCount;
}

