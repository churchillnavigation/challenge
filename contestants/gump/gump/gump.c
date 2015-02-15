#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gump.h"

#define DEBUG 1
#define MAXRESLEN 20
#define SIMPLELIMIT 1000
#define MINRANGE 100000
#define BASELIMIT 10000
#define NODEMULTFACTOR 1000
#define LEAFMULTFACTOR 100

// DEBUGGING --------------------------------------------------------------------------------------

void printRect(Rect rect) {
	printf("[%f, %f, %f, %f]\n", rect.lx, rect.hx, rect.ly, rect.hy);
}

void printPoints(Point* points, int n) {
	for (int i = 0; i < n; i++) {
		printf("%d, %d, %f, %f\n", points[i].id, points[i].rank, points[i].x, points[i].y);
	}
}



// SORT ROUTINES ----------------------------------------------------------------------------------

inline int xcomp(const void* a, const void* b) {
	float diff = ((Point*)a)->x - ((Point*)b)->x;
	return diff > 0 ? 1 : diff < 0 ? -1 : 0;
}

inline int ycomp(const void* a, const void* b) {
	float diff = ((Point*)a)->y - ((Point*)b)->y;
	return diff > 0 ? 1 : diff < 0 ? -1 : 0;
}

inline int rankcomp(const void* a, const void* b) {
	return ((Point*)a)->rank - ((Point*)b)->rank;
}



// HELPER FUNCTIONS -------------------------------------------------------------------------------

int hitchecks = 0;
int totalhitchecks = 0;

inline bool isHit(const Rect* r, Point* p) {
	// hitchecks++;
	return p->x >= r->lx && p->x <= r->hx && p->y >= r->ly && p->y <= r->hy;
}

inline bool isHitX(const Rect* r, Point* p) {
	// hitchecks++;
	return p->x >= r->lx && p->x <= r->hx;
}

inline bool isHitY(const Rect* r, Point* p) {
	// hitchecks++;
	return p->y >= r->ly && p->y <= r->hy;
}

int bsearch(Point p[], bool xOrY, bool minOrMax, float v, int imin, int imax) {
	while (imax >= imin) {
		int imid = (imin + imax) / 2;
		float val = xOrY ? p[imid].x : p[imid].y;
		if (val == v) {
			if (minOrMax) {
				while (imid > imin && (xOrY ? p[imid-1].x : p[imid-1].y) == v) imid--;
				return imid;
			} else {
				while (imid < imax && (xOrY ? p[imid+1].x : p[imid+1].y) == v) imid++;
				return imid;
			}
		}
		else if (val < v) imin = imid + 1;
		else imax = imid - 1;
	}
	return minOrMax ? imin : imax;
}

int32_t findHitsUX(const Rect* rect, Point* in, int n, Point* out, int count) {
	int i = 0;
	int hits = 0;

	// if fewer points in test buffer than allowed hits, use all hits
	if (n <= count) {
		for (int i = 0; i < n; i++) {
			Point p = in[i];
			if (p.x >= rect->lx && p.x <= rect->hx) {
				out[hits] = p;
				hits++;
			}
		}
		qsort(out, hits, sizeof(Point), rankcomp);
		return hits;
	}

	int j = 0;
	int max = -1;
	int maxloc = -1;

	// start by filling out with the first count hits from in
	while (i < n && hits < count) {
		Point p = in[i];
		if (p.x >= rect->lx && p.x <= rect->hx) {
			out[hits] = p;
			if (p.rank > max) {
				max = p.rank;
				maxloc = hits;
			}
			hits++;
		}
		i++;
	}

	// search through the remaining points in in
	while (i < n) {
		Point p = in[i];
		if (p.rank > max) {
			i++;
			continue;
		}

		if (p.x >= rect->lx && p.x <= rect->hx) {
			// replace previous max with this point
			out[maxloc] = p;

			// find new max
			max = -1;
			maxloc = -1;
			for (j = 0; j < count; j++) {
				if (out[j].rank > max) {
					max = out[j].rank;
					maxloc = j;
				}
			}
		}
		i++;
	}

	qsort(out, hits, sizeof(Point), rankcomp);
	return hits;
}

int32_t findHitsUY(const Rect* rect, Point* in, int n, Point* out, int count) {
	int i = 0;
	int hits = 0;

	// if fewer points in test buffer than allowed hits, use all hits
	if (n <= count) {
		for (int i = 0; i < n; i++) {
			Point p = in[i];
			if (p.y >= rect->ly && p.y <= rect->hy) {
				out[hits] = p;
				hits++;
			}
		}
		qsort(out, hits, sizeof(Point), rankcomp);
		return hits;
	}

	int j = 0;
	int max = -1;
	int maxloc = -1;

	// start by filling out with the first count hits from in
	while (i < n && hits < count) {
		Point p = in[i];
		if (p.y >= rect->ly && p.y <= rect->hy) {
			out[hits] = p;
			if (p.rank > max) {
				max = p.rank;
				maxloc = hits;
			}
			hits++;
		}
		i++;
	}

	// search through the remaining points in in
	while (i < n) {
		Point p = in[i];
		if (p.rank > max) {
			i++;
			continue;
		}

		if (p.y >= rect->ly && p.y <= rect->hy) {
			// replace previous max with this point
			out[maxloc] = p;

			// find new max
			max = -1;
			maxloc = -1;
			for (j = 0; j < count; j++) {
				if (out[j].rank > max) {
					max = out[j].rank;
					maxloc = j;
				}
			}
		}
		i++;
	}

	qsort(out, hits, sizeof(Point), rankcomp);
	return hits;
}

int32_t findHitsS(const Rect* rect, Point* in, int n, Point* out, int count) {
	int32_t k = 0;
	int i = 0;
	while (k < count && i < n) {
		Point p = in[i];
		if (p.x >= rect->lx && p.x <= rect->hx && p.y >= rect->ly && p.y <= rect->hy) {
			out[k] = p;
			k++;
		}
		i++;
	}

	return k;
}



// SEARCH IMPLEMENTATIONS -------------------------------------------------------------------------

// baseline search - search full list of points, sorted by rank increasing
int32_t searchBaseline(GumpSearchContext* sc, const Rect rect, const int32_t count, Point* out_points) {
	return findHitsS(&rect, sc->ranksort, sc->N, out_points, count);
}


// binary search - narrow search to points in x range, y range, and check smaller set
int32_t searchBinary(GumpSearchContext* sc, const Rect rect, const int32_t count, Point* out_points) {
	int32_t n = 0;
	int xidxl = bsearch(sc->xsort, true, true, rect.lx, 0, sc->N);
	int xidxr = bsearch(sc->xsort, true, false, rect.hx, 0, sc->N);
	int yidxl = bsearch(sc->ysort, false, true, rect.ly, 0, sc->N);
	int yidxr = bsearch(sc->ysort, false, false, rect.hy, 0, sc->N);
	int nx = xidxr - xidxl + 1;
	int ny = yidxr - yidxl + 1;

	if (nx == 0 || ny == 0) return 0;

	if ((nx < ny ? nx : ny) > BASELIMIT) return searchBaseline(sc, rect, count, out_points);

	if (nx < ny) return findHitsUY(&rect, &sc->xsort[xidxl], nx, out_points, count);
	else return findHitsUX(&rect, &sc->ysort[yidxl], ny, out_points, count);
}


// ranked binary - narrow search to points in x range, y range, and check smaller set using range data structure
int32_t rangeHits(GumpSearchContext* sc, const Rect rect, Range* range, int left, int right, int count, Point* out_points) {
	if (range->left  && range->left->l  <= left && range->left->r  >= right) return rangeHits(sc, rect, range->left,  left, right, count, out_points);
	if (range->right && range->right->l <= left && range->right->r >= right) return rangeHits(sc, rect, range->right, left, right, count, out_points);
	if (range->mid   && range->mid->l   <= left && range->mid->r   >= right) return rangeHits(sc, rect, range->mid,   left, right, count, out_points);

	int len = range->mid ? NODEMULTFACTOR*MAXRESLEN : LEAFMULTFACTOR*MAXRESLEN;
	int hits = findHitsS(&rect, range->ranksort, len, out_points, count);
	//if (hits < count) { if (range->mid) printf("NODE\n"); else printf("LEAF\n"); return -1; }
	if (hits < count) return -1;
	return hits;
}

int32_t searchRankBinary(GumpSearchContext* sc, const Rect rect, const int32_t count, Point* out_points) {
	int xidxl = bsearch(sc->xsort, true, true, rect.lx, 0, sc->N);
	int xidxr = bsearch(sc->xsort, true, false, rect.hx, 0, sc->N);
	int yidxl = bsearch(sc->ysort, false, true, rect.ly, 0, sc->N);
	int yidxr = bsearch(sc->ysort, false, false, rect.hy, 0, sc->N);
	int nx = xidxr - xidxl + 1;
	int ny = yidxr - yidxl + 1;

	if (nx == 0 || ny == 0) return 0;

	// hitchecks = 0;

	// use simple binary if small range
	if ((nx < ny ? nx : ny) < SIMPLELIMIT) {
		if (nx < ny) return findHitsUY(&rect, &sc->xsort[xidxl], nx, out_points, count);
		else return findHitsUX(&rect, &sc->ysort[yidxl], ny, out_points, count);
	}

	int hits = 0;
	if (nx < ny) hits = rangeHits(sc, rect, sc->xroot, xidxl, xidxr, count, out_points);
	else hits = rangeHits(sc, rect, sc->yroot, yidxl, yidxr, count, out_points);

	if (hits == -1) {
		if (nx < ny) hits = findHitsUY(&rect, &sc->xsort[xidxl], nx, out_points, count);
		else hits = findHitsUX(&rect, &sc->ysort[yidxl], ny, out_points, count);
		// printf("Rect was %f,%f,%f,%f (%d,%d,%d,%d)\n",
		// 	rect.lx, rect.hx, rect.ly, rect.hy,
		// 	xidxl, xidxr, yidxl, yidxr
		// );
		// printf("%d, %d, %d, %d - %d checks (%d total)\n", xidxl, xidxr, yidxl, yidxr, hitchecks, totalhitchecks);
	}

	// totalhitchecks += hitchecks;

	return hits;
}



// DLL IMPLEMENTATION -----------------------------------------------------------------------------

int ranges = 0;

float selx(Point* p) { return p->x; }
float sely(Point* p) { return p->y; }

Range* buildRange(GumpSearchContext* sc, int l, int r, float (*sel)(Point* p), bool ismid, bool xOrY) {
	ranges++;
	Range* range = (Range*)malloc(sizeof(Range));
	range->l = l;
	range->r = r;
	range->left = NULL;
	range->right = NULL;
	range->mid = NULL;
	range->ranksort = NULL;

	int n = r - l + 1;
	int len = n < MINRANGE ? LEAFMULTFACTOR*MAXRESLEN : NODEMULTFACTOR*MAXRESLEN;

	range->ranksort = (Point*)calloc(len, sizeof(Point));
	const Rect rect = {
		.lx = xOrY ? sc->xsort[l].x : sc->xsort[0].x,
		.ly = xOrY ? sc->ysort[0].y : sc->ysort[l].y,
		.hx = xOrY ? sc->xsort[r].x : sc->xsort[sc->N-1].x,
		.hy = xOrY ? sc->ysort[sc->N-1].y : sc->ysort[r].y
	};
	searchBinary(sc, rect, len, range->ranksort);

	// is this a leaf
	if (n < MINRANGE) return range;

	// don't split on same val
	Point* sort = xOrY ? sc->xsort : sc->ysort;

	int med = (l + r) / 2;
	float cur = sel(&sort[med]);
	float next = sel(&sort[med+1]);
	while (cur == next) {
		med++;
		cur = sel(&sort[med]);
		next = sel(&sort[med+1]);
	}

	int q1 = (l + med) / 2;
	cur = sel(&sort[q1]);
	next = sel(&sort[q1+1]);
	while (cur == next) {
		q1++;
		cur = sel(&sort[q1]);
		next = sel(&sort[q1+1]);
	}

	int q3 = (med + r) / 2;
	cur = sel(&sort[med]);
	next = sel(&sort[med+1]);
	while (cur == next) {
		q3++;
		cur = sel(&sort[q3]);
		next = sel(&sort[q3+1]);
	}

	if (!ismid) {
		range->left  = buildRange(sc, l, med, sel, false, xOrY);
		range->right = buildRange(sc, med, r, sel, false, xOrY);
	}
	range->mid = buildRange(sc, q1, q3, sel, true, xOrY);

	return range;
}

void freeRange(Range* range) {
	if (range->left)     freeRange(range->left);
	if (range->right)    freeRange(range->right);
	if (range->mid)      freeRange(range->mid);
	if (range->ranksort) free(range->ranksort);
	free(range);
}

__stdcall SearchContext* create(const Point* points_begin, const Point* points_end) {
	GumpSearchContext* sc = (GumpSearchContext*)malloc(sizeof(GumpSearchContext));
	sc->N = points_end - points_begin;
	sc->xsort = (Point*)calloc(sc->N, sizeof(Point));
	sc->ysort = (Point*)calloc(sc->N, sizeof(Point));
	sc->ranksort = (Point*)calloc(sc->N, sizeof(Point));
	memcpy(sc->xsort, points_begin, sc->N * sizeof(Point));
	memcpy(sc->ysort, points_begin, sc->N * sizeof(Point));
	memcpy(sc->ranksort, points_begin, sc->N * sizeof(Point));
	qsort(sc->xsort, sc->N, sizeof(Point), xcomp);
	qsort(sc->ysort, sc->N, sizeof(Point), ycomp);
	qsort(sc->ranksort, sc->N, sizeof(Point), rankcomp);

	sc->xroot = buildRange(sc, 0, sc->N-1, selx, false, true);
	sc->yroot = buildRange(sc, 0, sc->N-1, sely, false, false);

	// printf("\nBuilt %d ranges\n", ranges);
	free(sc->ranksort);

	return (SearchContext*)sc;
}

__stdcall int32_t search(SearchContext* sc, const Rect rect, const int32_t count, Point* out_points) {
	GumpSearchContext* context = (GumpSearchContext*)sc;
	return searchRankBinary(context, rect, count, out_points);
}

__stdcall SearchContext* destroy(SearchContext* sc) {
	GumpSearchContext* context = (GumpSearchContext*)sc;
	free(context->xsort);
	free(context->ysort);
	freeRange(context->xroot);
	freeRange(context->yroot);
	return NULL;
}