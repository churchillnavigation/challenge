// -p1000000  -q10 -r20 
#include "point_search.h"
#include <vector>
#include <algorithm>
using namespace std;

//#define DUMP 1

#define ALL(X) X.begin(), X.end()
#define FOR_EACH(ITER_T, ITER, C) for(ITER_T ITER = C.begin(); ITER != C.end(); ++ITER)

typedef vector<Point> Points;
typedef Points::iterator PtIter;

struct CmpPointRank
{
    inline bool operator()(const Point& p1, const Point& p2) const
    {
        return p1.rank < p2.rank;
    }
};

struct XY
{
    float x, y;
};


struct SearchContext
{
    Points m_vecPts;
    vector<XY> m_vecXYs;
    XY *m_first, *m_last; 
    size_t m_size;
    vector<int> m_vecIDX;

    SearchContext(const Point* pPtsBeg, const Point* pPtsEnd): m_vecPts(pPtsBeg, pPtsEnd), m_first(0), m_last(0)
    {
        m_size = m_vecPts.size();

        if(m_size)
        {
            // Sort by linear index YX
            sort(ALL(m_vecPts), CmpPointRank());

            m_vecIDX.resize(m_size);
            m_vecXYs.reserve(m_size);
            FOR_EACH(Points::iterator, pt, m_vecPts)
            {
                XY p;
                p.x = pt->x;
                p.y = pt->y;
                m_vecXYs.push_back(p);
            }

            m_first = &*m_vecXYs.begin();
            m_last = m_first + m_size;
        }
    }

    // For tall rectangles, compare x first
    inline bool isPointInRectXY(const XY &pt, const Rect &rc)
    {
        return (pt.x >= rc.lx && pt.x <= rc.hx && pt.y >= rc.ly && pt.y <= rc.hy);
    }

    // For wide rectangles compare y first
    inline bool isPointInRectYX(const XY &pt, const Rect &rc)
    {
        return (pt.y >= rc.ly && pt.y <= rc.hy && pt.x >= rc.lx && pt.x <= rc.hx);
    }


    int32_t searchIt(const Rect rc, const int32_t cou, Point* pPtsOut)
    {
        int count = cou;

        if(m_size)
        {
            XY *first = m_first;
            int idx = 0;
            int *pIDX = &*m_vecIDX.begin(), *p = pIDX;

            // Tall rectangle use isPointInRectXY
            if((rc.hx - rc.lx) < (rc.hy - rc.ly))
            {
                for(idx = 0; idx < m_size; ++idx, ++first)
                {
                    if(isPointInRectXY(*first, rc)) 
                    {                               
                        *pIDX++ = idx;        
                        if(!--count) break;           
                    }                               
                }
            }
            else
            {
                for(idx = 0; idx < m_size; ++idx, ++first)
                {
                    if(isPointInRectYX(*first, rc)) 
                    {                               
                        *pIDX++ = idx;        
                        if(!--count) break;           
                    }                               
                }
            }

            while(p != pIDX)
            {
                *pPtsOut++ = m_vecPts[*p++];
            }

        }

        return cou - count;
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
