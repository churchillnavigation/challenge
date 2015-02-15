// dllmain.cpp : Defines the entry point for the DLL application.
#include "stdafx.h"
#include "dllmain.h"
#include <stdexcept>
#include <Windows.h>

using namespace std;
BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}

DWORD processPriority;
DWORD threadPriority;

/* Load the provided points into an internal data structure. The pointers follow the STL iterator convention, where
"points_begin" points to the first element, and "points_end" points to one past the last element. The input points are
only guaranteed to be valid for the duration of the call. Return a pointer to the context that can be used for
consecutive searches on the data. */
SearchContext* create(const Point* points_begin, const Point* points_end)
{
	// Hopefully input is good...
	if (points_end < points_begin)
	{
		return nullptr;
	}

	SearchContext* context = new SearchContext;
    
	context->point_container = new PointContainer(points_begin, points_end);
    
    return context;
}

/* Search for "count" points with the smallest ranks inside "rect" and copy them ordered by smallest rank first in
"out_points". Return the number of points copied. "out_points" points to a buffer owned by the caller that
can hold "count" number of Points. */
int32_t search(SearchContext* sc, const Rect rect, const int32_t count, Point* out_points)
{
	return sc->point_container->Evaluate(rect, count, out_points);
}

/* Release the resources associated with the context. Return nullptr if successful, "sc" otherwise. */
SearchContext* destroy(SearchContext* sc)
{
	if (sc != nullptr)
	{
		if (sc->point_container != nullptr)
		{
			delete sc->point_container;
			sc->point_container = nullptr;
		}
		sc = nullptr;

        // Restore to previous
        SetPriorityClass(GetCurrentProcess(), processPriority);
        SetThreadPriority(GetCurrentThread(), threadPriority);
	}


	return sc;
}