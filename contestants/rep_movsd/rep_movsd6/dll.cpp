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
#include <intrin.h>
#include <xmmintrin.h>
#include <emmintrin.h>
using namespace std;

#define to_i _mm_castps_si128
#define fm_i _mm_castsi128_ps

#pragma pack(push, 1)
struct XY
{
    float x, y;
    XY(){}
    XY(float x, float y):x(x), y(y){}
};

#pragma pack(pop)

struct SearchContext
{
    static const int SLICES = 8;
    static const int UNROLL = 6;

    typedef vector<Point> Points;
    typedef Points::iterator PtIter;

    typedef vector<XY> CoOrds;

    size_t m_nPts;
    vector<Points> m_vecPtsGrid;
    vector<Rect> m_vecRCGrid;
    vector<CoOrds> m_vecXYGrid;

    
    float m_fMinX, m_fMinY, m_fMaxX, m_fMaxY;

    vector<Point> m_vecPtsCandidate;

    template<typename PT>
    struct CmpPointRank
    {
        inline bool operator()(const PT& p1, const PT &p2) const
        {
            return p1.rank < p2.rank;
        }
    };


    struct CmpPointX
    {
        inline bool operator()(const Point& p1, const Point & p2) const
        {
            return p1.x < p2.x;
        }
    };


    struct CmpPointY
    {
        inline bool operator()(const Point& p1, const Point & p2) const
        {
            return p1.y < p2.y;
        }
    };


    SearchContext(const Point* pPtsBeg, const Point* pPtsEnd)
    {
        m_nPts = pPtsEnd - pPtsBeg;

        if(m_nPts)
        {
            m_fMinX = min_element(pPtsBeg, pPtsEnd, CmpPointX())->x;
            m_fMaxX = max_element(pPtsBeg, pPtsEnd, CmpPointX())->x;
            m_fMinY = min_element(pPtsBeg, pPtsEnd, CmpPointY())->y;
            m_fMaxY = max_element(pPtsBeg, pPtsEnd, CmpPointY())->y;

            // How many points per rect (ensure multiple of 2)
            size_t nMax = m_nPts / SLICES;
            m_vecPtsGrid.resize(SLICES);
            m_vecXYGrid.resize(SLICES);

            {
                // Get points sorted by Y
                vector<Point> pts(pPtsBeg, pPtsEnd);
                sort(pts.begin(), pts.end(), CmpPointY());

                // Put each point into its own horizontal stripe
                int n = 0;
                Rect rc;
                rc.lx = FLT_MIN;
                rc.hx = FLT_MAX;
                rc.ly = m_fMinY;

                for(PtIter p = pts.begin(); p != pts.end(); ++p)
                {
                    const Point &pt = *p;

                    // If current grid is full
                    if(m_vecPtsGrid[n].size() > nMax)
                    {
                        rc.hy = p->y;               // prev grids rectangle bottom (open range] is this points Y
                        m_vecRCGrid.push_back(rc);  // Save this rect
                        ++n;                        // Goto pPtNext grid
                        rc.ly = p->y;               // New grids Y start is prev grids Y end
                    }

                    m_vecPtsGrid[n].push_back(pt);
                }

                // Save final rect
                rc.hy = m_fMaxY;
                m_vecRCGrid.push_back(rc);
            }

            // Sort each grid by Y, X then rank
            size_t nGrids = m_vecPtsGrid.size();
            for(int i = 0; i < nGrids; ++i)
            {
                sort(m_vecPtsGrid[i].begin(), m_vecPtsGrid[i].end(), CmpPointRank<Point>());

                // Ensure grid has an even number of points always, we process 2 at a time
                while(m_vecPtsGrid[i].size() % UNROLL)
                {
                    // Dummy points guaranteed to never fall in any rectangle
                    Point pt;
                    pt.x = FLT_MIN;
                    pt.y = FLT_MIN;
                    pt.rank = INT_MAX;
                    pt.id = 0;
                    m_vecPtsGrid[i].push_back(pt);
                }

                for(PtIter p = m_vecPtsGrid[i].begin(); p != m_vecPtsGrid[i].end(); ++p)
                {
                    XY xy(p->x, p->y);
                    m_vecXYGrid[i].push_back(xy);
                }
            }
            m_vecPtsCandidate.resize(m_nPts);
        }
    }

#define BETWEEN(X, MN, MX) (X >= MN && X <= MX)

    int32_t searchIt(const Rect rec, const int32_t nRequested, Point* pPtsOut)
    {
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

            // First detect which grids this rect spans
            size_t nGridMin = 0, nGridMax = m_vecPtsGrid.size();
            for(int i = 0; i < m_vecRCGrid.size(); ++i)
            {
                Rect &rcG = m_vecRCGrid[i];
                if(BETWEEN(rec.ly, rcG.ly, rcG.hy))
                {
                    nGridMin = i;
                    break;
                }
            }

            for(int i = 0; i < m_vecRCGrid.size(); ++i)
            {
                Rect &rcG = m_vecRCGrid[i];
                if(BETWEEN(rec.hy, rcG.ly, rcG.hy))
                {
                    nGridMax = i;
                    break;
                }
            }
            ++nGridMax;

            __m128 xmmXyxy1, xmmDiff1, xmmTemp11, xmmData, xmmMask;
            __m128 xmmXyxy2, xmmDiff2;

            Point *pCandidateStart = &m_vecPtsCandidate[0], *pCandidateNext = pCandidateStart;
            
            // Load rect coords
            __m128 xmmRect = _mm_load_ps((float*)&rc);          // xmmRect <- HY LY HX LX where HY is in the MSDW

            xmmMask = fm_i(_mm_set_epi32(0xFFFFFFFF, 0xFFFFFFFF, 0, 0));

            for(size_t i = nGridMin; i < nGridMax; ++i)
            {
                Points &vecPtsGrid = m_vecPtsGrid[i];
                CoOrds &vecXYGrid = m_vecXYGrid[i];
                XY *pXYFirst = &vecXYGrid[0], *pXYNext = pXYFirst;

                xmmData = _mm_load_ps((float*)pXYNext);         // xmmTemp1 <- Y2 X2 Y1 X1 where Y2 is in the MSDW

                // n is always 0 % UNROLL 
                size_t n = vecXYGrid.size() / UNROLL;
                Point *pCandidateLast = pCandidateNext + nRequested;  
                                
                // Unrolled to test 6 points
                while(n)
                {
                    --n;

                    xmmTemp11 = xmmData;                         // xmmTemp1 <- Y2 X2 Y1 X1 where Y2 is in the MSDW
                    xmmData = _mm_load_ps((float*)(pXYNext+2));  // Load the next two points - whilst it loads, other stuff can happen in parallel
                    
                    // xmmData <- Y2 X2 Y1 X1 where Y2 is in the MSDW
                    // Get xmmXyxy1 to have Y1 X1 Y1 X1 by shuffling using the control mask 1, 0, 1, 0 = 01 00 01 00
                    // Get xmmXyxy2 to have Y2 X2 Y2 X2 by shuffling using the control mask 3, 2, 3, 2 = 11 10 11 10
                    xmmXyxy1 = _mm_shuffle_ps(xmmTemp11, xmmTemp11, 0x44);
                    xmmXyxy2 = _mm_shuffle_ps(xmmTemp11, xmmTemp11, 0xEE);
                    xmmDiff1 = _mm_cmplt_ps(xmmXyxy1, xmmRect);     // Result has signs ++-- if point in rect
                    xmmDiff2 = _mm_cmplt_ps(xmmXyxy2, xmmRect);     // Result has signs ++-- if point in rect

                    int r1 = _mm_movemask_ps(xmmDiff1);
                    if(r1 == 0xC)
                    {
                        int64_t idx = pXYNext - pXYFirst;
                        *pCandidateNext++ = vecPtsGrid[idx];
                        if(pCandidateNext == pCandidateLast) break;
                    }
                    pXYNext += 2;
        
                    int r2 = _mm_movemask_ps(xmmDiff2);
                    if(r2 == 0xC)
                    {
                        int64_t idx = pXYNext - pXYFirst;
                        *pCandidateNext++ = vecPtsGrid[idx -1];
                        if(pCandidateNext == pCandidateLast) break;
                    }

                    xmmTemp11 = xmmData;                         
                    xmmData = _mm_load_ps((float*)(pXYNext+2));     
                    xmmXyxy1 = _mm_shuffle_ps(xmmTemp11, xmmTemp11, 0x44);
                    xmmXyxy2 = _mm_shuffle_ps(xmmTemp11, xmmTemp11, 0xEE);
                    xmmDiff1 = _mm_cmplt_ps(xmmXyxy1, xmmRect);     // Result has signs ++-- if point in rect
                    xmmDiff2 = _mm_cmplt_ps(xmmXyxy2, xmmRect);     // Result has signs ++-- if point in rect

                    r1 = _mm_movemask_ps(xmmDiff1);
                    if(r1 == 0xC)
                    {
                        int64_t idx = pXYNext - pXYFirst;
                        *pCandidateNext++ = vecPtsGrid[idx];
                        if(pCandidateNext == pCandidateLast) break;
                    }
                    pXYNext += 2;

                    r2 = _mm_movemask_ps(xmmDiff2);
                    if(r2 == 0xC)
                    {
                        int64_t idx = pXYNext - pXYFirst;
                        *pCandidateNext++ = vecPtsGrid[idx -1];
                        if(pCandidateNext == pCandidateLast) break;
                    }

                    xmmTemp11 = xmmData;                         
                    xmmData = _mm_load_ps((float*)(pXYNext+2));     
                    xmmXyxy1 = _mm_shuffle_ps(xmmTemp11, xmmTemp11, 0x44);
                    xmmXyxy2 = _mm_shuffle_ps(xmmTemp11, xmmTemp11, 0xEE);
                    xmmDiff1 = _mm_cmplt_ps(xmmXyxy1, xmmRect);     // Result has signs ++-- if point in rect
                    xmmDiff2 = _mm_cmplt_ps(xmmXyxy2, xmmRect);     // Result has signs ++-- if point in rect

                    r1 = _mm_movemask_ps(xmmDiff1);
                    if(r1 == 0xC)
                    {
                        int64_t idx = pXYNext - pXYFirst;
                        *pCandidateNext++ = vecPtsGrid[idx];
                        if(pCandidateNext == pCandidateLast) break;
                    }
                    pXYNext += 2;

                    r2 = _mm_movemask_ps(xmmDiff2);
                    if(r2 == 0xC)
                    {
                        int64_t idx = pXYNext - pXYFirst;
                        *pCandidateNext++ = vecPtsGrid[idx -1];
                        if(pCandidateNext == pCandidateLast) break;
                    }
                }
            }

            int nCandidates = pCandidateNext - pCandidateStart;
            nCount = min(nCandidates, nRequested);

            partial_sort(pCandidateStart, pCandidateStart + nCount, pCandidateNext, CmpPointRank<Point>());
            for(int i = 0; i < nCount; ++i)
            {
                *pPtsOut++ = *pCandidateStart++;
            }
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
