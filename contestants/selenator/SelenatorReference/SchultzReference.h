#ifndef SCHULTZREFERENCE_H
#define SCHULTZREFERENCE_H

#include "schultzreference_global.h"
#include "point_search.h"


#ifdef __cplusplus
extern "C"
{
#endif

struct SearchContext
{
    Point* points_begin;
    Point* points_end;
    size_t numPoints;
};

S3_EXPORT SearchContext* create(const Point* points_begin, const Point* points_end);

S3_EXPORT int32_t search(SearchContext* sc, const Rect rect, const int32_t count, Point* out_points);

S3_EXPORT SearchContext* destroy(SearchContext* sc);

#ifdef __cplusplus
}
#endif

#endif // SCHULTZREFERENCE_H
