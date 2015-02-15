#include "point_search.h"
#include <vector>
#include <cmath>
#include <algorithm>
#include <xmmintrin.h>
#include <emmintrin.h>
using namespace std;

#pragma pack(push, 1)
struct XYXY
{
    float x, y, x1, y1;
};
#pragma pack(pop)


struct SearchContext
{
    typedef vector<Point> Points;
    typedef Points::iterator PtIter;

    Points m_vecPts;
    vector<XYXY> m_vecXYs;
    size_t m_size;
    vector<size_t> m_vecIDX;

    struct CmpPointRank                                    
    {                                                           
        inline bool operator()(const Point& p1, const Point & p2) const     
        {                                                       
            return p1.rank < p2.rank;                         
        }                                                       
    };

    SearchContext(const Point* pPtsBeg, const Point* pPtsEnd): m_vecPts(pPtsBeg, pPtsEnd)
    {
        m_size = m_vecPts.size();

        if(m_size)
        {
            // Sort by linear index YX
            sort(m_vecPts.begin(), m_vecPts.end(), CmpPointRank());

            m_vecIDX.resize(m_size);
            m_vecXYs.reserve(m_size);
            XYXY p;
            for(PtIter pt = m_vecPts.begin(); pt != m_vecPts.end(); ++pt)
            {
                p.x = p.x1 = pt->x;
                p.y = p.y1 = pt->y;
                m_vecXYs.push_back(p);
            }
        }
    }


    int32_t searchIt(const Rect rc, const int32_t cou, Point* pPtsOut)
    {
        int count = cou;

        if(m_size)
        {
            XYXY *first = &m_vecXYs[0];
            size_t *pIDX = &m_vecIDX[0], *p = pIDX;
            size_t iSize = m_size - 1;

#define to_i _mm_castps_si128
#define fm_i _mm_castsi128_ps

            __m128 ltrb = _mm_load_ps((float*)&rc);
            __m128i adjust = _mm_set_epi32(1, 1, 0, 0);

            for(;;)
            {
                //  XYXY 
                // -LTRB
                //  ++--
                __m128 xyxy = _mm_load_ps((float*)first);
                __m128 diff = _mm_sub_ps(xyxy, ltrb);

                // Sometimes the difference will be exactly 0
                // This should still count as - for the last 2 floats
                // We decrement the third and 4th float treating them as 32 bit integers
                // For the value 0, this will flip the sign bit of the float since it will go to 0xffffffff
                __m128i diff2 = _mm_sub_epi32(to_i(diff), adjust);

                // Get the sign bits for each float in diff
                int ret = _mm_movemask_ps(fm_i(diff2));

                //bool bCompare2 = compBTRL((float*)first, (float*)&rc);
                if(ret == 0xC) 
                { 
                    *pIDX++ = iSize;   
                    if(!--count) break;
                } 

                if(!iSize--) break; 
                ++first;
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
