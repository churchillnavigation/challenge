// -sAC262FDF-A6AE418F-AC262FDF-B67D0759-87EEC00F
/*
Algorithm description :

In the constructor, we sort the points based on Y coord.
We divide the points into SLICES number of groups and put them in individual vectors
Each of those vectors is sorted by rank

In the main search function, we check which of the "slices" the rect overlaps

We iterate through each of the points coordinates in the slices and we need to check if it is in the rect
For efficiency reasons, we store the X and Y alone in a vector rather than the whole point.
Since we have only co-ords, we can load 2 co-ordinates in a single SIMD instruction

This is good for 20 to 25% improvement 

Each pair of coordinates is duplicated into a 128 bit SIMD value
X1Y1X2Y2 -> X1Y1X1Y1
X1Y1X2Y2 -> X2Y2X2Y2

We put the rects coords into a 128 bit SIMD register, lets call it LLTTRRBB

Now we do a SIMD compare (X and Y are Xn and Yn for n = 1,2):
XYXY
< LTRB
====
ABCD

For the point to fall within the rect, A and B have to be positive and C and D have to be negative
_EXCEPT_ that sometimes the difference between co-ordinates is 0 : in these cases the 0 has to appear as negative for this
reasoning to work

The problem definition uses inclusive ranges : lx <= x <= hx and ly <= y <= hy
This causes the above condition of C or D being 0 sometimes.

To handle the situation of C and D being 0, we initially nudge the rectangle coordinates hx and hy upwards using the nextafter() function
This returns the pPtNext float value towards +INF, so we never get that condition of C or D being 0

Now we can call an SIMD instruction that takes the sign bits of the four floats PQRS and packs it into a int as a 4-bit value
If this integer has the binary value 1100 or 0xC, that means the point was in the rectangle

Note that 1 means negative, but we need 1100 and not 0011 because all our PQRS, ABCD etc are written with LSB to the left

The intrinsics based assembler works out to only 4 or 5 instructions in total, which is way more efficient than writing :
(lx <= x && x <= hx && ly <= y && y <= hy)

Once we do this its relatively straightforward, we accumulate the candidate points in a vector.
After accumulating count number of points per each tested slice, we sort the entire vector by rank.

Now we can return the pPtFirst N points in this vector


The inner loop is unrolled thrice, so 6 points are processed per iteration
We ensure that the number of points tested will always be a multiple of 6, by adding dummy points to each point grid 
The dummy points have co-ords that can never be part of  a rect (FLT_MAX, FLT_MAX)

Unrolling is worth 25% in tests

A tiny benefit can perhaps be got by shuffling around the order of independent instructions, but it's inconclusive

*/

#include "point_search.h"
#include <vector>
#include <cmath>
#include <cfloat>
#include <algorithm>
#include <utility>
#include <intrin.h>
#include <xmmintrin.h>
#include <emmintrin.h>
using namespace std;

uint64_t g_Total;
uint64_t g_Limit;

double g_MBRead;


FILE *g_LogFile;

#define to_i _mm_castps_si128
#define fm_i _mm_castsi128_ps

#define MAKE_COMPARATOR(C, F) struct C {inline bool operator()(const Point& p1, const Point& p2) const {return p1.F < p2.F;}};
MAKE_COMPARATOR(CmpX, x)
MAKE_COMPARATOR(CmpY, y)

#define ALL(X) X.begin(), X.end()
#define FOR_EACH(T, I, C) for(T I = C.begin(); I != C.end(); ++I)
#define FOR_N(I, N) for(size_t I = 0; I < N; ++I)
#define BETWEEN(X, MN, MX) (X >= MN && X < MX)

struct TMinMax
{
    float mn, mx;
    TMinMax():mn(FLT_MAX), mx(FLT_MIN) {}
    TMinMax(float mn, float mx):mn(mn), mx(mx){}

    inline void take(float a)
    {
        mn = std::min(a, mn);
        mx = std::max(a, mx);
    }
};


#pragma pack(push, 1)
struct XY
{
    float x, y;
    XY(){}
    XY(float x, float y):x(x), y(y){}
};


struct CmpXY 
{
    inline bool operator()(const XY& p1, const XY& p2) const 
    {
        if(p1.x < p2.x) return true;
        if(p1.x == p2.x) return p1.y < p2.y;
        return false;
    }
};

struct Chunk
{
    Point *beg;
    Point *end;
    Rect rc;

    Chunk(){}

    Chunk(Point *beg, Point *end): beg(beg), end(end)
    {
    }

    void setRect()
    {
        rc.lx = min_element(beg, end, CmpX())->x;
        rc.hx = max_element(beg, end, CmpX())->x;
        rc.ly = min_element(beg, end, CmpY())->y;
        rc.hy = max_element(beg, end, CmpY())->y;
    }

    int size()
    {
        return (int)(end - beg);
    }

    // Does this rect not overlap with the given one
    inline bool outRect(const Rect &rc2)
    {
        return
            rc.hx <= rc2.lx || rc2.hx <= rc.lx ||
            rc.hy <= rc2.ly || rc2.hy <= rc.ly;
    }


    // whether this chunk is completely inside rcOuter
    inline bool inRect(const Rect &rcOuter)
    {
        return 
            rc.lx >= rcOuter.lx && rc.hx < rcOuter.hx &&
            rc.ly >= rcOuter.ly && rc.hy < rcOuter.hy;
    }
};

#pragma pack(pop)

uint64_t gGood, gTotal, gBad;

double percent(uint64_t n, uint64_t total)
{
    double d = (double)n;
    d /= total;
    return d * 100;
}

typedef vector<Point> Points;
typedef Points::iterator PtIter;
typedef vector<XY> CoOrds;
typedef vector<Chunk> Chunks;

struct SearchContext
{
    static const int UNROLL = 6;
    static const int SPLITS = 4;
    Chunks m_Chunks;
    size_t m_nPts;
    Points m_vecPts;

    vector<CoOrds> m_vecXYGrid;
    vector<Point> m_vecPtsCandidate;

    template<typename PT>
    struct CmpPointRank
    {
        inline bool operator()(const PT& p1, const PT &p2) const
        {
            return p1.rank < p2.rank;
        }
    };

    // Given a range of points, splits them into 2 halfs so that 
    // there are equal number of points in each, Comparator CMP is used 
    template<typename CMP>
    void split(Point* pPtsBeg, Point* pPtsEnd,  vector<Chunk> &chunks)
    {
        // Split into two half
        sort(pPtsBeg, pPtsEnd, CMP());
        Point* pPtsMid = pPtsBeg + (pPtsEnd - pPtsBeg) / 2;
        chunks.push_back(Chunk(pPtsBeg, pPtsMid));
        chunks.push_back(Chunk(pPtsMid, pPtsEnd));
    }

    void splitAll()
    {
        Point *pBeg = &m_vecPts[0], *pEnd = pBeg + m_nPts;

        m_Chunks.push_back(Chunk(pBeg, pEnd));

        // Chop SPLITS times
        FOR_N(j, SPLITS)
        {
            Chunks chunks;

            FOR_EACH(Chunks::iterator, i, m_Chunks)
            {
                sort(i->beg, i->end, CmpY());
                Point* pPtsMid = i->beg + i->size() / 2;
                Point* pNext = pPtsMid + 1;
                while(pPtsMid->y == pNext->y || ( (pPtsMid - i->beg) % UNROLL))
                {
                    ++pPtsMid;
                    ++pNext;
                }

                chunks.push_back(Chunk(i->beg, pPtsMid));
                chunks.push_back(Chunk(pPtsMid, i->end));
            }

            m_Chunks = chunks;
        }

        // Split SPLITS times
        FOR_N(j, SPLITS)
        {
            Chunks chunks;

            FOR_EACH(Chunks::iterator, i, m_Chunks)
            {
                sort(i->beg, i->end, CmpX());
                Point* pPtsMid = i->beg + i->size() / 2;
                Point* pNext = pPtsMid + 1;
                while(pPtsMid->x == pNext->x || ( (pPtsMid - i->beg) % UNROLL))
                {
                    ++pPtsMid;
                    ++pNext;
                }

                chunks.push_back(Chunk(i->beg, pPtsMid));
                chunks.push_back(Chunk(pPtsMid, i->end));
            }

            m_Chunks = chunks;
        }
    }

    SearchContext(const Point* pPtsBeg, const Point* pPtsEnd): m_vecPts(pPtsBeg, pPtsEnd)
    {
        g_Total = 0;
        g_Limit = 0;
        g_MBRead = 0;
        m_nPts = pPtsEnd - pPtsBeg;

        if(m_nPts)
        {
            Point pt;
            pt.x = FLT_MIN;
            pt.y = FLT_MIN;
            pt.rank = INT_MAX;
            pt.id = 0;

            // Ensure points are multiple of UNROLL
            while(m_vecPts.size() % UNROLL)
            {
                // Dummy points guaranteed to never fall in any rectangle
                m_vecPts.push_back(pt);
            }

            if(m_nPts % UNROLL)
            {

            }

            splitAll();

            size_t nChunks = m_Chunks.size();
            m_vecXYGrid.resize(nChunks);
            FOR_N(i, nChunks)
            {
                Chunk &c = m_Chunks[i];
                c.setRect();
                sort(c.beg, c.end, CmpPointRank<Point>());

                m_vecXYGrid[i].reserve(c.size());
                // Store all the XYs in the XY vectors
                for(Point *p = c.beg; p != c.end; ++p)
                {
                    m_vecXYGrid[i].push_back(XY(p->x, p->y));
                }
            }

            m_vecPtsCandidate.resize(m_nPts);
        }
    }


    inline Point* searchOnRects(const Rect &rc, const int32_t nRequested, vector<size_t>& vecOnRectIdx, Point *pCandidateNext)
    {
        __m128 xmmXyxy1, xmmDiff1;
        __m128 xmmXyxy2, xmmDiff2;
        __m128 xmmData1, xmmData2, xmmData3;
        __m128 xmmRect = _mm_load_ps((float*)&rc);          // Load rect coords xmmRect <- HY LY HX LX where HY is in the MSDW

        FOR_N(i, vecOnRectIdx.size())
        {
            size_t iChunk = vecOnRectIdx[i];
            Chunk &c = m_Chunks[iChunk];
            CoOrds &vecXYGrid = m_vecXYGrid[iChunk];
            XY *pXYFirst = &vecXYGrid[0], *pXYNext = pXYFirst;
            size_t n = vecXYGrid.size();
            Point *pCandidateLast = pCandidateNext + nRequested;  
            int r1, r2;

            n = vecXYGrid.size() / UNROLL;
            
            while(n--)
            {
                // Load 4 co-ordinates in all
                xmmData1 = _mm_load_ps((float*)pXYNext);         // xmmTemp1 <- Y2 X2 Y1 X1 where Y2 is in the MSDW
                xmmData2 = _mm_load_ps((float*)(pXYNext + 2));         
                xmmData3 = _mm_load_ps((float*)(pXYNext + 4));         
                xmmXyxy1 = _mm_shuffle_ps(xmmData1, xmmData1, 0x44);
                xmmXyxy2 = _mm_shuffle_ps(xmmData1, xmmData1, 0xEE);
                xmmDiff1 = _mm_cmplt_ps(xmmXyxy1, xmmRect);     // Result has signs ++-- if point in rect
                xmmDiff2 = _mm_cmplt_ps(xmmXyxy2, xmmRect);     // Result has signs ++-- if point in rect
                r1 = _mm_movemask_ps(xmmDiff1);
                r2 = _mm_movemask_ps(xmmDiff2);
                if(r1 == 0xC)
                {
                    int64_t idx = pXYNext - pXYFirst;                                                                    
                    *pCandidateNext++ = c.beg[idx];
                    if(pCandidateNext == pCandidateLast) break;
                }
                if(r2 == 0xC)
                {
                    int64_t idx = pXYNext - pXYFirst;                                                                    
                    *pCandidateNext++ = c.beg[idx + 1];
                    if(pCandidateNext == pCandidateLast) break;
                }

                xmmXyxy1 = _mm_shuffle_ps(xmmData2, xmmData2, 0x44);
                xmmXyxy2 = _mm_shuffle_ps(xmmData2, xmmData2, 0xEE);
                xmmDiff1 = _mm_cmplt_ps(xmmXyxy1, xmmRect);     // Result has signs ++-- if point in rect
                xmmDiff2 = _mm_cmplt_ps(xmmXyxy2, xmmRect);     // Result has signs ++-- if point in rect
                r1 = _mm_movemask_ps(xmmDiff1);
                r2 = _mm_movemask_ps(xmmDiff2);
                if(r1 == 0xC)
                {
                    int64_t idx = pXYNext - pXYFirst;                                                                    
                    *pCandidateNext++ = c.beg[idx + 2];
                    if(pCandidateNext == pCandidateLast) break;
                }
                if(r2 == 0xC)
                {
                    int64_t idx = pXYNext - pXYFirst;                                                                    
                    *pCandidateNext++ = c.beg[idx + 3];
                    if(pCandidateNext == pCandidateLast) break;
                }

                xmmXyxy1 = _mm_shuffle_ps(xmmData3, xmmData3, 0x44);
                xmmXyxy2 = _mm_shuffle_ps(xmmData3, xmmData3, 0xEE);
                xmmDiff1 = _mm_cmplt_ps(xmmXyxy1, xmmRect);     // Result has signs ++-- if point in rect
                xmmDiff2 = _mm_cmplt_ps(xmmXyxy2, xmmRect);     // Result has signs ++-- if point in rect
                r1 = _mm_movemask_ps(xmmDiff1);
                r2 = _mm_movemask_ps(xmmDiff2);
                if(r1 == 0xC)
                {
                    int64_t idx = pXYNext - pXYFirst;                                                                    
                    *pCandidateNext++ = c.beg[idx + 4];
                    if(pCandidateNext == pCandidateLast) break;
                }
                if(r2 == 0xC)
                {
                    int64_t idx = pXYNext - pXYFirst;                                                                    
                    *pCandidateNext++ = c.beg[idx + 5];
                    if(pCandidateNext == pCandidateLast) break;
                }

                pXYNext += 6;
            }

            // How many grid coords read
            //g_Total += m_vecXYGrid.size();
            //double bytes = (m_vecXYGrid.size() * sizeof(XY));
//             double bytes = 
//             g_MBRead += bytes / 1048576;
        }
        return pCandidateNext;
    }



    int32_t searchIt(const Rect rec, const int32_t nRequested, Point* pPtsOut)
    {
        //Point *pptsO = pPtsOut;
        int nCount = 0;
        if(m_nPts)
        {
            // Bump up the rectangle right and bottom to deal with the fact that the problem
            // uses closed ranges when defining when a point is in a rect : left <= x <= right
            __declspec(align(16)) Rect rc;
            rc.lx = rec.lx;
            rc.ly = rec.ly;
            rc.hx = _nextafterf(rec.hx, FLT_MAX);
            rc.hy = _nextafterf(rec.hy, FLT_MAX);

            vector<size_t> vecInRectIdx;
            vector<size_t> vecOnRectIdx;

            size_t nChunks = m_Chunks.size();
            FOR_N(i, nChunks)
            {
                Chunk &c = m_Chunks[i];
                if(c.inRect(rc))
                {
                    vecInRectIdx.push_back(i);
                }
                else if(!c.outRect(rec))
                {
                    vecOnRectIdx.push_back(i);
                }
            }

            Point *pCandidateStart = &m_vecPtsCandidate[0], *pCandidateNext = pCandidateStart;

            // Those chunks that are completely inside the rect, just take upto nRequested points
            FOR_N(i, vecInRectIdx.size())
            {
                size_t iChunk = vecInRectIdx[i];
                Chunk &c = m_Chunks[iChunk];
                int nSize = c.size();
                int nToCopy = std::min(nSize, nRequested);
                copy(c.beg, c.beg + nToCopy, pCandidateNext);
                pCandidateNext += nToCopy;
            }

            pCandidateNext = searchOnRects(rc, nRequested, vecOnRectIdx, pCandidateNext);
            int nCandidates = (int)(pCandidateNext - pCandidateStart);
            nCount = min(nCandidates, nRequested);
            partial_sort(pCandidateStart, pCandidateStart + nCount, pCandidateNext, CmpPointRank<Point>());
            for(int i = 0; i < nCount; ++i)
            {
                *pPtsOut++ = *pCandidateStart++;
            }

//             fprintf(g_LogFile, "%f %f %f %f\n", rec.lx, rec.hx, rec.ly, rec.hy);
// 
//             for(int i = 0; i < nCount; ++i)
//             {
//                 fprintf(g_LogFile, "%f %f %d %d\n", pptsO->x, pptsO->y, pptsO->rank, (int)pptsO->id);
//                 ++pptsO;
//             }
        }

        return nCount;
    }
};

//////////////////////////////////////////////////////////////////////////

SearchContext* create(const Point* points_begin, const Point* points_end)
{
//    g_LogFile = fopen("output.txt", "w");
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
//     fclose(g_LogFile);
   // fprintf(stderr, "\nJumped %I64u\nTotal %I64u\n%f%%\n", g_Limit, g_Total, percent(g_Limit, g_Total));

    //fprintf(stderr, "\nCoords read %I64u\nBytes %I64u\n", g_Total, g_Total * (sizeof XY));
//    fprintf(stderr, "\n%f MB read\n", g_MBRead);

    delete sc;
    return NULL;
}
//////////////////////////////////////////////////////////////////////////
