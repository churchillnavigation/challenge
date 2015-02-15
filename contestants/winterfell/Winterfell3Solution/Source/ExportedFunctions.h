#include "point_search.h"

#ifdef CHURCHHILLNAVCHALLENGE_EXPORTS
#define CHURCHHILLNAVCHALLENGE_API __declspec(dllexport)
#else
#define CHURCHHILLNAVCHALLENGE_API __declspec(dllimport)
#endif

extern "C"
{

	CHURCHHILLNAVCHALLENGE_API SearchContext*	create(const Point* points_begin, const Point* points_end);
	CHURCHHILLNAVCHALLENGE_API int32_t			search(SearchContext* sc, const Rect rect, const int32_t count, Point* out_points);
	CHURCHHILLNAVCHALLENGE_API SearchContext*	destroy(SearchContext* sc);

}
