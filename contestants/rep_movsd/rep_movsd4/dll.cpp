/*
Algorithm description :

In the constructor, we sort the points based on Y coord.
We divide the points into SLICES number of groups and put them in individual vectors
Each of those vectors is sorted by rank

In the main search function, we check which of the "slices" the rect overlaps

We iterate through each of the points in the slices and we need to check if it is in the rect

We put the rects coords into an XYXY, lets call it LTRB

Now we do a SIMD subtract :
   XYXY
 - LTRB
   ====
   ABCD

For the point to fall within the rect, A and B have to be positive and C and D have to be negative
_EXCEPT_ that sometimes the difference between co-ordinates is 0 : in these cases the 0 has to appear as negative for this
reasoning to work 

The problem definition uses inclusive ranges : lx <= x <= hx and ly <= y <= hy 
This causes the above condition of C or D being 0 sometimes.

To handle the situation of C and D being 0, we initially nudge the rectangle coordinates hx and hy upwards using the nextafter() function
This returns the next float value towards +INF, so we never get that condition of C or D being 0

Now we can call an SIMD instruction that takes the sign bits of the four floats PQRS and packs it into a int as a 4-bit value
If this integer has the binary value 1100 or 0xC, that means the point was in the rectangle 

Note that 1 means negative, but we need 1100 and not 0011 because all our PQRS, ABCD etc are written with LSB to the left

The intrinsics based assembler works out to only 4 or 5 instructions in total, which is way more efficient than writing :
(lx <= x && x <= hx && ly <= y && y <= hy)

Once we do this its relatively straightforward, we accumulate the candidate points in a vector.
After accumulating count number of points per each tested slice, we sort the entire vector by rank.

Now we can return the first N points in this vector
*/

#include "point_search.h"
#include <vector>
#include <cmath>
#include <cfloat>
#include <algorithm>
#include <xmmintrin.h>
#include <emmintrin.h>
using namespace std;

#define to_i _mm_castps_si128
#define fm_i _mm_castsi128_ps

#define _mm_rshift(A, B) fm_i(_mm_srli_si128(to_i(A), B))
#define _mm_lshift(A, B) fm_i(_mm_slli_si128(to_i(A), B))

#pragma pack(push, 1)
struct XY
{
    float x, y;
};

#pragma pack(pop)

// Unpacked structure
struct MyPoint
{
    float x, y;
    int32_t rank;
    int32_t id;

    void writeToPoint(Point *pt)
    {
        pt->x = x;
        pt->y = y;
        pt->rank = rank;
        pt->id = (int8_t)id;
    }

    MyPoint(){}

    MyPoint(const Point *pt): x(pt->x), y(pt->y), rank(pt->rank), id(pt->id) {}

    inline bool isInRect(const Rect &rc)
    {
        return x >= rc.lx && x < rc.hx && y >= rc.ly && y < rc.hy;
    }

};

struct SearchContext
{
    typedef vector<MyPoint> MyPoints;
    typedef MyPoints::iterator MyPtIter;
    typedef vector<Point>::iterator PtIter;

    size_t m_size;
    vector<MyPoints> m_Grids;
    vector<Rect> m_GridRects;
    static const int SLICES = 12;
    float fMinX, fMinY, fMaxX, fMaxY;

    vector<MyPoint> m_vecCandidatePts;

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
        m_size = pPtsEnd - pPtsBeg;

        if(m_size)
        {
            fMinX = min_element(pPtsBeg, pPtsEnd, CmpPointX())->x;
            fMaxX = max_element(pPtsBeg, pPtsEnd, CmpPointX())->x;
            fMinY = min_element(pPtsBeg, pPtsEnd, CmpPointY())->y;
            fMaxY = max_element(pPtsBeg, pPtsEnd, CmpPointY())->y;
            
            // How many points per rect
            size_t nMax = m_size / SLICES;
            m_Grids.resize(SLICES);
            
            {
                // Get points sorted by Y
                vector<Point> pts(pPtsBeg, pPtsEnd);
                sort(pts.begin(), pts.end(), CmpPointY());

                // Put each point into its own horizontal stripe
                int n = 0;
                Rect rc;
                rc.lx = FLT_MIN;
                rc.hx = FLT_MAX;
                rc.ly = fMinY;

                for(PtIter p = pts.begin(); p != pts.end(); ++p)
                {
                    MyPoint pt(&*p);

                    // If current grid is full
                    if(m_Grids[n].size() > nMax)
                    {
                        rc.hy = p->y;               // prev grids rectangle bottom (open range] is this points Y
                        m_GridRects.push_back(rc);  // Save this rect
                        ++n;                        // Goto next grid
                        rc.ly = p->y;               // New grids Y start is prev grids Y end
                    }

                    m_Grids[n].push_back(pt);
                }

                // Save final rect 
                rc.hy = fMaxY;
                m_GridRects.push_back(rc);

            }
            
            // Sort each grid by rank
            size_t nGrids = m_Grids.size();
            for(int i = 0; i < nGrids; ++i)
            {
                sort(m_Grids[i].begin(), m_Grids[i].end(), CmpPointRank<MyPoint>());
            }

            m_vecCandidatePts.resize(m_size);
        }
    }

#define BETWEEN(X, MN, MX) (X >= MN && X <= MX)

    int32_t searchIt(const Rect rec, const int32_t cou, Point* pPtsOut)
    {
        int count = 0;

        if(m_size)
        {

            // Bump up the rectangle right and bottom to deal with the fact that the problem 
            // uses closed ranges when defining when a point is in a rect : left <= x <= right
            __declspec(align(16)) Rect rc;
            rc.lx = rec.lx;
            rc.ly = rec.ly;
            rc.hx = _nextafterf(rec.hx, FLT_MAX);
            rc.hy = _nextafterf(rec.hy, FLT_MAX);


            // First detect which grids this rect spans
            //size_t gridIDX[SLICES];
            size_t nPts = 0;
            size_t nGridMin = 0, nGridMax = m_Grids.size();
            for(int i = 0; i < m_GridRects.size(); ++i)
            {
                Rect &rcG = m_GridRects[i];
                if(BETWEEN(rec.ly, rcG.ly, rcG.hy))
                {
                    nGridMin = i;
                    break;
                }
            }

            for(int i = 0; i < m_GridRects.size(); ++i)
            {
                Rect &rcG = m_GridRects[i];
                if(BETWEEN(rec.hy, rcG.ly, rcG.hy))
                {
                    nGridMax = i;
                    break;
                }
            }

            ++nGridMax;

            nPts = 0;
            for(size_t i = nGridMin; i < nGridMax; ++i)
            {
                nPts += m_Grids[i].size();
            }

            __m128 xyxy1, diff1, temp1, temp2, data;

            MyPoint *pCandidateStart = &m_vecCandidatePts[0], *pCandidateNext = pCandidateStart;

            for(size_t i = nGridMin; i < nGridMax; ++i)
            {
                // Load rect coords 
                __m128 ltrb = _mm_load_ps((float*)&rc);     // ltrb <- HY LY HX LX where HY is in the MSDW

                count = cou;
                MyPoints &gridPts = m_Grids[i];
                MyPoint *first= &gridPts[0], *next = first;
                size_t nTotal = gridPts.size();
            
                const int UNROLL = 2;
                size_t n = nTotal / UNROLL;

                data = _mm_load_ps((float*)next);      // temp1 <- Y2 X2 Y1 X1 where Y2 is in the MSDW

                while(n--)
                {
                    // Get xyxy1 to have Y1 X1 Y1 X1
                    temp1 = data;
                    xyxy1 = _mm_lshift(temp1, 8);           // xyxy1 <- Y1 X1 0 0
                    temp2 = _mm_rshift(xyxy1, 8);           // temp2 <- 0 0 Y1 X1
                    xyxy1 = _mm_or_ps(xyxy1, temp2);        // xyxy1 = xyxy1 | temp2 : xyxy1 <- Y1 X1 Y1 X1

                    diff1 = _mm_sub_ps(xyxy1, ltrb);                
                    if(_mm_movemask_ps(diff1) == 0xC)       
                    {
                        *pCandidateNext++ = *next;
                        if(!--count) break;                       
                    }

                    ++next;
                    data = _mm_load_ps((float*)next);      // temp1 <- Y2 X2 Y1 X1 where Y2 is in the MSDW

                    // Get xyxy1 to have Y1 X1 Y1 X1
                    temp1 = data;
                    xyxy1 = _mm_lshift(temp1, 8);           // xyxy1 <- Y1 X1 0 0
                    temp2 = _mm_rshift(xyxy1, 8);           // temp2 <- 0 0 Y1 X1
                    xyxy1 = _mm_or_ps(xyxy1, temp2);        // xyxy1 = xyxy1 | temp2 : xyxy1 <- Y1 X1 Y1 X1

                    ++next;
                    data = _mm_load_ps((float*)next);      // temp1 <- Y2 X2 Y1 X1 where Y2 is in the MSDW

                    diff1 = _mm_sub_ps(xyxy1, ltrb);                
                    if(_mm_movemask_ps(diff1) == 0xC)       
                    {
                        *pCandidateNext++ = *(next-1);
                        if(!--count) break;                       
                    }
                }


                n = nTotal % UNROLL;
                while(n--)
                {
                    data = _mm_load_ps((float*)next);      // temp1 <- Y2 X2 Y1 X1 where Y2 is in the MSDW

                    // Get xyxy1 to have Y1 X1 Y1 X1
                    temp1 = data;
                    xyxy1 = _mm_lshift(temp1, 8);           // xyxy1 <- Y1 X1 0 0
                    temp2 = _mm_rshift(xyxy1, 8);           // temp2 <- 0 0 Y1 X1
                    xyxy1 = _mm_or_ps(xyxy1, temp2);        // xyxy1 = xyxy1 | temp2 : xyxy1 <- Y1 X1 Y1 X1

                    diff1 = _mm_sub_ps(xyxy1, ltrb);                
                    if(_mm_movemask_ps(diff1) == 0xC)       
                    {
                        *pCandidateNext++ = *next;
                        if(!--count) break;                       
                    }

                    ++next;
                }
            }

            sort(pCandidateStart, pCandidateNext, CmpPointRank<MyPoint>());
            //MyPtIter pti = m_vecCandidatePts.begin();
            for(count = 0; count < cou && pCandidateStart != pCandidateNext; ++count, ++pCandidateStart)            
            {
                pCandidateStart->writeToPoint(pPtsOut++);
            }

        }

        return count;
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


