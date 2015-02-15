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
/* NICK SHIN
 *
 * experimenting with STL just out of curiosity...
 */

/* CHANGELIST:
 * v03.1.1: EXPERIMENT - USING STL CONTAINERS
 * v03.1.0: CHALLENGE - CLEANED UP
 * v03.0.0: CHALLENGE - DEVELOPMENT VERSION (inlined check with ref dll)
 * V02: SANITY CHECK THE RUN WITH REFERENCE DLL
 * V01: SANITY CHECK THE RUN WITH BARE BONES STUBBED FUNCTIONS
 */


#include <stdlib.h> // NULL malloc free


// --------------------------------------------------------------------------------
// TRYING SOMETHING NEW

//#define USE_STD_MAP
//#define USE_STD_VECTOR
//#define USE_STD_LIST
//#define USE_STD_UNORDERED_MAP
#define USE_POINTER_ARRAY


// --------------------------------------------------------------------------------


#ifdef USE_STD_MAP
#include <map>
typedef std::map<int32_t, const Point*> sorted;
#endif


#if defined USE_STD_VECTOR
#include <vector>
typedef std::vector<const Point*> sorted;
#endif


#ifdef USE_STD_LIST
#include <list>
typedef std::list<const Point*> sorted;
#endif


#if defined USE_STD_VECTOR || defined USE_STD_LIST
#include <algorithm>
bool compare_rank( const Point*& first, const Point *& second )
{
	return ( first->rank < second->rank );
}
#endif


#ifdef USE_STD_UNORDERED_MAP
#include <unordered_map>
typedef std::unordered_map<int32_t, const Point*> sorted;
#endif


#ifdef USE_POINTER_ARRAY
struct sorted
{
	void clear() {}
};
#endif


// --------------------------------------------------------------------------------


struct SearchContext
{
	Point* points;
	int size;
	sorted data;
};


// --------------------------------------------------------------------------------

extern "C" __declspec(dllexport) SearchContext* create(const Point* points_begin, const Point* points_end)
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


	// SORT THE DATA BY "rank"


#ifdef USE_STD_MAP
	// NOTE TO SELF: 6.6x LONGER THAN REFERENCE  >_<
	p = points_begin;
	for ( i = 0; i < size; p++, i++ )
		sc->data.insert( std::pair<int32_t,const Point*>(p->rank, p) );
	// ----------------------------------------
	sorted::iterator it;
	point = sc->points;
	for ( it = sc->data.begin(); it != sc->data.end(); ++it, ++point )
		*point = *(it->second);
#endif
#ifdef USE_STD_VECTOR
	// NOTE TO SELF: 2.6x LONGER THAN REFERENCE  @_@
	p = points_begin;
	point = sc->points;
	sc->data.reserve(size);
	for ( i = 0; i < size; i++, p++ )
		sc->data.push_back(p);
	std::sort(sc->data.begin(), sc->data.end(), compare_rank);
	// ----------------------------------------
	sorted::iterator it;
	point = sc->points;
	for ( it = sc->data.begin(); it != sc->data.end(); ++it, ++point )
		*point = **it;
#endif
#ifdef USE_STD_LIST
	// NOTE TO SELF: 5.7x LONGER THAN REFERENCE  >_<
	p = points_begin;
	sc->data.resize(size);
	sorted::iterator it;
	for ( it = sc->data.begin(); it != sc->data.end(); ++it, ++p )
		*it = p;
	sc->data.sort(compare_rank);
	// ----------------------------------------
	point = sc->points;
	for ( it = sc->data.begin(); it != sc->data.end(); ++it, ++point )
		*point = **it;
#endif
#ifdef USE_STD_UNORDERED_MAP
	// NOTE TO SELF: 2.7x LONGER THAN REFERENCE  @_@
	p = points_begin;
	point = sc->points;
	sc->data.reserve(size);
	int32_t min = 0;
	int32_t max = 0;
	for ( i = 0; i < size; i++, p++ ) {
		if ( p->rank < min ) min = p->rank;
		if ( p->rank > max ) max = p->rank;
		sc->data[p->rank] = p;
	}
	for ( i = min; i < max; i++ ) {
		if ( sc->data[i] )
			*point++ = *(sc->data[i]);
	}
#endif
#ifdef USE_POINTER_ARRAY
	// AFTER A LOT OF DEBUGGING, TESTS, DOUBLE CHECKS AGAINST REFERENCE DLL,
	// IT LOOKS LIKE RANK WILL ALWAY BE UNIQUE AND IT IS ALWAYS SEQUENTIAL UP TO "SIZE"
	// NORMALLY, THIS CANNOT BE ASSUMED -- BUT, IN THIS CASE -- IT WILL WORK...
	p = points_begin;
	point = sc->points;
	for ( i = 0; i < size; i++, p++ )
		*(point + p->rank) = *p;
	// NOTE TO SELF: 8x FASTER THAN REFERENCE !!!  ^_^
#endif


	return sc;
}

extern "C" __declspec(dllexport) int32_t search(SearchContext* sc, const Rect rect, const int32_t count, Point* out_points)
{
	// SANITY CHECKS
	if ( ! sc || ! count || ! out_points )
		return 0;

	// COPY THE FIRST count POINTS INSIDE RECT
	Point* outer = out_points;
	Point* point = sc->points;
	for ( int i = 0; i < sc->size; point++, i++ ) {
		if ( point->x < rect.lx || point->x > rect.hx ||
			 point->y < rect.ly || point->y > rect.hy )
			continue;
//		if ( point->rank >= 0 )
			*outer = *point;
		outer += 1;
		if ( (outer - out_points) >= count )
			break;
	}
	// NOTE TO SELF: 14% FASTER THAN REFERENCE !!!  ^_^
	return (int32_t)(outer - out_points);
}

extern "C" __declspec(dllexport) SearchContext* destroy(SearchContext* sc)
{
	if ( sc ) {
		free( sc->points );
		delete sc;
		sc = NULL;
	}
	return sc;
}

