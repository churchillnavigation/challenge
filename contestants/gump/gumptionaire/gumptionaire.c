#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "gumptionaire.h"
#include "iqsort.h"

// #define DEBUG 0
#define WRITEFILES 0

#ifdef DEBUG
	#define DPRINT(x) printf x
#else
	#define DPRINT(x) do {} while (0)
#endif

// rank search parameters
#define BASELIMIT 1000000

// region search parameters
#define REGIONTHRESH 0.00011f
#define BLOCKCHECK 5
#define MAXDEPTH 10
#define MAXLEAF 150000
#define NODESIZE 500
#define LEAFSIZE 600

// grid search parameters
#define DIVS 175
#define GRIDFACTOR 1.0f
#define LINTHRESH1 1000
#define LINTHRESH2 1500
#define LINTHRESH3 15000
#define RANKMAX 100000000

// DEBUGGING --------------------------------------------------------------------------------------

void printRect(Rect rect) {
	DPRINT(("[%f, %f, %f, %f]\n", rect.lx, rect.hx, rect.ly, rect.hy));
}

void printPoints(Point* points, int n) {
	for (int i = 0; i < n; i++) {
		DPRINT(("%d, %d, %f, %f\n", points[i].id, points[i].rank, points[i].x, points[i].y));
	}
}



// SORT ROUTINES ----------------------------------------------------------------------------------

void xsort(struct Point *arr, unsigned n) {
	#define point_x_lt(a,b) ((a)->x < (b)->x)
	QSORT(struct Point, arr, n, point_x_lt);
}

void ysort(struct Point *arr, unsigned n) {
	#define point_y_lt(a,b) ((a)->y < (b)->y)
	QSORT(struct Point, arr, n, point_y_lt);
}

void ranksort(struct Point *arr, unsigned n) {
	#define point_rank_lt(a,b) ((a)->rank < (b)->rank)
	QSORT(struct Point, arr, n, point_rank_lt);
}



// HELPER FUNCTIONS -------------------------------------------------------------------------------

int ops = 0;
int totops = 0;

inline float rectArea(Rect* rect) {
	return (rect->hx - rect->lx) * (rect->hy - rect->ly);
}

inline bool isRectInside(Rect* r1, Rect* r2) {
	return r2->lx >= r1->lx && r2->ly >= r1->ly && r2->hx <= r1->hx && r2->hy <= r1->hy;
}

inline bool isRectOverlap(Rect* r1, Rect* r2) {
	return r1->lx <= r2->hx && r1->hx >= r2->lx && r1->ly <= r2->hy && r1->hy >= r2->ly;
}

inline float rectOverlapPercent(Rect* r1, Rect* r2) {
	float lx = r1->lx > r2->lx ? r1->lx : r2->lx;
	float hx = r1->hx < r2->hx ? r1->hx : r2->hx;
	float ly = r1->ly > r2->ly ? r1->ly : r2->ly;
	float hy = r1->hy < r2->hy ? r1->hy : r2->hy;
	return ((hx - lx) * (hy - ly)) / rectArea(r1);
}

inline bool isHit(Rect* r, Point* p) {
	return p->x >= r->lx && p->x <= r->hx && p->y >= r->ly && p->y <= r->hy;
}

inline bool isHitX(Rect* r, Point* p) {
	return p->x >= r->lx && p->x <= r->hx;
}

inline bool isHitY(Rect* r, Point* p) {
	return p->y >= r->ly && p->y <= r->hy;
}

int bsearchx(Point p[], bool minOrMax, float v, int imin, int imax) {
	while (imax >= imin) {
		int imid = (imin + imax) / 2;
		float val = p[imid].x;
		if (val == v) {
			if (minOrMax) {
				while (imid > imin && p[imid-1].x == v) imid--;
				return imid;
			} else {
				while (imid < imax && p[imid+1].x == v) imid++;
				return imid;
			}
		}
		else if (val < v) imin = imid + 1;
		else imax = imid - 1;
	}
	return minOrMax ? imin : imax;
}

int bsearchy(Point p[], bool minOrMax, float v, int imin, int imax) {
	while (imax >= imin) {
		int imid = (imin + imax) / 2;
		float val = p[imid].y;
		if (val == v) {
			if (minOrMax) {
				while (imid > imin && p[imid-1].y == v) imid--;
				return imid;
			} else {
				while (imid < imax && p[imid+1].y == v) imid++;
				return imid;
			}
		}
		else if (val < v) imin = imid + 1;
		else imax = imid - 1;
	}
	return minOrMax ? imin : imax;
}

int bvalsearch(float* restrict p, bool minOrMax, float v, int imin, int imax) {
	while (imax >= imin) {
		int imid = (imin + imax) >> 1;
		float val = p[imid];
		if (val == v) {
			if (minOrMax) {
				while (imid > imin && p[imid-1] == v) imid--;
				return imid;
			} else {
				while (imid < imax && p[imid+1] == v) imid++;
				return imid;
			}
		}
		else if (val < v) imin = imid + 1;
		else imax = imid - 1;
	}
	return minOrMax ? imin : imax;
}

int32_t findHitsU(Rect* rect, Point* in, int n, Point* out, int count, bool (*hitcheck)(Rect* r, Point* p)) {
	int i = 0;
	int hits = 0;

	// if fewer points in test buffer than allowed hits, use all hits
	if (n <= count) {
		for (int i = 0; i < n; i++) {
			Point p = in[i];
			if (hitcheck(rect, &p)) {
				out[hits] = p;
				hits++;
			}
		}
		ranksort(out, hits);
		return hits;
	}

	int j = 0;
	int max = -1;
	int maxloc = -1;

	// start by filling out with the first count hits from in
	while (i < n && hits < count) {
		Point p = in[i];
		if (hitcheck(rect, &p)) {
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

		if (hitcheck(rect, &p)) {
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

	ranksort(out, hits);
	return hits;
}

int32_t findHitsUxV(const Rect* rect, int8_t* restrict ids, int32_t* restrict ranks, float* restrict xs, int n, Point* out, int count) {
	int8_t* id    = (int8_t*)__builtin_assume_aligned(ids, 16);
	int32_t* rank = (int32_t*)__builtin_assume_aligned(ranks, 16);
	float* x      = (float*)__builtin_assume_aligned(xs, 16);

	int i = 0;
	int hits = 0;

	// if fewer points in test buffer than allowed hits, use all hits
	if (n <= count) {
		for (int i = 0; i < n; i++) {
			// ops += 2;
			if (x[i] >= rect->lx && x[i] <= rect->hx) {
				out[hits].id = id[i];
				out[hits].rank = rank[i];
				hits++;
			}
		}
		ranksort(out, hits);
		return hits;
	}

	int j = 0;
	int max = -1;
	int maxloc = -1;

	// start by filling out with the first count hits from in
	while (i < n && hits < count) {
		// ops += 2;
		if (x[i] >= rect->lx && x[i] <= rect->hx) {
			out[hits].id = id[i];
			out[hits].rank = rank[i];
			if (rank[i] > max) {
				max = rank[i];
				maxloc = hits;
			}
			hits++;
		}
		i++;
	}

	// search through the remaining points in in
	while (i < n) {
		// ops += 1;
		if (rank[i] > max) {
			i++;
			continue;
		}

		// ops += 2;
		if (x[i] >= rect->lx && x[i] <= rect->hx) {
			// replace previous max with this point
			out[maxloc].id = id[i];
			out[maxloc].rank = rank[i];

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

	ranksort(out, hits);
	return hits;
}

int32_t findHitsUyV(const Rect* rect, int8_t* restrict ids, int32_t* restrict ranks, float* restrict ys, int n, Point* out, int count) {
	int8_t* id    = (int8_t*)__builtin_assume_aligned(ids, 16);
	int32_t* rank = (int32_t*)__builtin_assume_aligned(ranks, 16);
	float* y      = (float*)__builtin_assume_aligned(ys, 16);

	int i = 0;
	int hits = 0;

	// if fewer points in test buffer than allowed hits, use all hits
	if (n <= count) {
		for (int i = 0; i < n; i++) {
			if (y[i] >= rect->ly && y[i] <= rect->hy) {
				out[hits].id = id[i];
				out[hits].rank = rank[i];
				hits++;
			}
		}
		ranksort(out, hits);
		return hits;
	}

	int j = 0;
	int max = -1;
	int maxloc = -1;

	// start by filling out with the first count hits from in
	while (i < n && hits < count) {
		if (y[i] >= rect->ly && y[i] <= rect->hy) {
			out[hits].id = id[i];
			out[hits].rank = rank[i];
			if (rank[i] > max) {
				max = rank[i];
				maxloc = hits;
			}
			hits++;
		}
		i++;
	}

	// search through the remaining points in in
	while (i < n) {
		if (rank[i] > max) {
			i++;
			continue;
		}

		if (y[i] >= rect->ly && y[i] <= rect->hy) {
			// replace previous max with this point
			out[maxloc].id = id[i];
			out[maxloc].rank = rank[i];

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

	ranksort(out, hits);
	return hits;
}

int32_t findHitsS(const Rect* rect, Point* in, int n, Point* out, int count) {
	int32_t k = 0;
	int i = 0;
	while (i < n) {
		Point p = in[i];
		if (p.x >= rect->lx && p.x <= rect->hx && p.y >= rect->ly && p.y <= rect->hy) {
			out[k] = p;
			k++;
			if (k == count) return k;
		}
		i++;
	}
	return k;
}

int32_t findHitsSV(const Rect* rect, int8_t* restrict ids, int32_t* restrict ranks, float* restrict xs, float* restrict ys, int n, Point* out, int count) {
	int8_t* id    = (int8_t*)__builtin_assume_aligned(ids, 16);
	int32_t* rank = (int32_t*)__builtin_assume_aligned(ranks, 16);
	float* x      = (float*)__builtin_assume_aligned(xs, 16);
	float* y      = (float*)__builtin_assume_aligned(ys, 16);
	int32_t k = 0;
	for (int i = 0; i < n; i++) {
		if (x[i] >= rect->lx && x[i] <= rect->hx && y[i] >= rect->ly && y[i] <= rect->hy) {
			out[k].id = id[i];
			out[k].rank = rank[i];
			k++;
			if (k == count) return k;
		}
	}
	return k;
}

int32_t findHitsB(const Rect* rect, int b, Point** restrict blocks, int* restrict blocki, int* restrict blockn, Point* out, int count) {
	int* bi = (int*)__builtin_assume_aligned(blocki, 16);
	int* bn = (int*)__builtin_assume_aligned(blockn, 16);

	int32_t k = 0;
	int minrank = RANKMAX;
	int prank = -1;
	int minb = -1;
	int fin = 0;
	while (k < count) {
		minrank = RANKMAX;
		fin = 0;

		// find min rank
		for (int i = 0; i < b; i++) {
			if (bi[i] >= blockn[i]) { fin++; continue; }

			Point p = blocks[i][bi[i]];
			if (p.rank < minrank) {
				if (p.rank == prank) {
					bi[i]++;
					p = blocks[i][bi[i]];
					if (p.rank < minrank) {
						minb = i;
						minrank = p.rank;
					}
				} else {
					minb = i;
					minrank = p.rank;
				}
			}
		}

		// If we've hit the end of all blocks, exit
		if (fin == b) break;

		Point bestp = blocks[minb][bi[minb]];
		if (bestp.x >= rect->lx && bestp.x <= rect->hx && bestp.y >= rect->ly && bestp.y <= rect->hy) {
			out[k] = bestp;
			prank = bestp.rank;
			k++;
		}
		bi[minb]++;
	}

	return k;
}



// SEARCH IMPLEMENTATIONS -------------------------------------------------------------------------

// binary search - narrow search to points in x range, y range, and check smaller set
int32_t searchBinary(GumpSearchContext* sc, const Rect rect, const int32_t count, Point* out_points) {
	int xidxl = bvalsearch(sc->xpoints->x, true, rect.lx, 0, sc->N);
	int xidxr = bvalsearch(sc->xpoints->x, false, rect.hx, 0, sc->N);
	int nx = xidxr - xidxl + 1;
	if (nx == 0) return 0;

	int yidxl = bvalsearch(sc->ypoints->y, true, rect.ly, 0, sc->N);
	int yidxr = bvalsearch(sc->ypoints->y, false, rect.hy, 0, sc->N);
	int ny = yidxr - yidxl + 1;
	if (ny == 0) return 0;

	if ((nx < ny ? nx : ny) > BASELIMIT) return findHitsS((Rect*)&rect, sc->ranksort, sc->N, out_points, count);

	if (nx < ny) return findHitsU((Rect*)&rect, &sc->xsort[xidxl], nx, out_points, count, isHitY);
	else return findHitsU((Rect*)&rect, &sc->ysort[yidxl], ny, out_points, count, isHitX);
}


int32_t regionHits(GumpSearchContext* sc, Rect rect, Region* region, int count, Point* out_points) {
	if (region->n == 0) return 0;

	// if this is a leaf, check it
	if (region->left == NULL) {
		Points* p = region->rankpoints;
		int hits = findHitsSV((Rect*)&rect, p->id, p->rank, p->x, p->y, p->n, out_points, count);
		if (hits < count) return -1;
		return hits;
	}

	// look for a child that fully contains this rect
	if (sc->w < region->subw) {
		if (isRectInside(region->left->rect,   &rect)) return regionHits(sc, rect, region->left,   count, out_points);
		if (isRectInside(region->right->rect,  &rect)) return regionHits(sc, rect, region->right,  count, out_points);
		if (isRectInside(region->lrmid->rect,  &rect)) return regionHits(sc, rect, region->lrmid,  count, out_points);
	}
	if (sc->h < region->subh) {
		if (isRectInside(region->bottom->rect, &rect)) return regionHits(sc, rect, region->bottom, count, out_points);
		if (isRectInside(region->top->rect,    &rect)) return regionHits(sc, rect, region->top,    count, out_points);
		if (isRectInside(region->btmid->rect,  &rect)) return regionHits(sc, rect, region->btmid,  count, out_points);
	}

	// if not fully contained in any children, check self
	Points* p = region->rankpoints;
	int hits = findHitsSV((Rect*)&rect, p->id, p->rank, p->x, p->y, p->n, out_points, count);
	if (hits < count) return -1;
	return hits;
}



// DLL IMPLEMENTATION -----------------------------------------------------------------------------

int regions = 0;

Points* buildPoints(int n) {
	Points* p = (Points*)malloc(sizeof(Points));
	p->n    = n;
	p->id   = (int8_t*)calloc(n, sizeof(int8_t));
	p->rank = (int32_t*)calloc(n, sizeof(int32_t));
	p->x    = (float*)calloc(n, sizeof(float));
	p->y    = (float*)calloc(n, sizeof(float));
	return p;
}

Points* copyPoints(Points* src) {
	Points* p = (Points*)malloc(sizeof(Points));
	p->n    = src->n;
	p->id   = (int8_t*)calloc(src->n, sizeof(int8_t));
	p->rank = (int32_t*)calloc(src->n, sizeof(int32_t));
	p->x    = (float*)calloc(src->n, sizeof(float));
	p->y    = (float*)calloc(src->n, sizeof(float));
	memcpy(p->id,   src->id,   p->n * sizeof(int8_t));
	memcpy(p->rank, src->rank, p->n * sizeof(int32_t));
	memcpy(p->x,    src->x,    p->n * sizeof(float));
	memcpy(p->y,    src->y,    p->n * sizeof(float));
	return p;
}

void fillPoints(Points* p, Point* arr, int n) {
	for (int i = 0; i < n; i++) {
		p->id[i]   = arr[i].id;
		p->rank[i] = arr[i].rank;
		p->x[i]    = arr[i].x;
		p->y[i]    = arr[i].y;
	}
}

void fillPointArr(Point* arr, Points* p) {
	for (int i = 0; i < p->n; i++) {
		arr[i].id   = p->id[i];
		arr[i].rank = p->rank[i];
		arr[i].x    = p->x[i];
		arr[i].y    = p->y[i];
	}
}

void freePoints(Points* p) {
	free(p->id);
	free(p->rank);
	free(p->x);
	free(p->y);
	free(p);
	p = NULL;
}

Region* buildRegion(GumpSearchContext* sc, Rect* rect, Region* lover, Region* lrover, Region* rover, Region* bover, Region* btover, Region* tover, int depth) {
	regions++;
	Region* region = (Region*)malloc(sizeof(Region));
	region->rect       = rect;
	region->subw       = (rect->hx - rect->lx) / 2;
	region->subh       = (rect->hy - rect->ly) / 2;
	region->crect      = NULL;
	region->left       = NULL;
	region->right      = NULL;
	region->lrmid      = NULL;
	region->bottom     = NULL;
	region->top        = NULL;
	region->btmid      = NULL;
	region->ranksort   = NULL;
	region->rankpoints = NULL;

	int est = MAXLEAF;
	int blocks = -1;

	// only compute point count estimate if deep in tree
	if (depth >= BLOCKCHECK) {
		double di = (double)(rect->lx - sc->bounds->lx) / sc->dx;
		double dj = (double)(rect->ly - sc->bounds->ly) / sc->dy;
		double dp = (double)(rect->hx - sc->bounds->lx) / sc->dx;
		double dq = (double)(rect->hy - sc->bounds->ly) / sc->dy;
		int i = floor(di); if (i < 0) i = 0;
		int j = floor(dj); if (j < 0) j = 0;
		int p = ceil(dp); if (p > DIVS) p = DIVS;
		int q = ceil(dq); if (q > DIVS) q = DIVS;
		int w = p - i;
		int h = q - j;

		if (rect->lx < sc->grect[i][j].lx) i--;
		if (rect->ly < sc->grect[i][j].ly) j--;

		blocks = 0;
		est = 0;
		for (int a = 0; a < w; a++) {
			for (int b = 0; b < h; b++) {
				int dlen = sc->dlen[a+i][b+j];
				if (dlen == 0) continue;
				if (!isRectOverlap(rect, &sc->drect[a+i][b+j])) continue;
				est += dlen;

				sc->blocks[blocks] = sc->grid[a+i][b+j];
				sc->blocki[blocks] = 0;
				sc->blockn[blocks] = dlen;
				blocks++;
			}
		}
	}

	bool isleaf = (depth == 9 && est < MAXLEAF) || depth == 10;
	int len = isleaf ? LEAFSIZE : NODESIZE;
	region->ranksort = (Point*)calloc(len, sizeof(Point));
	if (blocks > 0) {
		if (blocks == 1) region->n = findHitsS(rect, sc->blocks[0], sc->blockn[0], region->ranksort, len);
		else region->n = findHitsB(rect, blocks, sc->blocks, sc->blocki, sc->blockn, region->ranksort, len);
	} else region->n = searchBinary(sc, *rect, len, region->ranksort);

	if (isleaf) return region;

	// build child regions
	double xmid = ((double)rect->lx + (double)rect->hx) / 2;
	double ymid = ((double)rect->ly + (double)rect->hy) / 2;
	double xq1  = ((double)rect->lx + (double)xmid)     / 2;
	double xq3  = ((double)xmid     + (double)rect->hx) / 2;
	double yq1  = ((double)rect->ly + (double)ymid)     / 2;
	double yq3  = ((double)ymid     + (double)rect->hy) / 2;
	region->crect = (Rect*)calloc(6, sizeof(Rect));
	region->crect[0].lx = rect->lx; region->crect[0].hx = xmid;     region->crect[0].ly = rect->ly; region->crect[0].hy = rect->hy;
	region->crect[1].lx = xmid;     region->crect[1].hx = rect->hx; region->crect[1].ly = rect->ly; region->crect[1].hy = rect->hy;
	region->crect[2].lx = xq1;      region->crect[2].hx = xq3;      region->crect[2].ly = rect->ly; region->crect[2].hy = rect->hy;
	region->crect[3].lx = rect->lx; region->crect[3].hx = rect->hx; region->crect[3].ly = rect->ly; region->crect[3].hy = ymid;
	region->crect[4].lx = rect->lx; region->crect[4].hx = rect->hx; region->crect[4].ly = ymid;     region->crect[4].hy = rect->hy;
	region->crect[5].lx = rect->lx; region->crect[5].hx = rect->hx; region->crect[5].ly = yq1;      region->crect[5].hy = yq3;
	region->left   = lover  ? lover  : buildRegion(sc, &region->crect[0], NULL, NULL, NULL, NULL, NULL, NULL, depth+1);
	region->right  = rover  ? rover  : buildRegion(sc, &region->crect[1], NULL, NULL, NULL, NULL, NULL, NULL, depth+1);
	region->lrmid  = lrover ? lrover : buildRegion(sc, &region->crect[2], region->left->right, NULL, region->right->left, NULL, NULL, NULL, depth+1);
	region->bottom = bover  ? bover  : buildRegion(sc, &region->crect[3], region->left->bottom, region->lrmid->bottom, region->right->bottom, NULL, NULL, NULL, depth+1);
	region->top    = tover  ? tover  : buildRegion(sc, &region->crect[4], region->left->top, region->lrmid->top, region->right->top, NULL, NULL, NULL, depth+1);
	region->btmid  = btover ? btover : buildRegion(sc, &region->crect[5], region->left->btmid, region->lrmid->btmid, region->right->btmid, region->bottom->top, NULL, region->top->bottom, depth+1);

	return region;
}

void convertRegion(Region* region) {
	if (region->rankpoints != NULL) return; // This region has already been converted

	region->rankpoints = buildPoints(region->n);
	fillPoints(region->rankpoints, region->ranksort, region->n);
	free(region->ranksort);
	region->ranksort = NULL;

	if (region->left)   convertRegion(region->left);
	if (region->right)  convertRegion(region->right);
	if (region->lrmid)  convertRegion(region->lrmid);
	if (region->bottom) convertRegion(region->bottom);
	if (region->top)    convertRegion(region->top);
	if (region->btmid)  convertRegion(region->btmid);
}

void freeRegion(Region* region, bool left, bool lrmid, bool right, bool bottom, bool btmid, bool top) {
	if (left   && region->left)   freeRegion(region->left,   true,  true,  true,  true,  true, true);
	if (right  && region->right)  freeRegion(region->right,  true,  true,  true,  true,  true, true);
	if (lrmid  && region->lrmid)  freeRegion(region->lrmid,  false, true,  false, true,  true, true);
	if (bottom && region->bottom) freeRegion(region->bottom, false, false, false, true,  true, true);
	if (top    && region->top)    freeRegion(region->top,    false, false, false, true,  true, true);
	if (btmid  && region->btmid)  freeRegion(region->btmid,  false, false, false, false, true, false);
	if (region->crect) free(region->crect);
	if (region->rankpoints) freePoints(region->rankpoints);
	free(region);
}

void buildGrid(GumpSearchContext* sc) {
	sc->blocks = (Point**)calloc(DIVS*DIVS, sizeof(Point*));
	sc->blocki = (int*)calloc(DIVS*DIVS, sizeof(int));
	sc->blockn = (int*)calloc(DIVS*DIVS, sizeof(int));

	sc->dx = (double)(sc->bounds->hx - sc->bounds->lx) / (double)DIVS;
	sc->dy = (double)(sc->bounds->hy - sc->bounds->ly) / (double)DIVS;
	DPRINT(("Bounds are [%f,%f,%f,%f]: dx = %f, dy = %f, area %f\n",
		sc->bounds->lx, sc->bounds->hx, sc->bounds->ly, sc->bounds->hy,
		sc->dx, sc->dy, sc->area
	));

	memcpy(sc->gridsort, sc->xsort, sc->N * sizeof(Point));
	sc->grid = (Point***)calloc(DIVS, sizeof(Point**));
	sc->grect = (Rect**)calloc(DIVS, sizeof(Rect*));
	sc->drect = (Rect**)calloc(DIVS, sizeof(Rect*));
	sc->dlen = (int**)calloc(DIVS, sizeof(int*));
	int xidxl = 0;
	for (int i = 0; i < DIVS; i++) {
		double lx = sc->bounds->lx + (double)i * sc->dx;
		double hx = sc->bounds->lx + (double)(i+1) * sc->dx;
		if (i == DIVS - 1) hx = sc->bounds->hx;
		int xidxr = xidxl + bsearchx(&sc->gridsort[xidxl], false, hx, 0, sc->N - xidxl + 1);
		int nx = xidxr - xidxl + 1;
		ysort(&sc->gridsort[xidxl], nx);

		sc->grid[i] = (Point**)calloc(DIVS, sizeof(Point*));
		sc->grect[i] = (Rect*)calloc(DIVS, sizeof(Rect));
		sc->drect[i] = (Rect*)calloc(DIVS, sizeof(Rect));
		sc->dlen[i] = (int*)calloc(DIVS, sizeof(int));
		int yidxl = xidxl;
		for (int j = 0; j < DIVS; j++) {
			double ly = sc->bounds->ly + (double)j * sc->dy;
			double hy = sc->bounds->ly + (double)(j+1) * sc->dy;
			if (j == DIVS - 1) hy = sc->bounds->hy;
			int yidxr = yidxl + bsearchy(&sc->gridsort[yidxl], false, hy, 0, xidxr - yidxl + 1);
			int ny = yidxr - yidxl + 1;

			sc->grect[i][j].lx = lx;
			sc->grect[i][j].ly = ly;
			sc->grect[i][j].hx = hx;
			sc->grect[i][j].hy = hy;
			if (ny <= 0) {
				sc->dlen[i][j] = 0;
				sc->grid[i][j] = NULL;
			} else {
				sc->dlen[i][j] = ny;
				sc->grid[i][j] = (Point*)calloc(ny, sizeof(Point));
				memcpy(sc->grid[i][j], &sc->gridsort[yidxl], ny * sizeof(Point));
				ranksort(sc->grid[i][j], ny);

				sc->drect[i][j].lx = RANKMAX;
				sc->drect[i][j].ly = RANKMAX;
				sc->drect[i][j].hx = -RANKMAX;
				sc->drect[i][j].hy = -RANKMAX;
				for (int p = 0; p < ny; p++) {
					if (p == 0 || sc->grid[i][j][p].x < sc->drect[i][j].lx) sc->drect[i][j].lx = sc->grid[i][j][p].x;
					if (p == 0 || sc->grid[i][j][p].y < sc->drect[i][j].ly) sc->drect[i][j].ly = sc->grid[i][j][p].y;
					if (p == 0 || sc->grid[i][j][p].x > sc->drect[i][j].hx) sc->drect[i][j].hx = sc->grid[i][j][p].x;
					if (p == 0 || sc->grid[i][j][p].y > sc->drect[i][j].hy) sc->drect[i][j].hy = sc->grid[i][j][p].y;
				}
			}

			// If there are points on the boundary, they need to be included in both grid blocks
			if (ny > 0 && sc->gridsort[yidxr].y == hx) {
				yidxl = yidxr;
				while (sc->gridsort[yidxl].y == sc->gridsort[yidxl-1].y) yidxl--;
			} else yidxl = yidxr + 1;
		}

		// If there are points on the boundary, they need to be included in both grid blocks
		if (nx > 0 && sc->gridsort[xidxr].x == hx) {
			xidxl = xidxr;
			while (sc->gridsort[xidxl].x == sc->gridsort[xidxl-1].x) xidxl--;
		} else xidxl = xidxr + 1;
	}
}

void freeGrid(GumpSearchContext* sc) {
	DPRINT(("Freeing grid tree\n"));
	for (int i = 0; i < DIVS; i++) {
		for (int j = 0; j < DIVS; j++) {
			free(sc->grid[i][j]);
		}
		free(sc->grid[i]);
		free(sc->dlen[i]);
		free(sc->drect[i]);
	}
	free(sc->grid);
	free(sc->dlen);
	free(sc->drect);
	free(sc->bounds);
	free(sc->blocks);
	free(sc->blocki);
	free(sc->blockn);
}

__stdcall SearchContext* create(const Point* points_begin, const Point* points_end) {
	GumpSearchContext* gsc = (GumpSearchContext*)malloc(sizeof(GumpSearchContext));
	gsc->N = points_end - points_begin;
	if (gsc->N == 0) return (SearchContext*)gsc;

	DPRINT(("Allocating and copying memory\n"));
	gsc->trim = (Rect*)malloc(sizeof(Rect));
	gsc->xsort = (Point*)calloc(gsc->N, sizeof(Point));
	gsc->ysort = (Point*)calloc(gsc->N, sizeof(Point));
	gsc->ranksort = (Point*)calloc(gsc->N, sizeof(Point));
	gsc->gridsort = (Point*)calloc(gsc->N, sizeof(Point));
	memcpy(gsc->xsort, points_begin, gsc->N * sizeof(Point));
	memcpy(gsc->ysort, points_begin, gsc->N * sizeof(Point));
	memcpy(gsc->ranksort, points_begin, gsc->N * sizeof(Point));

	DPRINT(("Sorting points\n"));
	xsort(gsc->xsort, gsc->N);
	ysort(gsc->ysort, gsc->N);
	ranksort(gsc->ranksort, gsc->N);

	gsc->bounds = (Rect*)malloc(sizeof(Rect));
	gsc->bounds->lx = gsc->xsort[1].x;
	gsc->bounds->hx = gsc->xsort[gsc->N-2].x;
	gsc->bounds->ly = gsc->ysort[1].y;
	gsc->bounds->hy = gsc->ysort[gsc->N-2].y;
	gsc->area = rectArea(gsc->bounds);

	DPRINT(("Building grid tree\n"));
	buildGrid(gsc);

	DPRINT(("Building region tree\n"));
	// convert array of stuctures pattern to structure of arrays pattern
	gsc->xpoints = buildPoints(gsc->N); fillPoints(gsc->xpoints, gsc->xsort, gsc->N);
	gsc->ypoints = buildPoints(gsc->N); fillPoints(gsc->ypoints, gsc->ysort, gsc->N);
	gsc->root = buildRegion(gsc, gsc->bounds, NULL, NULL, NULL, NULL, NULL, NULL, 1);

	// remove("rects.csv");
	// FILE *f = fopen("points.csv", "w");
	// for (int i = 0; i < gsc->N; i++) {
	// 	fprintf(f, "%d,%f,%f\n", gsc->ranksort[i].rank, gsc->ranksort[i].x, gsc->ranksort[i].y);
	// }
	// fclose(f);

	free(gsc->xsort);
	free(gsc->ysort);
	free(gsc->gridsort);
	free(gsc->ranksort);

	convertRegion(gsc->root);

	return (SearchContext*)gsc;
}

__stdcall int32_t search(SearchContext* sc, Rect rect, const int32_t count, Point* out_points) {
	GumpSearchContext* gsc = (GumpSearchContext*)sc;
	if (gsc->N == 0) return 0;

	gsc->trim->lx = (rect.lx < gsc->bounds->lx) ? gsc->bounds->lx : rect.lx;
	gsc->trim->hx = (rect.hx > gsc->bounds->hx) ? gsc->bounds->hx : rect.hx;
	gsc->trim->ly = (rect.ly < gsc->bounds->ly) ? gsc->bounds->ly : rect.ly;
	gsc->trim->hy = (rect.hy > gsc->bounds->hy) ? gsc->bounds->hy : rect.hy;

	gsc->w = gsc->trim->hx - gsc->trim->lx;
	gsc->h = gsc->trim->hy - gsc->trim->ly;
	float apct = (gsc->w * gsc->h) / gsc->area;

	int hits = 0;
	// Don't run region search if likely to fail
	if (apct > REGIONTHRESH) {
		hits = regionHits(gsc, *gsc->trim, gsc->root, count, out_points);
		if (hits > 0) return hits;
	}

	// if region search fails, fall back on grid or binary
	int xidxl, xidxr, yidxl, yidxr, nx, ny;

	// if valid x range is likely to be smaller than y range, check it first
	if (gsc->w / gsc->dx < gsc->h / gsc->dy) {
		xidxl = bvalsearch(gsc->xpoints->x, true, rect.lx, 0, gsc->N);
		xidxr = bvalsearch(gsc->xpoints->x, false, rect.hx, 0, gsc->N);
		nx = xidxr - xidxl + 1;
		if (nx == 0) return 0;

		if (nx < LINTHRESH1) return findHitsUyV(&rect, &gsc->xpoints->id[xidxl], &gsc->xpoints->rank[xidxl], &gsc->xpoints->y[xidxl], nx, out_points, count);

		yidxl = bvalsearch(gsc->ypoints->y, true, rect.ly, 0, gsc->N);
		yidxr = bvalsearch(gsc->ypoints->y, false, rect.hy, 0, gsc->N);
		ny = yidxr - yidxl + 1;
		if (ny == 0) return 0;

		if (ny < LINTHRESH2) return findHitsUxV(&rect, &gsc->ypoints->id[yidxl], &gsc->ypoints->rank[yidxl], &gsc->ypoints->x[yidxl], ny, out_points, count);
	} else {
		yidxl = bvalsearch(gsc->ypoints->y, true, rect.ly, 0, gsc->N);
		yidxr = bvalsearch(gsc->ypoints->y, false, rect.hy, 0, gsc->N);
		ny = yidxr - yidxl + 1;
		if (ny == 0) return 0;

		if (ny < LINTHRESH1) return findHitsUxV(&rect, &gsc->ypoints->id[yidxl], &gsc->ypoints->rank[yidxl], &gsc->ypoints->x[yidxl], ny, out_points, count);

		xidxl = bvalsearch(gsc->xpoints->x, true, rect.lx, 0, gsc->N);
		xidxr = bvalsearch(gsc->xpoints->x, false, rect.hx, 0, gsc->N);
		nx = xidxr - xidxl + 1;
		if (nx == 0) return 0;

		if (nx < LINTHRESH2) return findHitsUyV(&rect, &gsc->xpoints->id[xidxl], &gsc->xpoints->rank[xidxl], &gsc->xpoints->y[xidxl], nx, out_points, count);
	}

	// find grid block for the bottom left and top right corners of the query rect
	double di = (double)(gsc->trim->lx - gsc->bounds->lx) / gsc->dx;
	double dj = (double)(gsc->trim->ly - gsc->bounds->ly) / gsc->dy;
	double dp = (double)(gsc->trim->hx - gsc->bounds->lx) / gsc->dx;
	double dq = (double)(gsc->trim->hy - gsc->bounds->ly) / gsc->dy;
	int i = floor(di); if (i < 0) i = 0;
	int j = floor(dj); if (j < 0) j = 0;
	int p = ceil(dp); if (p > DIVS) p = DIVS;
	int q = ceil(dq); if (q > DIVS) q = DIVS;
	int w = p - i;
	int h = q - j;

	if (gsc->trim->lx < gsc->grect[i][j].lx) i--;
	if (gsc->trim->ly < gsc->grect[i][j].ly) j--;

	int exptests = 0;
	int blocks = 0;
	for (int a = 0; a < w; a++) {
		for (int b = 0; b < h; b++) {
			int len = gsc->dlen[a+i][b+j];
			if (len == 0) continue;
			if (!isRectOverlap(&rect, &gsc->drect[a+i][b+j])) continue;

			gsc->blocks[blocks] = gsc->grid[a+i][b+j];
			gsc->blocki[blocks] = 0;
			gsc->blockn[blocks] = len;
			exptests += len;
			blocks++;
		}
	}

	if (blocks == 0) return 0;

	int nsmall = nx < ny ? nx : ny;
	if (nsmall > LINTHRESH3 || exptests * GRIDFACTOR < nsmall) {
		if (blocks == 1) return findHitsS((Rect*)&rect, gsc->blocks[0], gsc->blockn[0], out_points, count);
		else return findHitsB((Rect*)&rect, blocks, gsc->blocks, gsc->blocki, gsc->blockn, out_points, count);
	} else {
		if (nx < ny) return findHitsUyV(&rect, &gsc->xpoints->id[xidxl], &gsc->xpoints->rank[xidxl], &gsc->xpoints->y[xidxl], nx, out_points, count);
		else return findHitsUxV(&rect, &gsc->ypoints->id[yidxl], &gsc->ypoints->rank[yidxl], &gsc->ypoints->x[yidxl], ny, out_points, count);
	}

	// totops += ops;
	// if (method == 2) DPRINT(("%d,%d,%d,%f,%d,%d,%d,%d,%d\n", ops, nx, ny, pct, blocks, exptests, nsmall, w, h));

	// FILE *f = fopen("rects.csv", "a");
	// fprintf(f, "%f,%f,%f,%f,%d\n", gsc->trim->lx, gsc->trim->hx, gsc->trim->ly, gsc->trim->hy, ops);
	// fclose(f);
}

__stdcall SearchContext* destroy(SearchContext* sc) {
	GumpSearchContext* gsc = (GumpSearchContext*)sc;
	if (gsc->N == 0) {
		free(gsc);
		return NULL;
	}

	freePoints(gsc->xpoints);
	freePoints(gsc->ypoints);
	free(gsc->trim);
	freeRegion(gsc->root, true, true, true, true, true, true);
	freeGrid(gsc);
	free(gsc);
	return NULL;
}