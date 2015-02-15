#include "ExportedFunctions.h"

#include "SearchContext.h"

extern "C"
{

	CHURCHHILLNAVCHALLENGE_API SearchContext* create(const Point* points_begin, const Point* points_end)
	{
		return new SearchContext(points_begin, points_end);
	}

	CHURCHHILLNAVCHALLENGE_API int32_t search(SearchContext* sc, const Rect rect, const int32_t count, Point* out_points)
	{
		return nullptr != sc ? sc->Search(rect, count, out_points) : 0;
	}

	CHURCHHILLNAVCHALLENGE_API SearchContext* destroy(SearchContext* sc)
	{
		delete sc;
		return nullptr;
	}

}
