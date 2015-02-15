// darkhorse.cpp : Defines the exported functions for the DLL application.
//

// http://churchillnavigation.com/challenge/
// https://www.reddit.com/r/programming/comments/2s0rv3/5k_for_the_fastest_code_2d_ranked_point_search/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <emmintrin.h>

#include "point_search.h"

// not packed, for better alignment over Point
typedef struct NewPoint {
	uint32_t quad_idx[4];
	float x, y;
	int8_t id;
} NewPoint;

/* Quadrant indices
     -y
    0 | 1
-x ---+--- +x
    2 | 3
     +y    */


#define BSIZE 16

struct SearchContext {
	// quadtree, with lower ranks nearer to the trunk of the tree
	// each node's rank is lower than all sub-nodes
	NewPoint *p;
	uint32_t *heap;
	uint32_t heap_cnt;
	uint32_t head[BSIZE][BSIZE]; // bucket head index
	unsigned char headinit[BSIZE][BSIZE]; // bucket heads init
	float medx[BSIZE + 1], medy[BSIZE + 1]; // median values
};

static unsigned int bucket(float x, float *v)
{
	unsigned int i = 8;
	i += (x < v[i]) ? -4 : 4;
	i += (x < v[i]) ? -2 : 2;
	i += (x < v[i]) ? -1 : 1;
	i += (x < v[i]) ? -1 : 0;
	return i;
}


static int compare_float(const void *a, const void *b)
{
	float i = *(const float*)a;
	float j = *(const float*)b;
	return (i > j) - (i < j);
}

static uint32_t *next_node(NewPoint *p, const NewPoint *node)
{
	return &p->quad_idx[(node->x > p->x) + 2 * (node->y > p->y)];
}



/* Load the provided points into an internal data structure. The pointers follow the STL iterator convention, where
"points_begin" points to the first element, and "points_end" points to one past the last element. The input points are
only guaranteed to be valid for the duration of the call. Return a pointer to the context that can be used for
consecutive searches on the data. */
extern "C" __declspec(dllexport) 
SearchContext* __stdcall create(const Point* points_begin, const Point* points_end)
{
	SearchContext *sc;
	int32_t count = (int32_t)(points_end - points_begin);
	NewPoint *p;
	char sorted = 1;
	int32_t i, rank = 0;

	// no points to process
	if (count == 0) return NULL;

	sc = (SearchContext*)calloc(1, sizeof(*sc));
	if (!sc) return NULL;

	//printf("\ncomputing buckets..."); fflush(stdout);
	float *fx = (float*)calloc(count, sizeof(*fx));
	float *fy = (float*)calloc(count, sizeof(*fy));
	for (i = 0; i < count; i++) {
		fx[i] = points_begin[i].x;
		fy[i] = points_begin[i].y;
	}
	qsort(fx, count, sizeof(*fx), compare_float);
	qsort(fy, count, sizeof(*fy), compare_float);
	// calculate median values for buckets
	for (i = 0; i <= BSIZE; i++) {
		sc->medx[i] = fx[i*(count - 1) / BSIZE];
		sc->medy[i] = fy[i*(count - 1) / BSIZE];
	}
	free(fx);
	free(fy);

	// create arrays
	p = (NewPoint*)calloc(count, sizeof(*p));
	if (!p) { free(sc); return NULL; }
	sc->p = p;
	sc->heap_cnt = 4096;
	sc->heap = (uint32_t*)malloc(sc->heap_cnt * sizeof(*sc->heap));


	//printf("\ncopying..."); fflush(stdout);
	// copy items into aligned array
	for (i = 0; i < count; i++) {
		rank = points_begin[i].rank;
		p[rank].x = points_begin[i].x;
		p[rank].y = points_begin[i].y;
		p[rank].id = points_begin[i].id;
	}

	//printf("\ninserting..."); fflush(stdout);
	// insert points into quadtree by rank
	for (i = 0; i <= count; i++) {
		unsigned int x = bucket(p[i].x, sc->medx);
		unsigned int y = bucket(p[i].y, sc->medy);
		if (!sc->headinit[y][x]) {
			sc->headinit[y][x] = 1;
			sc->head[y][x] = i;
		} else {
			uint32_t *next = next_node(p + sc->head[y][x], p + i);
			while (*next) {
				next = next_node(p + *next, p + i);
			}
			*next = i;
		}
	}

	return sc;
}



/* binary heap
 * root is lowest ranked item
 * each item is:
 *   index of point (into SearchContext::p[]) (also its rank)
 */

#define heap_parent(idx) (((idx) - 1) >> 1)
#define heap_sibling(idx) ((((idx) - 1) ^ 1) + 1)
#define heap_left_child(idx) ((idx) * 2 + 1)
#define heap_right_child(idx) ((idx) * 2 + 2)

// insert a point into the heap by its index
static void heap_insert(
	uint32_t *__restrict heap,
	uint32_t len,
	uint32_t idx)
{
	uint32_t i, parent;

	for (i = len; i > 0; i = parent) {
		parent = heap_parent(i);
		if (idx > heap[parent])
			break;
		heap[i] = heap[parent];
	}
	heap[i] = idx;
}

// remove the root item from the heap
static void __forceinline heap_remove(
	uint32_t *__restrict heap,
	uint32_t len)
{
	uint32_t i = 0;
	uint32_t idx = heap[--len]; // take off the last item
	uint32_t l = heap_left_child(i);

	while (l + 1 < len) {
		int64_t l_idx = heap[l];
		int64_t r_idx = heap[l + 1];

		if (idx <= l_idx && idx <= r_idx) {
			// heap is stable
			heap[i] = idx;
			return;
		}
		// swap left or right node up, whichever is smallest
		l += r_idx < l_idx;
		heap[i] = heap[l];
		i = l;
		l = heap_left_child(l);
	}
	if (l < len && idx > heap[l]) {
		// swap left node up
		heap[i] = heap[l];
		heap[l] = idx;
	} else {
		heap[i] = idx;
	}
}

/* Search for "count" points with the smallest ranks inside "rect" and copy them ordered by smallest rank first in
"out_points". Return the number of points copied. "out_points" points to a buffer owned by the caller that
can hold "count" number of Points. */
extern "C" __declspec(dllexport) 
int32_t __stdcall search(SearchContext* sc, const Rect rect, const int32_t count, Point* out_points)
{
	int32_t found = 0;

	// robustness checks
	if (sc == NULL ||
		rect.lx > rect.hx || rect.ly > rect.hy ||  // invalid rect?
		count == 0)
		return 0;

	const NewPoint *points = sc->p;
	unsigned int lx = bucket(rect.lx, sc->medx);
	unsigned int ly = bucket(rect.ly, sc->medy);
	unsigned int hx = bucket(rect.hx, sc->medx);
	unsigned int hy = bucket(rect.hy, sc->medy);

	uint32_t heap_len = 0;
	uint32_t *heap = sc->heap;
	uint32_t x, y;

	// fill the binary heap with head of each bucket
	for (y = ly; y <= hy; y++) {
		for (x = lx; x <= hx; x++) {
			if (sc->headinit[y][x]) {
				heap_insert(heap, heap_len++, sc->head[y][x]);
			}
		}
	}

	// search loop
	while (heap_len) {
		// grab the point at the top of the heap (lowest ranked)
		uint32_t rank = heap[0];
		const NewPoint *p = points + (uint32_t)rank;

		// get the quadtree index for the next points
		uint32_t i = p->quad_idx[0], j = p->quad_idx[1], k = p->quad_idx[2], l = p->quad_idx[3];

		// memory accesses are erratic, so prefetching helps
		_mm_prefetch((char*)(points + i), _MM_HINT_T0);
		_mm_prefetch((char*)(points + k), _MM_HINT_T0);
		_mm_prefetch((char*)(points + j), _MM_HINT_T0);
		_mm_prefetch((char*)(points + l), _MM_HINT_T0);

		// grow heap as needed
		if (heap_len + 4 > sc->heap_cnt) {
			sc->heap_cnt *= 2;
			heap = (uint32_t*)realloc(heap, sc->heap_cnt * sizeof(*heap));
			sc->heap = heap;
		}

		// remove heap root
		heap_remove(heap, heap_len--);

		if (rect.lx <= p->x) { // rect left of point
			if (rect.ly <= p->y) { // rect above point
				if (rect.hx >= p->x && rect.hy >= p->y) { // rect right and below point
					// add it to results
					out_points[found].rank = rank;
					//out_points[found].x = p->x;
					//out_points[found].y = p->y;
					out_points[found].id = p->id;
					if (++found == count)
						return found;
				}
				if (i) heap_insert(heap, heap_len++, i);
			}
			if (rect.hy > p->y && k) { // rect below point
				heap_insert(heap, heap_len++, k);
			}
		}
		if (rect.hx > p->x) { // rect right of point
			if (rect.ly <= p->y && j) { // rect above point
				heap_insert(heap, heap_len++, j);
			}
			if (rect.hy > p->y && l) { // rect below point
				heap_insert(heap, heap_len++, l);
			}
		}
	}
	return found;
}


/* Release the resources associated with the context. Return nullptr if successful, "sc" otherwise. */
extern "C" __declspec(dllexport) 
SearchContext* __stdcall destroy(SearchContext* sc)
{
	if (sc) {
		free(sc->p);
		free(sc->heap);
		free(sc);
	}
	return NULL; // free always succeeds
}

