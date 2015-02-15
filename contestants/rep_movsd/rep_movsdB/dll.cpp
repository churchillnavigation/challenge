#include "point_search.h"

#define _SCL_SECURE_NO_WARNINGS 1

#ifndef _DEBUG
#define _SECURE_SCL 0
#endif

#pragma warning(disable:4530)

#include <vector>
#include <cmath>
#include <cfloat>
#include <cassert>
#include <algorithm>
#include <intrin.h>
#include <xmmintrin.h>
#include <emmintrin.h>
using namespace std;

#define ALL(X) X.begin(), X.end()
#define FOR_EACH(T, I, C) for(T I = C.begin(); I != C.end(); ++I)
#define FOR_N(I, N) for(size_t I = 0; I < N; ++I)
#define fm_i _mm_castsi128_ps
#define BETWEEN(X, MN, MX) (X >= MN && X < MX)

#pragma pack(push, 1)
struct XY
{
    float x, y;
    XY() {}
    XY(float x, float y):x(x), y(y) {}
};
//////////////////////////////////////////////////////////////////////////
#pragma pack(pop)


__declspec(align(16)) struct PtAligned
{
    int32_t rank;
    int32_t id;
    float x, y;
    PtAligned() {}

    // Dummy point maker
    PtAligned(int):rank(INT_MAX) {}

    PtAligned(const Point &p): rank(p.rank), x(p.x), y(p.y), id(p.id) {}

    // Returns false without writing if dummy point
    bool writeToPointPacked(Point *p)
    {
        if(rank == INT_MAX) return false;
        p->id = (uint8_t)id;
        p->rank = rank;
        p->x = x;
        p->y = y;

        return true;
    }


};
//////////////////////////////////////////////////////////////////////////

#define MAKE_COMPARATOR(C, F) struct C {inline bool operator()(const PtAligned& p1, const PtAligned& p2) const {return p1.F < p2.F;}};
MAKE_COMPARATOR(CmpX, x)
MAKE_COMPARATOR(CmpY, y)
MAKE_COMPARATOR(CmpPointRank, rank)

typedef vector<PtAligned> Points;
typedef Points::iterator PtIter;
typedef vector<XY> CoOrds;

// This struct accumulates a list of the best N ranked points
// it maintains the list of current results sorted by rank
// When a point is added, it returns false if that point doesn't make it to the list 
// Since we always process points in ascending rank order per chunk, once this returns false, we can ignore all
// the other points n the chunk since they are guaranteed to be worse ranked than the results
__declspec(align(16)) class Results
{
    PtAligned front[24];

public:

    PtAligned *back;
    int32_t rank;

    Results(int nSize)
    {
        FOR_N(i, nSize + 1)
        {
            front[i] = PtAligned(0);
        }
        back = front + nSize;
        --back;
        rank = back->rank;
    }
    //////////////////////////////////////////////////////////////////////////

    // Add a point to results, return false if it did not make the cut
    // results remain sorted
    inline bool addPt(const PtAligned &pt)
    {
        if(rank > pt.rank)
        {
            PtAligned *ptWhere = lower_bound(front, back, pt, CmpPointRank());
            for(PtAligned *p = back; p != ptWhere; --p)
            {
                *p = *(p - 1);
            }
            *ptWhere = pt;

            rank = back->rank;

            return true;
        }
        return false;
    }
    //////////////////////////////////////////////////////////////////////////

    // Get the results out, we need to re-sort here in the rare case that pts.size() < nSize at the end of processing
    int getPts(Point *ptOut)
    {
        int n = 0;
        FOR_N(i, 21)
        {
            if(front[i].writeToPointPacked(ptOut))
            {
                ++ptOut;
                ++n;
            }
            else
            {
                break;
            }
        }
        return n;
    }
    //////////////////////////////////////////////////////////////////////////
};


//////////////////////////////////////////////////////////////////////////

// Chunk represents a rectangle full of points
// Points are sorted by rank
// Every chunk has the same number of points
// The bounding rectangle closed range ()
// Input rectangles are closed ranges too 

enum SortOrder
{
    SortX, SortY, SortRank, SortNone
};

__declspec(align(16)) struct Chunk
{
    XY bounds[8];
    PtAligned *beg;
    PtAligned *end;
    Rect rc, rcInc;
    int32_t rank;
    SortOrder order;
    float width, hight;
    int32_t size;


    Chunk() {}

    Chunk(PtAligned *beg, PtAligned *end, SortOrder order = SortNone): beg(beg), end(end), order(order)
    {
        updateRect();
        size = (int32_t)(end - beg);
    }
    //////////////////////////////////////////////////////////////////////////

    // Get the bounding rect, and the best rank
    void updateRect()
    {
        rc.lx = min_element(beg, end, CmpX())->x;
        rc.hx = max_element(beg, end, CmpX())->x;
        rc.ly = min_element(beg, end, CmpY())->y;
        rc.hy = max_element(beg, end, CmpY())->y;

        // Inclusive rectangle useful to test if this rect is completely inside an exclusive rect using SSE
        rcInc = rc;

        rc.hx = _nextafterf(rc.hx, FLT_MAX);
        rc.hy = _nextafterf(rc.hy, FLT_MAX);
        width = rc.hx - rc.lx;
        hight = rc.hy - rc.ly;

    }
    //////////////////////////////////////////////////////////////////////////

    void updateRank()
    {
        sort(beg, end, CmpPointRank());
        rank = beg->rank;
    }

    inline bool onRectB(const float *hhll) const
    {
        __m128 xmmRect = _mm_load_ps((float*)hhll);         // Load rect co-ords xmmRect <- LY LX HY HX where LY is in the MSDW
        __m128 xmmData1 = _mm_load_ps((float*)&rc);         // xmmTemp1 <- HY HX LY LX where Y2 is in the MSDW
        __m128 xmmDiff1 = _mm_cmple_ps(xmmRect, xmmData1);          
        int r1 = _mm_movemask_ps(xmmDiff1);
        // Result has signs ++-- if 
        // !(rcOther.hx <= rc.lx) &&
        // !(rcOther.hy <= rc.ly) &&
        // (rcOther.lx <= rc.hx) &&
        // (rcOther.ly <= rc.hy);

        return r1 == 0xC;
    }
    //////////////////////////////////////////////////////////////////////////

    inline bool inRectB(const Rect &rcOther) const
    {
        //if this chunks rect inclusive ltrb is compared with the outer exclusive LTRB with >=
        // we should get true, true, false false
        __m128 xmmRect = _mm_load_ps((const float*)&rcOther);
        __m128 xmmData1 = _mm_load_ps((float*)&rcInc);
        __m128 xmmDiff1 = _mm_cmpge_ps(xmmData1, xmmRect);     // Result has signs --++ if true
        int r1 = _mm_movemask_ps(xmmDiff1);
        return r1 == 0x3;
    }
};
//////////////////////////////////////////////////////////////////////////

struct CmpChunkRank
{
    inline bool operator()(const Chunk& c1, const Chunk &c2) const
    {
        return c1.beg->rank < c2.beg->rank;
    }
};
//////////////////////////////////////////////////////////////////////////

struct SearchContext
{
    typedef vector<Chunk> Chunks;

    static const int UNROLL = 6;
    static const int SPLITS = 5;

    Chunks m_Chunks;
    size_t m_nPts;
    Points m_vecPts;
    vector<CoOrds> m_vecXYGrid;
    float m_fWidth, m_fHight;

    // Split chunk c into left and right halves, put results in chunks
    void splitEqualPointsOnX(Chunks &chunks, Chunk &c)
    {
        // first sort by X if necessary
        if(c.order != SortX)
        {
            sort(c.beg, c.end, CmpX());
        }

        // There have to be at least two points with two distinct X values otherwise we cannot split
        // The case is pathological, but we need to handle it 
        bool bHasDistinctX = false;
        if(c.size)
        {
            float x1 = c.beg->x;
            float x2 = ((c.end) - 1)->x;
            bHasDistinctX = x2 > x1;
        }
        
        if(bHasDistinctX)
        {
            PtAligned* pPtsMid = c.beg + c.size / 2;
            PtAligned* pNext = pPtsMid + 1;
            while(pPtsMid->x == pNext->x)
            {
                ++pPtsMid;
                ++pNext;
            }

            if(pPtsMid == c.end || pPtsMid == c.beg)
            {
                goto nosplitX;
            };

            chunks.push_back(Chunk(c.beg, pPtsMid, SortX));
            chunks.push_back(Chunk(pPtsMid, c.end, SortX));
        }
        else
        {
            nosplitX:
            chunks.push_back(c);
        }
    }
    //////////////////////////////////////////////////////////////////////////

    void splitEqualPointsOnY(Chunks &chunks, Chunk &c)
    {
        if(c.order != SortY)
        {
            sort(c.beg, c.end, CmpY());
        }

        // There have to be at least two points with two distinct Y values otherwise we cannot split
        // The case is pathological, but we need to handle it
        bool bHasDistinctY = false;
        if(c.size)
        {
            float y1 = c.beg->y;
            float y2 = ((c.end) - 1)->y;
            bHasDistinctY = y2 > y1;
        }

        if(bHasDistinctY)
        {
            PtAligned* pPtsMid = c.beg + c.size / 2;
            PtAligned* pNext = pPtsMid + 1;
            while(pPtsMid->y == pNext->y)
            {
                ++pPtsMid;
                ++pNext;
            }

            if(pPtsMid == c.end || pPtsMid == c.beg)
            {
                goto nosplitY;
            };

            chunks.push_back(Chunk(c.beg, pPtsMid, SortY));
            chunks.push_back(Chunk(pPtsMid, c.end, SortY));
        }
        else
        {
            nosplitY:
            chunks.push_back(c);
        }
    }
    //////////////////////////////////////////////////////////////////////////

    void splitAll()
    {
        PtAligned *pBeg = &m_vecPts[0], *pEnd = pBeg + m_nPts;
        m_Chunks.push_back(Chunk(pBeg, pEnd));
        m_fWidth = m_Chunks[0].width;
        m_fHight = m_Chunks[0].hight;

        FOR_N(j, SPLITS)
        {
            Chunks chunks;
            FOR_EACH(Chunks::iterator, i, m_Chunks)
            {
                if(i->size > 64)
                {
                    splitEqualPointsOnX(chunks, *i);
                }
            }
            m_Chunks = chunks;
        }

        FOR_N(j, SPLITS)
        {
            Chunks chunks;
            FOR_EACH(Chunks::iterator, i, m_Chunks)
            {
                if(i->size > 64)
                {
                    splitEqualPointsOnY(chunks, *i);
                }
            }
            m_Chunks = chunks;
        }
    }
    //////////////////////////////////////////////////////////////////////////

    SearchContext(const Point* pPtsBeg, const Point* pPtsEnd)
    {
        m_nPts = pPtsEnd - pPtsBeg;
        if(m_nPts)
        {
            // Convert all the points to our aligned struct
            m_vecPts.reserve(m_nPts);
            FOR_N(i, m_nPts)
            {
                m_vecPts.push_back(PtAligned(pPtsBeg[i]));
            }

            splitAll();

            // Update every chunks bounds and best rank
            size_t nChunks = m_Chunks.size();
            FOR_N(i, nChunks)
            {
                m_Chunks[i].updateRect();
                m_Chunks[i].updateRank();
            }

            // Sort the chunks vector itself ascending by rank
            sort(ALL(m_Chunks), CmpChunkRank());

            // Store all the XYs in the XY vectors
            m_vecXYGrid.resize(nChunks);
            FOR_N(i, nChunks)
            {
                Chunk &c = m_Chunks[i];
                CoOrds &vecXYGrid = m_vecXYGrid[i];
                vecXYGrid.reserve(c.size);
                for(PtAligned *p = c.beg; p != c.end; ++p)
                {
                    vecXYGrid.push_back(XY(p->x, p->y));
                }
            }
        }
    }
    //////////////////////////////////////////////////////////////////////////

    void searchOnRect(const Chunk &c, const Rect &rc, CoOrds &vecXYGrid, Results &results)
    {
        __m128 xmmXyxy1, xmmDiff1;
        __m128 xmmXyxy2, xmmDiff2;
        __m128 xmmData1, xmmData2, xmmData3;
        __m128 xmmRect = _mm_load_ps((float*)&rc);          // Load rect coords xmmRect <- HY LY HX LX where HY is in the MSDW

        XY *pXYFirst = &vecXYGrid[0];
        xmmData1 = _mm_load_ps((float*)(pXYFirst));         // xmmTemp1 <- Y2 X2 Y1 X1 where Y2 is in the MSDW


        int r1, r2;
        int64_t idx = 0;
        int nMax = (c.size / UNROLL) * UNROLL;
        int nOdd = c.size % UNROLL;
        while(idx < nMax)
        {
            // Load 6 co-ordinates in all
            xmmData2 = _mm_load_ps((float*)(pXYFirst + idx + 2));

            xmmXyxy1 = _mm_shuffle_ps(xmmData1, xmmData1, 0x44);
            xmmXyxy2 = _mm_shuffle_ps(xmmData1, xmmData1, 0xEE);
            xmmDiff1 = _mm_cmplt_ps(xmmXyxy1, xmmRect);     // Result has signs ++-- if point in rect
            xmmDiff2 = _mm_cmplt_ps(xmmXyxy2, xmmRect);     // Result has signs ++-- if point in rect
            r1 = _mm_movemask_ps(xmmDiff1);
            r2 = _mm_movemask_ps(xmmDiff2);

            xmmXyxy1 = _mm_shuffle_ps(xmmData2, xmmData2, 0x44);
            xmmXyxy2 = _mm_shuffle_ps(xmmData2, xmmData2, 0xEE);
            xmmDiff1 = _mm_cmplt_ps(xmmXyxy1, xmmRect);     // Result has signs ++-- if point in rect
            xmmDiff2 = _mm_cmplt_ps(xmmXyxy2, xmmRect);     // Result has signs ++-- if point in rect

            xmmData3 = _mm_load_ps((float*)(pXYFirst + idx + 4));

            if(r1 == 0xC)
            {
                if(!results.addPt(c.beg[idx])) return;
            }

            if(r2 == 0xC)
            {
                if(!results.addPt(c.beg[idx + 1])) return;
            }

            r1 = _mm_movemask_ps(xmmDiff1);
            r2 = _mm_movemask_ps(xmmDiff2);
            if(r1 == 0xC)
            {
                if(!results.addPt(c.beg[idx + 2])) return;
            }
            if(r2 == 0xC)
            {
                if(!results.addPt(c.beg[idx + 3])) return;
            }

            xmmData1 = _mm_load_ps((float*)(pXYFirst + idx + 6));         // xmmTemp1 <- Y2 X2 Y1 X1 where Y2 is in the MSDW

            xmmXyxy1 = _mm_shuffle_ps(xmmData3, xmmData3, 0x44);
            xmmXyxy2 = _mm_shuffle_ps(xmmData3, xmmData3, 0xEE);
            xmmDiff1 = _mm_cmplt_ps(xmmXyxy1, xmmRect);     // Result has signs ++-- if point in rect
            xmmDiff2 = _mm_cmplt_ps(xmmXyxy2, xmmRect);     // Result has signs ++-- if point in rect
            r1 = _mm_movemask_ps(xmmDiff1);
            r2 = _mm_movemask_ps(xmmDiff2);
            if(r1 == 0xC)
            {
                if(!results.addPt(c.beg[idx + 4])) return;
            }
            if(r2 == 0xC)
            {
                if(!results.addPt(c.beg[idx + 5])) return;
            }
            idx += 6;


        }

        // Process any odd points
        while(nOdd--)
        {
            xmmData1 = _mm_loadu_ps((float*)(pXYFirst + idx));         // xmmTemp1 <- Y2 X2 Y1 X1 where Y2 is in the MSDW
            xmmXyxy1 = _mm_shuffle_ps(xmmData1, xmmData1, 0x44);
            xmmDiff1 = _mm_cmplt_ps(xmmXyxy1, xmmRect);     // Result has signs ++-- if point in rect
            r1 = _mm_movemask_ps(xmmDiff1);
            if(r1 == 0xC)
            {
                if(!results.addPt(c.beg[idx])) return;
            }
            ++idx;
        }
    }
    //////////////////////////////////////////////////////////////////////////

    int32_t searchIt(const Rect rec, const int32_t nRequested, Point* pPtsOut)
    {
        int nCount = 0;
        if(m_nPts)
        {
            // Bump up the rectangle right and bottom to deal with the fact that the problem
            // uses closed ranges when defining when a point is in a rect : left <= x <= right
            // We need lx <= x < hx
            __declspec(align(16)) Rect rc;
            rc.lx = rec.lx;
            rc.ly = rec.ly;
            rc.hx = _nextafterf(rec.hx, FLT_MAX);
            rc.hy = _nextafterf(rec.hy, FLT_MAX);

            // Points of the rect arranged as HX HY LX LY used to calculate rect overlap
            __declspec(align(16)) float hhll[4] = {rc.hx, rc.hy, rc.lx, rc.ly};

            Results results(nRequested);
            size_t nChunks = m_Chunks.size();
            FOR_N(i, nChunks)
            {
                const Chunk &c = m_Chunks[i];

                // If this chunks best point doesn't make the cut, ignore the whole chunk
                if(c.rank <= results.rank)
                {
                    if(c.onRectB(hhll))
                    {
                        if(c.inRectB(rc))
                        {
                            // Keep adding points until rank is worse than the worst
                            int nSize = c.size;
                            int nToCopy = std::min(nSize, nRequested);
                            for(int k = 0; k < nToCopy; ++k)
                            {
                                if(!results.addPt(c.beg[k])) break;
                            }
                        }
                        else
                        {
                            searchOnRect(c, rc, m_vecXYGrid[i], results);
                        }
                    }
                }
            }

            nCount = results.getPts(pPtsOut);
        }

        return nCount;
    }
};

//////////////////////////////////////////////////////////////////////////

SearchContext* create(const Point* points_begin, const Point* points_end)
{
    return new SearchContext(points_begin, points_end);
}
//////////////////////////////////////////////////////////////////////////

int32_t search(SearchContext* sc, const Rect rect, const int32_t count, Point* out_points)
{
    return sc->searchIt(rect, count, out_points);
}
//////////////////////////////////////////////////////////////////////////

SearchContext* destro(SearchContext* sc)
{
    delete sc;
    return NULL;
}
//////////////////////////////////////////////////////////////////////////
