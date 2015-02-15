/****************************************************************************
**
** Copyright (C) 2015 Schultz Software Solutions LLC
** Author: Greg Schultz
** Contact: gregory.schultz@embarqmail.com
** Creation Date: 5 Jan 2015
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of Schultz Software Solutions LLC nor the names
**     of its contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "SchultzReference.h"

#include <cstdlib>
#include <future>
#include <algorithm>


int cmpfunc (const void* pt1, const void* pt2)
{
    return ( ((Point*)pt1)->rank - ((Point*)pt2)->rank );
}

SearchContext* create(const Point* points_begin, const Point* points_end)
{
    SearchContext* sc = new SearchContext;

    sc->numPoints = points_end - points_begin;

    sc->points_begin = new Point[sc->numPoints];

    sc->points_end = sc->points_begin + sc->numPoints;

    memcpy(sc->points_begin, points_begin, sc->numPoints*sizeof(Point));

    qsort((void*)sc->points_begin, sc->numPoints, sizeof(Point), cmpfunc);

    sc->match = new Point[sc->numPoints];

    return sc;
}

int32_t search_part(Point* points_begin, Point* points_end, const Rect rect, const int32_t count, Point* out_points)
{
    int32_t numfound( 0 );

    for(Point* ptr=points_begin; ptr<points_end; ++ptr)
    {
        if ( ptr->x < rect.lx || ptr->x > rect.hx )
            continue;

        if ( ptr->y < rect.ly || ptr->y > rect.hy )
            continue;

        *(out_points+numfound) = *ptr;
        if ( ++numfound >= count ) break;
    }

    return numfound;
}

#define MAIN_SIZE  1
#define CHUNKS     7

int32_t search(SearchContext* sc, const Rect rect, const int32_t count, Point* out_points)
{
    size_t searchSize = (sc->points_end - sc->points_begin) / CHUNKS;
    for (int i=MAIN_SIZE; i<CHUNKS-1; ++i)
    {
        sc->future[i] = std::async(std::launch::async, search_part, sc->points_begin+i*searchSize, sc->points_begin+(i+1)*searchSize, rect, count, sc->match+(i-1)*count);
    }
    sc->future[CHUNKS-1] = std::async(std::launch::async, search_part, sc->points_begin+(CHUNKS-1)*searchSize, sc->points_end, rect, count, sc->match+(CHUNKS-2)*count);
    int32_t numfound = search_part(sc->points_begin, sc->points_begin+MAIN_SIZE*searchSize, rect, count, out_points);
    for (sc->idx=MAIN_SIZE; sc->idx<CHUNKS&&numfound < count; ++sc->idx)
    {
        int32_t numfoundNext = sc->future[sc->idx].get();
        if (numfoundNext > 0)
        {
            int32_t copycount = std::min(count - numfound, numfoundNext);
            memcpy(out_points+numfound, sc->match+(sc->idx-1)*count, copycount * sizeof(Point));
            numfound += copycount;
        }
    }
    return numfound;
}

SearchContext* destroy(SearchContext* sc)
{
    for (int i=sc->idx; i<CHUNKS; ++i)
    {
        sc->future[i].wait();
    }
    delete[] sc->points_begin;
    delete[] sc->match;
    delete sc;
    return NULL;
}
