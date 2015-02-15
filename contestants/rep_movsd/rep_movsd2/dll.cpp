// -p1000000  -q10 -r20 
#include "point_search.h"
#include <vector>
#include <cmath>
#include <algorithm>
using namespace std;

//#define DUMP 1

#define ALL(X) X.begin(), X.end()
#define FOR_EACH(ITER_T, ITER, C) for(ITER_T ITER = C.begin(); ITER != C.end(); ++ITER)

#define DEF_COMPARATOR(FIELD)                                     \
struct CmpPoint ## FIELD                                    \
{                                                           \
    inline bool operator()(const Point& p1, const Point & p2) const     \
{                                                       \
    return p1.FIELD < p2.FIELD;                         \
}                                                       \
}

DEF_COMPARATOR(rank);
DEF_COMPARATOR(x);
DEF_COMPARATOR(y);
#define CMP(FIELD) CmpPoint ## FIELD()

typedef vector<Point> Points;
typedef Points::iterator PtIter;

#pragma pack(push, 1)
struct XY
{
    float x, y;
};


// union
// {
//     struct 
//     {
//         XY *first;
// 
//     };
// 
// };


#pragma pack(pop)


struct SearchContext
{
    Points m_vecPts;
    vector<XY> m_vecXYs;
    size_t m_size;
    vector<int> m_vecIDX;


    SearchContext(const Point* pPtsBeg, const Point* pPtsEnd): m_vecPts(pPtsBeg, pPtsEnd)
    {
        m_size = m_vecPts.size();

        if(m_size)
        {
            // Sort by linear index YX
            sort(ALL(m_vecPts), CmpPointrank());

            m_vecIDX.resize(m_size);
            m_vecXYs.reserve(m_size);
            XY p;
            FOR_EACH(PtIter, pt, m_vecPts)
            {
                p.x = pt->x;
                p.y = pt->y;
                m_vecXYs.push_back(p);
            }
        }
    }


    int32_t searchIt(const Rect rc, const int32_t cou, Point* pPtsOut)
    {
        int count = cou;


#define CL (first->x >= rc.lx)
#define CT (first->y >= rc.ly)
#define CR (first->x <= rc.hx)
#define CB (first->y <= rc.hy)

#define BTRL  if(CB && CT && CR && CL) { *pIDX = iSize;   pIDX ++; --count; if(!count) break;} if(!iSize--) break; ++first;
#define RLBT  if(CR && CL && CB && CT) { *pIDX = iSize;   pIDX ++; --count; if(!count) break;} if(!iSize--) break; ++first;

#define TWICE(X) X X

        if(m_size)
        {
            XY *first = &m_vecXYs[0];
            int *pIDX = &*m_vecIDX.begin(), *p = pIDX;
            int iSize = m_size - 1;
            float fW = (rc.hx - rc.lx);
            float fH = (rc.hy - rc.ly);

            if(fW > fH)
            {
                for(;;)                                         
                {
                    TWICE(TWICE(TWICE(TWICE(TWICE(TWICE(TWICE(BTRL)))))));
                } 
            }
            else
            {
                for(;;)                                         
                {
                    TWICE(TWICE(TWICE(TWICE(TWICE(TWICE(TWICE(RLBT)))))));
                } 

            }

            while(p != pIDX)
            {
                *pPtsOut++ = m_vecPts[m_size - (*p++ + 1)];
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
