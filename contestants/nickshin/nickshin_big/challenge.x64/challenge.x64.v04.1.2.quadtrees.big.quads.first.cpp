/* NICK SHIN
 * nick.shin@gmail.com
 * Written: Dec 2014
 * License: public domain
 *
 * For: http://churchillnavigation.com/challenge/
 */


/*___________________________________________________________________________________________________________________________*/
/* Windows x64 {{{ */
/*___________________________________________________________________________________________________________________________*/

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
/* Windows x64 }}} */
/* Churchill Navigation Challenge {{{ */
/*___________________________________________________________________________________________________________________________*/


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
extern "C" __declspec(dllexport) SearchContext* destroy(SearchContext* sc);


/*___________________________________________________________________________________________________________________________*/
/* Churchill Navigation Challenge }}} */
/* NICK SHIN {{{ */
/*___________________________________________________________________________________________________________________________*/

/* CHANGELIST:
 * v04.1.2: CHALLENGE2 - QUADTREE (oops) BIG QUADS FIRST
 * v04.1.1: CHALLENGE2 - QUADTREE MEMORY OPTIMIZATIONS
 * v04.1.0: CHALLENGE2 - QUADTREE
 * v04.0.1: EXPERIMENT - CLEANED UP
 * v04.0.0: EXPERIMENT - DUMP POINTS AND RECT FOR INSPECTION
 * v03.1.1: EXPERIMENT - USING STL CONTAINERS
 * v03.1.0: CHALLENGE - CLEANED UP
 * v03.0.0: CHALLENGE - DEVELOPMENT VERSION (inlined check with ref dll)
 * V02: SANITY CHECK THE RUN WITH REFERENCE DLL
 * V01: SANITY CHECK THE RUN WITH BARE BONES STUBBED FUNCTIONS
 */

/* NOTES: v4.1.1
 * the reason for the memory optimizations was to see if this will help with
 * memory cache thrashing.  side note: this version (v4.1.1) uses an average
 * of 23% less ram (than v4.1.0).  stay tuned for v4.2.0 (parallelization)...
 */


#include <stdlib.h> // NULL malloc free
#include <algorithm> // sort
#include <vector>
#include <shellapi.h> // CommandLineToArgvW
#include <math.h> // pow


#define LIMIT 10000.0f
#if METRICS_TESTING
int MAX_QUADTREE_LEVEL = 7;
#else
#define MAX_QUADTREE_LEVEL  7
#endif


typedef std::vector<const Point*> sorted;
class QuadTree
{
	// housekeeping
	Rect box;
	int level;
	sorted points;
	Point* ranks;
	size_t ranks_count;

	// quads
	enum QUADTREE {
		NW = 0,
		NE,
		SW,
		SE,
		QUADTREES
	};
	union {
		struct {
			QuadTree* nw;
			QuadTree* ne;
			QuadTree* sw;
			QuadTree* se;
		};
		QuadTree* quadtrees[QUADTREES];
	};

public:
	QuadTree();
	QuadTree(float lx, float ly, float hx, float hy, int level, int count);
	~QuadTree();
	// create()
	bool Insert(const Point* p);
	void sort();
	// search()
	void GetPoints(Point* out_points, SearchContext* sc, const Rect* range, int32_t count);
	inline bool Check(Point* out_points, SearchContext* sc, const Rect* range, int32_t count);
};


struct SearchContext
{
	int size;
	Rect box;
	// ------
	QuadTree* quadtree;
	// local cache
	int32_t rank_count;
	int32_t rank_max;
	// helper
#if METRICS_TESTING
	// in order to allow the use of MAX_QUADTREE_LEVEL during
	// metrics testing, this triple pointer variable is used... ^_^
	QuadTree*** qt;
	int* qt_len;
#else
	QuadTree** qt[MAX_QUADTREE_LEVEL];
	int qt_len[MAX_QUADTREE_LEVEL];
#endif
};

bool compare_sort( Point first, Point second );


/* ............................................................ */
/* DLL {{{2 */
/* ............................................................ */


extern "C" __declspec(dllexport) SearchContext* create(const Point* points_begin, const Point* points_end)
{
	SearchContext* sc;
	const Point* p;
	int i, size;

	// SANITY CHECKS
	if ( ! points_begin || ! points_end )
		return NULL;

	// CALCULATE SIZE
	size = (int)(points_end - points_begin);
	if ( ! size )
		return NULL;

	// ----------------------------------------
	// ARGs
//	int points = 10000000;
	int count = 20;

		// WARNING: WINDOWS CENTRIC
		LPTSTR lpCmdLine = GetCommandLine();
		int pNumArgs;
		LPWSTR* szArglist = CommandLineToArgvW( lpCmdLine, &pNumArgs );
		char mbstr[1024];
		for( i=0; i<pNumArgs; i++) {
			LPWSTR s = szArglist[i];
			size_t value;
			wcstombs_s( &value, mbstr, s, sizeof(mbstr) );
			sscanf_s( mbstr, "-r%d", &count );
//			sscanf_s( mbstr, "-p%d", &points );
#if METRICS_TESTING
			sscanf_s( mbstr, "-t%d", &MAX_QUADTREE_LEVEL );
#endif
		}
		LocalFree(szArglist);

	// ----------------------------------------

	// ALLOCATE OBJECT
	sc = new SearchContext;
	if ( ! sc )
		return NULL;
	memset( sc, 0, sizeof( SearchContext ) );
	sc->size = size;


	// ALLOWED TO CHEW UP 512MB OF RAM AND TAKE AS LONG AS 30 SECONDS TO RUN


	// in the meantime, find some basic bounds...
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
#if 0 // to be "complete" the following should be done...
		if( p->x < -LIMIT ||
			p->x >  LIMIT ||
			p->y < -LIMIT ||
			p->y >  LIMIT ) {
			// ADD THIS POINT TO THE '0 level' QuadTree
		}
#endif
	}
	sc->quadtree = new QuadTree( sc->box.lx, sc->box.ly, sc->box.hx, sc->box.hy, 0, count );

	// SAVE POINT DATA
	p = points_begin;
	for ( i = 0; i < size; i++, p++ )
		sc->quadtree->Insert(p);
	sc->quadtree->sort(); // BAKE THE QuadTree DATA

	// HELPER
	i = 0;
	size = 0;
#if METRICS_TESTING
	sc->qt = (QuadTree***)malloc( MAX_QUADTREE_LEVEL * sizeof( QuadTree*** ) );
	sc->qt_len = (int*)malloc( MAX_QUADTREE_LEVEL * sizeof( int ) );
#endif
	for ( int j = 0; j < MAX_QUADTREE_LEVEL; j++ ) {
		i += 2 * (int)pow( 2, j+1 );
		size += i;
		sc->qt[j] = (QuadTree**)malloc( size * sizeof( QuadTree** ) );
	}
	return sc;
}

extern "C" __declspec(dllexport) int32_t search(SearchContext* sc, const Rect rect, const int32_t count, Point* out_points)
{
	// SANITY CHECKS
	if ( ! sc || ! count || ! out_points )
		return 0;

	// RESET QuadTree SEARCH RESULTS AND SCAN AGAINST rect
	sc->rank_count = 0;
	sc->rank_max = sc->size;
	if ( sc->quadtree->Check( out_points, sc, &rect, count ) )
		sc->quadtree->GetPoints( out_points, sc, &rect, count );
	if ( sc->rank_count < count )
		std::sort( out_points, out_points + sc->rank_count, compare_sort);
	return sc->rank_count;
}

extern "C" __declspec(dllexport) SearchContext* destroy(SearchContext* sc)
{
	if ( sc ) {
#if METRICS_TESTING
		if ( sc->qt ) {
#endif
			for ( int j = 0; j < MAX_QUADTREE_LEVEL; j++ ) {
				if ( sc->qt[j] )
					free( sc->qt[j] );
			}
#if METRICS_TESTING
			free( sc->qt );
		}
		if ( sc->qt_len )
			free( sc->qt_len );
#endif
		if ( sc->quadtree ) delete sc->quadtree;
		delete sc;
		sc = NULL;
	}
	return sc;
}


/* ............................................................ */
/* DLL }}}2 */
/* QUADTREE {{{2 */
/* ............................................................ */


// ================================================================================
// ================================================================================

QuadTree::QuadTree()
{
	QuadTree( -LIMIT, -LIMIT, LIMIT, LIMIT, 0, 1 );
}

QuadTree::QuadTree(float lx, float ly, float hx, float hy, int _level, int count)
{
	box.lx = lx;
	box.hx = hx;
	box.ly = ly;
	box.hy = hy;
	level = _level;
	ranks_count = count;
	ranks = (Point*) malloc( count * sizeof(Point) );

	if ( level >= MAX_QUADTREE_LEVEL ) {
		nw = ne = sw = se = NULL;
		return;
	}
	float w = 0.5f * abs(box.hx - box.lx);
	float h = 0.5f * abs(box.hy - box.ly);
	nw = new QuadTree(box.lx,     box.ly,     box.lx + w,     box.ly + h,     level+1, count);
	ne = new QuadTree(box.lx + w, box.ly,     box.lx + w * 2, box.ly + h,     level+1, count);
	sw = new QuadTree(box.lx,     box.ly + h, box.lx + w,     box.ly + h * 2, level+1, count);
	se = new QuadTree(box.lx + w, box.ly + h, box.lx + w * 2, box.ly + h * 2, level+1, count);
}

// --------------------------------------------------------------------------------

QuadTree::~QuadTree()
{
	points.clear();
	if ( ranks ) {
		free( ranks );
		ranks = NULL;
	}
	if ( level >= MAX_QUADTREE_LEVEL )
		return;
	for ( int i = 0; i < QUADTREES; i++ )
		delete quadtrees[i];
}

// ================================================================================
// ================================================================================

bool QuadTree::Insert(const Point* p)
{
	// IS POINT INSIDE THIS QUAD?
	if ( p->x > box.hx || p->x < box.lx || p->y > box.hy || p->y < box.ly )
		return false;

	// DRILL DOWN TO SUB-QUADs
	if ( level < MAX_QUADTREE_LEVEL ) {
		for ( int i = 0; i < QUADTREES; i++ ) {
			if ( quadtrees[i]->Insert(p) )
				return true;
	}	}

	points.push_back(p);
	return true;
}

bool compare_rank( const Point* first, const Point* second )
{
	return ( first->rank < second->rank );
}

bool compare_sort( const Point first, const Point second )
{
	return ( first.rank < second.rank );
}

void QuadTree::sort()
{
	if ( level >= MAX_QUADTREE_LEVEL ) {
		// AT LEAF, RANK ALL POINTS
		std::sort(points.begin(), points.end(), compare_rank);
		ranks_count = points.size();
		if ( ranks ) free( ranks );
		ranks = (Point*) malloc( ranks_count * sizeof(Point) );
		for ( int i = 0; i < ranks_count; i++ )
			*(ranks + i) = *(points[i]);
		points.clear();
		return;
	}

	std::vector<Point*> RANKS;
	for ( int i = 0; i < QUADTREES; i++ ) {
		QuadTree* qt = quadtrees[i];
		qt->sort();
		size_t cap = qt->ranks_count;
		if ( cap > ranks_count )
			cap = ranks_count;
		for ( int j = 0; j < cap; j++ )
			RANKS.push_back( qt->ranks + j );
	}
	std::sort(RANKS.begin(), RANKS.end(), compare_rank);
	size_t count = RANKS.size();
	if ( count > ranks_count )
		count = ranks_count;
	else
		ranks_count = count;
	for ( int i = 0; i < count; i++ )
		*(ranks + i) = *(RANKS[i]);
	RANKS.clear();
}

// ================================================================================
// ================================================================================

inline void add_rank( Point* out_points, SearchContext* sc, Point* p, int32_t count )
{
	// using "offset" to keep things "clear"
	int32_t offset = sc->rank_count;
	if ( offset >= count )
		offset -= 1; // stomp on last index

	*(out_points + offset) = *p;

	offset += 1;
	sc->rank_count = offset;
	if ( offset < count )
		return;

	// if here, start finding 'max'
	std::sort(out_points, out_points + count, compare_sort);
	sc->rank_max = (out_points + (count-1))->rank;
}

void QuadTree::GetPoints(Point* out_points, SearchContext* sc, const Rect* range, int32_t count)
{
	int level = 0;
	int i;

	// CHECK SUBQUADS FIRST
	sc->qt_len[level] = 0;
	for ( i = 0; i < QUADTREES; i++ ) {
		QuadTree** qnext = sc->qt[level];
		QuadTree* qt = sc->quadtree->quadtrees[i];
		if ( qt->Check(out_points, sc, range, count) ) {
			qnext[sc->qt_len[level]] = qt;
			sc->qt_len[level] += 1;
	}	}
	// NOW DRILL DOWN...
	for ( level = 0; level < (MAX_QUADTREE_LEVEL-1); level++ ) {
		int next = level + 1;
		sc->qt_len[next] = 0;
		QuadTree** qnext = sc->qt[next];
		QuadTree** q = sc->qt[level];
		for ( i = 0; i < sc->qt_len[level]; i++ ) {
			QuadTree* qq = q[i];
			for ( int j = 0; j < QUADTREES; j++ ) {
				QuadTree* qt = qq->quadtrees[j];
				if ( qt->Check(out_points, sc, range, count) ) {
					sc->qt[next][sc->qt_len[next]] = qt;
					sc->qt_len[next] += 1;
	}	}	}	}
}

inline bool QuadTree::Check(Point* out_points, SearchContext* sc, const Rect* range, int32_t count)
{
	int i;

	// SHORT CIRCUIT - means: the rest of ranks* is greater than current "max" -- so done...

	if ( ranks->rank > sc->rank_max || // SHORT CIRCUIT
		// DOES RANGE INTERSECT QUAD?
		box.lx > range->hx || box.hx < range->lx || box.ly > range->hy || box.hy < range->ly )
		return false;

	// IF WHOLE BOX IS INSIDE RANGE, RETURN count AT THIS QUAD LEVEL
	size_t size = ranks_count;
	if ( box.lx >= range->lx && box.hx <= range->hx && box.ly >= range->ly && box.hy <= range->hy ) {
		if ( size > count )
			size = count;
		for (i = 0; i < size; i++) {
			Point* p = ranks + i;
			if ( p->rank > sc->rank_max )
				break; // SHORT CIRCUIT
			add_rank( out_points, sc, p, count );
		}
		return false;
	}

	// IF LEAF - MUST DRILL DOWN WHOLE LIST
	if ( level >= MAX_QUADTREE_LEVEL ) {
		for (i = 0; i < size; i++) {
			Point* p = ranks + i;
			if ( p->rank > sc->rank_max )
				break; // SHORT CIRCUIT
			if ( p->x >= range->lx && p->x <= range->hx && p->y >= range->ly && p->y <= range->hy )
				add_rank( out_points, sc, p, count );
		}
		return false;
	}
	return true;
}


/*___________________________________________________________________________________________________________________________*/
/* QUADTREE }}}2 */
/* NICK SHIN }}} */
/*___________________________________________________________________________________________________________________________*/
