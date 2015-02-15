//Brian Thorpe 2015
#ifdef THORPEDODLL_EXPORTS
#define THORPEDODLL_API __declspec(dllexport)
#else
#define THORPEDODLL_API __declspec(dllimport)
#endif

#include "CommonStructs.h"
extern "C"
{
	THORPEDODLL_API int  fnTestDll(void);
	THORPEDODLL_API  SearchContext* __stdcall create(const Point* points_begin, const Point* points_end);
	THORPEDODLL_API int32_t  __stdcall search(SearchContext* o, const Rect rect, const int32_t count, Point* out_points);
	THORPEDODLL_API SearchContext*  __stdcall destroy(SearchContext* o);
}
