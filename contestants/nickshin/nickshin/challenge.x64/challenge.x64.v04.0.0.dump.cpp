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
 * v04.0.0: EXPERIMENT - DUMP POINTS AND RECT FOR INSPECTION
 * v03.1.1: EXPERIMENT - USING STL CONTAINERS
 * v03.1.0: CHALLENGE - CLEANED UP
 * v03.0.0: CHALLENGE - DEVELOPMENT VERSION (inlined check with ref dll)
 * V02: SANITY CHECK THE RUN WITH REFERENCE DLL
 * V01: SANITY CHECK THE RUN WITH BARE BONES STUBBED FUNCTIONS
 */


#include <stdlib.h> // NULL malloc free
#include <fstream> // fwrite


#define EXPERIMEMT_DUMP	5
/* experiment 1 just dumped points_begin to points_end to a txt file
 * experiment 2 just dumped search rect to a text file
 * experiment 3 generated an image based on points from points_begin to points_end
 * experiment 4 added on to #3 with search rect drawn on it
 * experiment 5 is just based on the visual inspection on the 100s of images generated
 */


// for testing...
#define LIMIT 10000.0f


struct SearchContext
{
	Point* points;
	int size;
	Rect box;
#if EXPERIMEMT_DUMP == 5
	FILE* stream;
	int total;
	int skip;
#endif
};


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

	// EXPERIMENT DUMP
#if EXPERIMEMT_DUMP == 5
	fopen_s( &(sc->stream), "dump.5.txt", "a" );
//	fprintf( sc->stream, "LIMIT: %.16f\n", LIMIT );
	p = points_begin;
	sc->box.lx =  LIMIT;
	sc->box.hx = -LIMIT;
	sc->box.ly =  LIMIT;
	sc->box.hy = -LIMIT;
	for ( i = 0; i < size; i++, p++ ) {
		if ( p->x < sc->box.lx && p->x > -LIMIT ) sc->box.lx = p->x;
		if ( p->x > sc->box.hx && p->x <  LIMIT ) sc->box.hx = p->x;
		if ( p->y < sc->box.ly && p->y > -LIMIT ) sc->box.ly = p->y;
		if ( p->y > sc->box.hy && p->y <  LIMIT ) sc->box.hy = p->y;
		if ( p->x < -LIMIT ||
			 p->x >  LIMIT ||
			 p->y < -LIMIT ||
			 p->y >  LIMIT ) {
			if ( abs(p->x) != 10000000000.0f && abs(p->y) != 10000000000.0f )
				fprintf( sc->stream, "!!!!!: %.16f,%.16f\n", p->x, p->y );
//			else
//				fprintf( sc->stream, "POINT: %.16f,%.16f\n", p->x, p->y );
		}
	}
//	fprintf( sc->stream, "BOX: %.16f,%.16f,%.16f,%.16f\n", sc->box.lx, sc->box.hx, sc->box.ly, sc->box.hy );
	sc->total = 0;
	sc->skip = 0;
#endif

	p = points_begin;
	point = sc->points;
	for ( i = 0; i < size; i++, p++ )
		*(point + p->rank) = *p;

	return sc;
}

extern "C" __declspec(dllexport) int32_t search(SearchContext* sc, const Rect rect, const int32_t count, Point* out_points)
{
	// SANITY CHECKS
	if ( ! sc || ! count || ! out_points )
		return 0;

#if EXPERIMEMT_DUMP == 5
	sc->total += 1;
	if ( rect.lx > sc->box.hx ||
		 rect.hx < sc->box.lx ||
		 rect.ly > sc->box.hy ||
		 rect.hy < sc->box.ly ) {
		if ( rect.hx < -LIMIT ||
			 rect.lx >  LIMIT ||
			 rect.hy < -LIMIT ||
			 rect.ly >  LIMIT )
			fprintf( sc->stream, "****: %.16f,%.16f,%.16f,%.16f\n", rect.lx, rect.hx, rect.ly, rect.hy );
//		else
//			fprintf( sc->stream, "RECT: %.16f,%.16f,%.16f,%.16f\n", rect.lx, rect.hx, rect.ly, rect.hy );
		sc->skip += 1;
	}
#endif
	return 0;

//	// COPY THE FIRST count POINTS INSIDE RECT
//	Point* outer = out_points;
//	Point* point = sc->points;
//	for ( int i = 0; i < sc->size; point++, i++ ) {
//		if ( point->x < rect.lx || point->x > rect.hx ||
//			 point->y < rect.ly || point->y > rect.hy )
//			continue;
////		if ( point->rank >= 0 )
//			*outer = *point;
//		outer += 1;
//		if ( (outer - out_points) >= count )
//			break;
//	}
//	return (int32_t)(outer - out_points);
}

extern "C" __declspec(dllexport) SearchContext* destroy(SearchContext* sc)
{
#if EXPERIMEMT_DUMP == 5
	if ( sc && sc->stream ) {
		fprintf( sc->stream, "REPORT- total:%d skipped:%d\n", sc->total, sc->skip );
		fclose(sc->stream);
		sc->stream = NULL;
	}
#endif

	if ( sc ) {
		if ( sc->points ) free( sc->points );
		delete sc;
		sc = NULL;
	}
	return sc;
}
