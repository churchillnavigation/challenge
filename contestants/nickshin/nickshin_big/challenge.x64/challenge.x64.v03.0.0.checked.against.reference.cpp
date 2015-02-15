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
 * v03: CHALLENGE - DEVELOPMENT VERSION (inlined check with ref dll)
 * V02: SANITY CHECK THE RUN WITH REFERENCE DLL
 * V01: SANITY CHECK THE RUN WITH BARE BONES STUBBED FUNCTIONS
 */


#include <stdlib.h> // NULL malloc free

#define CURIOSITY_CHECKS 1
#define COMPARE_WITH_REFERENCE 0


#if CURIOSITY_CHECKS
#include <unordered_map>
#endif


#if COMPARE_WITH_REFERENCE
SearchContext* localObject = NULL;
#endif


struct SearchContext
{
	Point* points;
	int size;
};


SearchContext* _create(const Point* points_begin, const Point* points_end)
{
	SearchContext* sc;
	Point* point;
	const Point* p;
	int i, size;

	// SANITY CHECKS
	if ( ! points_begin || ! points_end )
		return NULL;

	// CALCULATE SIZE
	size = (int)(points_end - points_begin);
	if ( ! size )
		return NULL;

	// ALLOCATE OBJECT
	sc = new SearchContext;
	if ( ! sc )
		return NULL;
	i = size * sizeof(Point);
	sc->points = (Point*)malloc( i );
	if ( ! sc->points ) {
		delete sc;
		return NULL;
	}
	memset( sc->points, -1, i );
	sc->size = size;

#if CURIOSITY_CHECKS
	// EXTRA TEST -- ARE RANK IDs UNIQUE
	p = points_begin;
	std::unordered_map<int32_t, const Point*> data;
	int32_t min = 0;
	int32_t max = 0;
	for ( i = 0; i < size; i++, p++ ) {
		if ( data[p->rank] )
			DebugBreak();
		if ( p->rank < min ) min = p->rank;
		if ( p->rank > max ) max = p->rank;
		data[p->rank] = p;
	}

	// EXTRA TEST -- ARE RANK IDs WITHIN LIMITS
	if ( min < 0 || max >= size )
		DebugBreak();

	// EXTRA TEST -- ARE RANK IDs SEQUENTIAL
	for ( i = min; i < max; i++ ) {
		if ( ! data[i] )
			DebugBreak();
	}
#endif

	p = points_begin;
	point = sc->points;
	for ( i = 0; i < size; i++, p++ )
		*(point + p->rank) = *p;

#if COMPARE_WITH_REFERENCE
	localObject = sc;
#endif
	return sc;
}

int32_t _search(SearchContext* sc, const Rect rect, const int32_t count, Point* out_points)
{
	// SANITY CHECKS
#if COMPARE_WITH_REFERENCE
	sc = localObject;
#endif
	if ( ! sc || ! count || ! out_points )
		return 0;

	// COPY THE FIRST count POINTS INSIDE RECT
	Point* outer = out_points;
	Point* point = sc->points;
	for ( int i = 0; i < sc->size; point++, i++ ) {
		if ( point->x < rect.lx || point->x > rect.hx ||
			 point->y < rect.ly || point->y > rect.hy )
			continue;
#if COMPARE_WITH_REFERENCE
		if (    outer->id  != point->id
			|| outer->rank != point->rank
			|| outer->x    != point->x
			|| outer->y    != point->y )
			DebugBreak();
#else
//		if ( point->rank >= 0 )
			*outer = *point;
#endif
		outer += 1;
		if ( (outer - out_points) >= count )
			break;
	}
	// NOTE TO SELF: 14% FASTER THAN REFERENCE !!!  ^_^
	return (int32_t)(outer - out_points);
}

SearchContext* _destroy(SearchContext* sc)
{
#if COMPARE_WITH_REFERENCE
	sc = localObject;
	localObject = NULL;
#endif
	if ( sc ) {
		free( sc->points );
		delete sc;
		sc = NULL;
	}
	return sc;
}


/*___________________________________________________________________________________________________________________________*/
/* The following is based on V02 -- this will be used to double check my work */


#if COMPARE_WITH_REFERENCE
HINSTANCE DllRef    = NULL;
T_create  createRef  = NULL;
T_search  searchRef  = NULL;
T_destroy destroyRef = NULL;
#endif


extern "C" __declspec(dllexport) SearchContext* create(const Point* points_begin, const Point* points_end)
{
	SearchContext* sc = NULL;
#if COMPARE_WITH_REFERENCE
	DllRef = LoadLibrary(TEXT("reference.dll"));
	if (DllRef) {
		createRef  = (T_create)  GetProcAddress(DllRef, "create");
		searchRef  = (T_search)  GetProcAddress(DllRef, "search");
		destroyRef = (T_destroy) GetProcAddress(DllRef, "destroy");
		if (createRef)
			sc = (createRef)(points_begin, points_end);
	}
	_create(points_begin, points_end);
#else
	sc = _create(points_begin, points_end);
#endif
	return sc;
}

extern "C" __declspec(dllexport) int32_t search(SearchContext* sc, const Rect rect, const int32_t count, Point* out_points)
{
	int32_t result = 0;
#if COMPARE_WITH_REFERENCE
	if ( searchRef )
		result = (searchRef)(sc, rect, count, out_points);
	if ( sc )
		_search(NULL, rect, count, out_points);
#else
	result = _search(sc, rect, count, out_points);
#endif
	return result;
}

extern "C" __declspec(dllexport) SearchContext* destroy(SearchContext* sc)
{
	SearchContext* _sc = NULL;
#if COMPARE_WITH_REFERENCE
	if ( destroyRef )
		_sc = (destroyRef)(sc);
	if (DllRef) {
		FreeLibrary(DllRef);
		DllRef     = NULL;
		createRef  = NULL;
		searchRef  = NULL;
		destroyRef = NULL;
	}
	_destroy(NULL);
#else
	_sc= _destroy(sc);
#endif
	return _sc;
}
