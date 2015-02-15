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

#define MAX_LEVELS   3


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

    return sc;
}

int32_t search_half(Point* points_begin, Point* points_end, int level, const Rect rect, const int32_t count, Point* out_points)
{
    int32_t numfound;
    if (++level < MAX_LEVELS)
    {
        Point* match = new Point[count];
        Point* points_mid = points_begin + (points_end - points_begin) / 2;
        std::future<int32_t> leftFuture = std::async(std::launch::async, search_half, points_begin, points_mid, level, rect, count, out_points);
        int32_t numfoundRight = search_half(points_mid, points_end, level, rect, count, match);
        numfound = leftFuture.get();
        if (numfound < count)
        {
            if (numfoundRight > 0)
            {
                int32_t copycount = std::min(count - numfound, numfoundRight);
                memcpy(out_points+numfound, match, copycount * sizeof(Point));
                numfound += copycount;
            }
        }
        delete[] match;
    }
    else
    {
        numfound = 0;
        for(Point* ptr=points_begin; ptr<points_end; ++ptr)
        {
            if ( ptr->x < rect.lx || ptr->x > rect.hx )
                continue;

            if ( ptr->y < rect.ly || ptr->y > rect.hy )
                continue;

            *(out_points+numfound) = *ptr;
            if ( ++numfound >= count ) break;
        }
    }

    return numfound;
}

int32_t search(SearchContext* sc, const Rect rect, const int32_t count, Point* out_points)
{
    return search_half(sc->points_begin, sc->points_end, 0, rect, count, out_points);
}

SearchContext* destroy(SearchContext* sc)
{
    delete[] sc->points_begin;
    delete sc;
    return NULL;
}
