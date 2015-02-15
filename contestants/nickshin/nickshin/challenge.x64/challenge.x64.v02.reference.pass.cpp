/* NICK SHIN
 * nick.shin@gmail.com
 * Written: Dec 2014
 * License: public domain
 *
 * For: http://churchillnavigation.com/challenge/
 */


/*___________________________________________________________________________________________________________________________*/
/* Windows x64 */

// challenge.x64.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "challenge.x64.h"


// This is an example of an exported variable
CHALLENGEX64_API int nchallengex64=0;

// This is an example of an exported function.
CHALLENGEX64_API int fnchallengex64(void)
{
	return 42;
}

// This is the constructor of a class that has been exported.
// see challenge.x64.h for the class definition
Cchallengex64::Cchallengex64()
{
	return;
}

/*___________________________________________________________________________________________________________________________*/
/* Churchill Navigation Challenge */

/* Given 10 million uniquely ranked points on a 2D plane, design a datastructure and an algorithm that can find the 20
most important points inside any given rectangle. The solution has to be reasonably fast even in the worst case, while
also not using an unreasonably large amount of memory.
Create an x64 Windows DLL, that can be loaded by the test application. The DLL and the functions will be loaded
dynamically. The DLL should export the following functions: "create", "search" and "destroy". The signatures and
descriptions of these functions are given below. You can use any language or compiler, as long as the resulting DLL
implements this interface. */

/* This standard header defines the sized types used. */
#include <stdint.h>

/* The following structs are packed with no padding. */
#pragma pack(push, 1)

/* Defines a point in 2D space with some additional attributes like id and rank. */
struct Point
{
	int8_t id;
	int32_t rank;
	float x;
	float y;
};

/* Defines a rectangle, where a point (x,y) is inside, if x is in [lx, hx] and y is in [ly, hy]. */
struct Rect
{
	float lx;
	float ly;
	float hx;
	float hy;
};
#pragma pack(pop)

/* Declaration of the struct that is used as the context for the calls. */
struct SearchContext;

/* Load the provided points into an internal data structure. The pointers follow the STL iterator convention, where
"points_begin" points to the first element, and "points_end" points to one past the last element. The input points are
only guaranteed to be valid for the duration of the call. Return a pointer to the context that can be used for
consecutive searches on the data. */
typedef SearchContext* (__stdcall* T_create)(const Point* points_begin, const Point* points_end);

/* Search for "count" points with the smallest ranks inside "rect" and copy them ordered by smallest rank first in
"out_points". Return the number of points copied. "out_points" points to a buffer owned by the caller that
can hold "count" number of Points. */
typedef int32_t (__stdcall* T_search)(SearchContext* sc, const Rect rect, const int32_t count, Point* out_points);

/* Release the resources associated with the context. Return nullptr if successful, "sc" otherwise. */
typedef SearchContext* (__stdcall* T_destroy)(SearchContext* sc);


/*___________________________________________________________________________________________________________________________*/
/* NICK SHIN */

/* CHANGELIST:
 * V02: SANITY CHECK THE RUN WITH REFERENCE DLL
 * V01: SANITY CHECK THE RUN WITH BARE BONES STUBBED FUNCTIONS
 */


HINSTANCE DllRef    = NULL;
T_create  createRef  = NULL;
T_search  searchRef  = NULL;
T_destroy destroyRef = NULL;


extern "C" __declspec(dllexport) SearchContext* create(const Point* points_begin, const Point* points_end)
{
	SearchContext* sc = NULL;
	DllRef = LoadLibrary(TEXT("reference.dll"));
	if (DllRef) {
		createRef  = (T_create)  GetProcAddress(DllRef, "create");
		searchRef  = (T_search)  GetProcAddress(DllRef, "search");
		destroyRef = (T_destroy) GetProcAddress(DllRef, "destroy");
		if (createRef)
			sc = (createRef)(points_begin, points_end);
	}
	return sc;
}

extern "C" __declspec(dllexport) int32_t search(SearchContext* sc, const Rect rect, const int32_t count, Point* out_points)
{
	int32_t result = 0;
	if ( searchRef )
		result = (searchRef)(sc, rect, count, out_points);
	return result;
}

extern "C" __declspec(dllexport) SearchContext* destroy(SearchContext* sc)
{
	SearchContext* _sc = NULL;
	if ( destroyRef )
		_sc = (destroyRef)(sc);
	if (DllRef) {
		FreeLibrary(DllRef);
		DllRef     = NULL;
		createRef  = NULL;
		searchRef  = NULL;
		destroyRef = NULL;
	}
	return _sc;
}
