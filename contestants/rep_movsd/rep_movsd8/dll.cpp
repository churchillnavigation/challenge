/*
Algorithm description :

Initial setup 
-------------
First we sort the points by Y coordinate
We subdivide the points recursively into horizontal slices, with equal number of points
We adjust the split so that no two points with the same Y coordinate are in different slices.
We split 5 times recursively, getting 32 slices

Next we repeat this process for each slice, splitting it along the Y axis.
We split 5 times recursively, getting 32*32 chunks in total

Each of these chunks is sorted by rank, so that the best ranked points are the earliest.
We also calculate the actual rectangle of each chunk.
For sanity, we store all rectangles as half open ranges : (lx, hx]

Next we order these chunks based on the best ranked point in each chunk.

For each chunk we make a vector of co-ordinates, and store each XY of each chunks points in the corresponding co-ord vector
We do this since we only need to read the X and Y while processing, and we can use SIMD to load 128 bits, or 4 floats 
which is 2 XY co-ordinates, if we keep them next to each other

A chunk is only a pair of pointers into the array of input points that we copy. 
Chunks contain no points values. We only make a copy of the input array because it's const and we need to sort it in 
every which way.

Searching
---------
Let N be the number of top ranked points requested...

For each input rectangle, we check it against each of our chunks bounding rectangles. 
We select those chunks that are completely inside the target rect and those chunks that overlap the rect

We have a result accumulator class - its job is to maintain the top N points.
When a new point is added to it, it checks if that point is in the top 20
If so it adds it and chucks out the 21st point if any
If not, it returns false, which means that any further points in the current chunk cannot possibly be a part 
of the final N, because all of the following points in the chunk have a worse rank. 


1) The chunks that are completely inside, we can process very quickly : 
For each chunk, we test its first point's rank against the worst rank of the result accumulator.
If it is greater, then this whole chunk has no points of interest, so we skip it and go to the next
If not, we iterate through the chunks points and add them to the accumulator. If the accumulator returns false, then the
rest of the chunks points are not useful, so we can skip them and go to the next chunk.
Doing this shenanigan causes at least 40% of points to be skipped in most tests 

2) The chunks that overlap the target rect, we do almost the same, except that when adding points we need to check if 
each of the chunks points is actually within the target rectangle. 

We check that as follows:

Load two XY coordinates into an SIMD register

Each X and Y of the co-ords is duplicated into two separate 128 bit SIMD registers
X1Y1X2Y2 -> X1Y1X1Y1
X1Y1X2Y2 -> X2Y2X2Y2

We put the rect's co-ord's into a 128 bit SIMD register, lets call it LLTTRRBB
The target rect has been adjusted outwards using _nextafterf() so that it is an open ranged rect rather than a closed one

Now we do a SIMD compare for both XYXYs using _mm_cmplt_ps
   X1 Y1 X1 Y1
<  LL TT RR BB
   ===========
   A1 B1 C1 D1

if A1 and B1 are false, X and Y are both NOT less than (meaning >= ) the top and left : X >= LX && Y >= LY
if C1 and D1 are true , X and Y are both less than the rect bottom right : X < HX && Y < HY
We could not have done this comparison in 1 instruction if we did not convert the rect to an open ranged one

We check if A and B are false and C and D are true - if that holds, then the point is in the target rect and we can 
feed it to the accumulator. We repeat the process for the second pair of coordinates.
The inner loop of this function is unrolled 3 times, so we process 6 points per iteration

The rest of the logic is the same as described above, we skip all the chunks remaining points if the accumulator 
returned false anytime.

*/

#include "point_search.h"

#define _SCL_SECURE_NO_WARNINGS 1

#include <vector>
#include <cmath>
#include <cfloat>
#include <algorithm>
#include <intrin.h>
#include <xmmintrin.h>
#include <emmintrin.h>
using namespace std;

uint64_t g_Total;
uint64_t g_Skipped;

double percent(uint64_t n, uint64_t total)
{
    double d = (double)n;
    d /= total;
    return d * 100;
}

//#define TEST 1

#ifdef TEST
FILE *g_LogFile;
#endif

#define MAKE_COMPARATOR(C, F) struct C {inline bool operator()(const Point& p1, const Point& p2) const {return p1.F < p2.F;}};
MAKE_COMPARATOR(CmpX, x)
MAKE_COMPARATOR(CmpY, y)

#define ALL(X) X.begin(), X.end()
#define FOR_EACH(T, I, C) for(T I = C.begin(); I != C.end(); ++I)
#define FOR_N(I, N) for(size_t I = 0; I < N; ++I)
#define fm_i _mm_castsi128_ps
#define BETWEEN(X, MN, MX) (X >= MN && X < MX)

#pragma pack(push, 1)
struct XY
{
    float x, y;
    XY(){}
    XY(float x, float y):x(x), y(y){}
};
//////////////////////////////////////////////////////////////////////////
#pragma pack(pop)

template<typename PT>
struct CmpPointRank
{
    inline bool operator()(const PT& p1, const PT &p2) const
    {
        return p1.rank < p2.rank;
    }
};
//////////////////////////////////////////////////////////////////////////


// This struct accumulates a list of the best N ranked points
// it maintains the list of current results sorted by rank
// When a point is added, it returns false if that point doesn't make it to the list 
// Since we always process points in ascending rank order per chunk, once this returns false, we can ignore all
// the other points n the chunk since they are guaranteed to be worse ranked than the results
class ResultVector
{
    typedef vector<Point> TList;
    typedef TList::iterator Iter;
    TList pts;

    
public:
    
    int nSize;

    ResultVector(int nSize):nSize(nSize){}
    //////////////////////////////////////////////////////////////////////////

    // Whats the worst ranked point so far
    inline int getWorstRank()
    {
        if(pts.size() >= nSize)
        {
            return pts.back().rank;
        }
        else
        {
            return INT_MAX;
        }
    }
    //////////////////////////////////////////////////////////////////////////

    // Add a point to results, return false if it did not make the cut
    // results remain sorted
    inline bool addPt(const Point &pt)
    {
        if(pts.size() == nSize)
        {
            // Where should this point go in the sorted by rank results?
            Iter itWhere = lower_bound(ALL(pts), pt, CmpPointRank<Point>());

            // If it goes anywhere except the end, add it
            if(itWhere != pts.end())
            {
                pts.insert(itWhere, pt);
                pts.resize(nSize);  // Throw away the last loser of a point
                return true;
            }
            return false;
        }
        else
        {
            pts.insert(pts.end(), pt);
            if(pts.size() == nSize)
            {
                sort(ALL(pts), CmpPointRank<Point>());
            }
            return true;
        }
    }
    //////////////////////////////////////////////////////////////////////////

    // Get the results out, we need to resort here in the rare case that pts.size() < nSize at the end of processing
    int getPts(Point *ptOut)
    {
        sort(ALL(pts), CmpPointRank<Point>());
        copy(pts.begin(), pts.end(), ptOut);
        return (int)pts.size();
    }
    //////////////////////////////////////////////////////////////////////////
};

//////////////////////////////////////////////////////////////////////////

// Chunk represents a rectangle full of points
// Points are sorted by rank
// Every chunk has the same number of points
// The bounding rectangle closed range ()
// Input rectangles are closed ranges too 
struct Chunk
{

    Point *beg;
    Point *end;
    Rect rc;
    int rank;

    Chunk(){}

    Chunk(Point *beg, Point *end): beg(beg), end(end){}
    //////////////////////////////////////////////////////////////////////////

    // Get the bounding rect, and the best rank
    void updateRectAndRank()
    {
        rc.lx = min_element(beg, end, CmpX())->x;
        rc.hx = max_element(beg, end, CmpX())->x;
        rc.ly = min_element(beg, end, CmpY())->y;
        rc.hy = max_element(beg, end, CmpY())->y;
        rc.hx = _nextafterf(rc.hx, FLT_MAX);
        rc.hy = _nextafterf(rc.hy, FLT_MAX);
        sort(beg, end, CmpPointRank<Point>());
        rank = beg->rank;
    }
    //////////////////////////////////////////////////////////////////////////

    int size()
    {
        return (int)(end - beg);
    }
    //////////////////////////////////////////////////////////////////////////

    // Does this rect overlap with the given one
    inline bool onRect(const Rect &rc2)
    {
        return
            rc.hx > rc2.lx && rc2.hx > rc.lx &&
            rc.hy > rc2.ly && rc2.hy > rc.ly;
    }
    //////////////////////////////////////////////////////////////////////////

    // whether this chunk is completely inside rcOuter
    inline bool inRect(const Rect &rcOuter)
    {
        return 
            rc.lx >= rcOuter.lx && rc.hx <= rcOuter.hx &&
            rc.ly >= rcOuter.ly && rc.hy <= rcOuter.hy;
    }
    //////////////////////////////////////////////////////////////////////////
};
//////////////////////////////////////////////////////////////////////////


typedef vector<Point> Points;
typedef Points::iterator PtIter;
typedef vector<XY> CoOrds;
typedef vector<Chunk> Chunks;

struct SearchContext
{
    typedef ResultVector Results;

    static const int UNROLL = 6;
    static const int SPLITS = 5;

    Chunks m_Chunks;
    size_t m_nPts;
    Points m_vecPts;
    vector<CoOrds> m_vecXYGrid;
    vector<size_t> m_vecInRectIdx;
    vector<size_t> m_vecOnRectIdx;

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
                // There have to be at least two points with two distinct Y values otherwise we cannot split
                // The case is pathological, but we need to handle it
                bool bHasDistinctY = false;
                if(i->size())
                {
                    sort(i->beg, i->end, CmpY());
                    float y1 = i->beg->y;
                    float y2 = ((i->end)-1)->y;
                    bHasDistinctY = y2 > y1;
                }

                if(bHasDistinctY)
                {
                    Point* pPtsMid = i->beg + i->size() / 2;
                    Point* pNext = pPtsMid + 1;
                    while(pPtsMid->y == pNext->y)
                    {
                        ++pPtsMid;
                        ++pNext;
                    }

                    chunks.push_back(Chunk(i->beg, pPtsMid));
                    chunks.push_back(Chunk(pPtsMid, i->end));
                }
                else
                {
                    chunks.push_back(*i);
                }
            }

            m_Chunks = chunks;
        }

        // Split SPLITS times
        FOR_N(j, SPLITS)
        {
            Chunks chunks;

            FOR_EACH(Chunks::iterator, i, m_Chunks)
            {
                bool bHasDistinctX = false;
                if(i->size())
                {
                    sort(i->beg, i->end, CmpX());
                    float x1 = i->beg->x;
                    float x2 = ((i->end)-1)->x;
                    bHasDistinctX = x2 > x1;
                }


                if(bHasDistinctX)
                {
                    Point* pPtsMid = i->beg + i->size() / 2;
                    Point* pNext = pPtsMid + 1;
                    while(pPtsMid->x == pNext->x)
                    {
                        ++pPtsMid;
                        ++pNext;
                    }

                    chunks.push_back(Chunk(i->beg, pPtsMid));
                    chunks.push_back(Chunk(pPtsMid, i->end));
                }
                else
                {
                    chunks.push_back(*i);
                }
            }

            m_Chunks = chunks;
        }
    }
    //////////////////////////////////////////////////////////////////////////

    SearchContext(const Point* pPtsBeg, const Point* pPtsEnd): m_vecPts(pPtsBeg, pPtsEnd)
    {
        m_nPts = pPtsEnd - pPtsBeg;
        if(m_nPts)
        {
            splitAll();

            // Update every chunks bounds and best rank
            size_t nChunks = m_Chunks.size();
            FOR_N(i, nChunks)
            {
                m_Chunks[i].updateRectAndRank();
            }

            // Sort the chunks vector itself ascending by rank
            sort(ALL(m_Chunks), CmpPointRank<Chunk>());

            // Store all the XYs in the XY vectors
            m_vecXYGrid.resize(nChunks);
            FOR_N(i, nChunks)
            {
                Chunk &c = m_Chunks[i];
                CoOrds &vecXYGrid = m_vecXYGrid[i];
                vecXYGrid.reserve(c.size());
                for(Point *p = c.beg; p != c.end; ++p)
                {
                    vecXYGrid.push_back(XY(p->x, p->y));
                }
            }
        }
    }
    //////////////////////////////////////////////////////////////////////////

    void searchOnRects(const Rect &rc, vector<size_t>& vecOnRectIdx, Results &results)
    {
        __m128 xmmXyxy1, xmmDiff1;
        __m128 xmmXyxy2, xmmDiff2;
        __m128 xmmData1, xmmData2, xmmData3;
        __m128 xmmRect = _mm_load_ps((float*)&rc);          // Load rect coords xmmRect <- HY LY HX LX where HY is in the MSDW

        FOR_N(i, vecOnRectIdx.size())
        {
            size_t iChunk = vecOnRectIdx[i];
            Chunk &c = m_Chunks[iChunk];

            // Check if this chunk is necessary to scan
            // If this chunks best point doesn't make the cut, ignore the whole chunk
            int bestChunkRank = c.beg->rank;
            int worstResultRank = results.getWorstRank();
            if(bestChunkRank > worstResultRank) 
            {
                continue;
            }

            CoOrds &vecXYGrid = m_vecXYGrid[iChunk];
            XY *pXYFirst = &vecXYGrid[0], *pXYNext = pXYFirst;
            size_t n = vecXYGrid.size();
            int r1, r2;

            n = vecXYGrid.size() / UNROLL;
            while(n--)
            {
                // Load 6 co-ordinates in all
                xmmData1 = _mm_load_ps((float*)pXYNext);         // xmmTemp1 <- Y2 X2 Y1 X1 where Y2 is in the MSDW
                xmmData2 = _mm_load_ps((float*)(pXYNext + 2));         
                xmmData3 = _mm_load_ps((float*)(pXYNext + 4));         

                xmmXyxy1 = _mm_shuffle_ps(xmmData1, xmmData1, 0x44);
                xmmXyxy2 = _mm_shuffle_ps(xmmData1, xmmData1, 0xEE);
                xmmDiff1 = _mm_cmplt_ps(xmmXyxy1, xmmRect);     // Result has signs ++-- if point in rect
                xmmDiff2 = _mm_cmplt_ps(xmmXyxy2, xmmRect);     // Result has signs ++-- if point in rect
                r1 = _mm_movemask_ps(xmmDiff1);
                r2 = _mm_movemask_ps(xmmDiff2);
                if(r1==0xC)
                {
                    int64_t idx = pXYNext - pXYFirst;
                    if(!results.addPt(c.beg[idx])) goto done;
                }
                if(r2==0xC)
                {
                    int64_t idx = pXYNext - pXYFirst;                                                                    
                    if(!results.addPt(c.beg[idx + 1])) goto done;
                }

                xmmXyxy1 = _mm_shuffle_ps(xmmData2, xmmData2, 0x44);
                xmmXyxy2 = _mm_shuffle_ps(xmmData2, xmmData2, 0xEE);
                xmmDiff1 = _mm_cmplt_ps(xmmXyxy1, xmmRect);     // Result has signs ++-- if point in rect
                xmmDiff2 = _mm_cmplt_ps(xmmXyxy2, xmmRect);     // Result has signs ++-- if point in rect
                r1 = _mm_movemask_ps(xmmDiff1);
                r2 = _mm_movemask_ps(xmmDiff2);
                if(r1==0xC)
                {
                    int64_t idx = pXYNext - pXYFirst; 
                    if(!results.addPt(c.beg[idx+2])) goto done;
                }
                if(r2==0xC)
                {
                    int64_t idx = pXYNext - pXYFirst;                                                                    
                    if(!results.addPt(c.beg[idx+3])) goto done;
                }

                xmmXyxy1 = _mm_shuffle_ps(xmmData3, xmmData3, 0x44);
                xmmXyxy2 = _mm_shuffle_ps(xmmData3, xmmData3, 0xEE);
                xmmDiff1 = _mm_cmplt_ps(xmmXyxy1, xmmRect);     // Result has signs ++-- if point in rect
                xmmDiff2 = _mm_cmplt_ps(xmmXyxy2, xmmRect);     // Result has signs ++-- if point in rect
                r1 = _mm_movemask_ps(xmmDiff1);
                r2 = _mm_movemask_ps(xmmDiff2);
                if(r1==0xC)
                {
                    int64_t idx = pXYNext - pXYFirst;                                                                    
                    if(!results.addPt(c.beg[idx+4])) goto done;
                }
                if(r2==0xC)
                {
                    int64_t idx = pXYNext - pXYFirst;                                                                    
                    if(!results.addPt(c.beg[idx+5])) goto done;
                }

                pXYNext += 6;
            }

            // Process any odd points
            n = vecXYGrid.size() % UNROLL;
            while(n--)
            {
                if(BETWEEN(pXYNext->x, rc.lx, rc.hx) && BETWEEN(pXYNext->y, rc.ly, rc.hy))
                {
                    int64_t idx = pXYNext - pXYFirst;                                                                    
                    if(!results.addPt(c.beg[idx])) goto done;
                }
                pXYNext++; 
            }
done:;
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

            m_vecInRectIdx.clear();
            m_vecOnRectIdx.clear();

            size_t nChunks = m_Chunks.size();
            FOR_N(i, nChunks)
            {
                Chunk &c = m_Chunks[i];
                if(c.inRect(rc))
                {
                    m_vecInRectIdx.push_back(i);
                }
                else if(c.onRect(rc))
                {
                    m_vecOnRectIdx.push_back(i);
                }
            }

            Results results(nRequested);
            FOR_N(i, m_vecInRectIdx.size())
            {
                size_t iChunk = m_vecInRectIdx[i];
                Chunk &c = m_Chunks[iChunk];
                int nSize = c.size();
                int nToCopy = std::min(nSize, nRequested);

                // Get best rank in chunk, and worst rank in results
                // If this chunks best point doesn't make the cut, ignore the whole chunk
                int bestChunkRank = c.beg->rank;
                int worstResultRank = results.getWorstRank();
                if(bestChunkRank > worstResultRank)
                {
                    continue;
                }

                // Keep adding points until rank is worse than the worst
                for(int k = 0; k < nToCopy; ++k)
                {
                    if(!results.addPt(c.beg[k])) break;
                }
            }

            searchOnRects(rc, m_vecOnRectIdx, results);
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
