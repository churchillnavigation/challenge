#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "gumption.h"
#include "iqsort.h"

// #define DEBUG 0
#define WRITEFILES 0

#ifdef DEBUG
	#define DPRINT(x) printf x
#else
	#define DPRINT(x) do {} while (0)
#endif

// rank search parameters
#define BASELIMIT 15000

// region search parameters
#define MAXDEPTH 9
#define MAXLEAF 100000
#define NODESIZE 500
#define LEAFSIZE 500

// grid search parameters
#define DIVS 150
#define GRIDFACTOR 0.75f
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

int hitchecks = 0;

inline float rectArea(Rect* rect) {
	return (rect->hx - rect->lx) * (rect->hy - rect->ly);
}

inline bool isRectInside(Rect* r1, Rect* r2) {
	return r2->lx >= r1->lx && r2->ly >= r1->ly && r2->hx <= r1->hx && r2->hy <= r1->hy;
}

inline bool isRectOverlap(Rect* r1, Rect* r2) {
	return r1->lx <= r2->hx && r1->hx >= r2->lx && r1->ly <= r2->hy && r1->hy >= r2->ly;
}

inline bool rectOverlapPercent(Rect* r1, Rect* r2) {
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

int bvalsearch(float p[], bool minOrMax, float v, int imin, int imax) {
	while (imax >= imin) {
		int imid = (imin + imax) / 2;
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

int32_t findHitsB(GumpSearchContext* sc, Rect* rect, int b, Point** blocks, int* blocki, int* blockn, Point* out, int count) {
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
			if (sc->blocki[i] >= sc->blockn[i]) { fin++; continue; }

			Point p = sc->blocks[i][sc->blocki[i]];
			if (p.rank == prank) {
				sc->blocki[i]++;
				p = sc->blocks[i][sc->blocki[i]];
			}
			if (p.rank < minrank) {
				minb = i;
				minrank = p.rank;
			}
		}

		// If we've hit the end of all blocks, exit
		if (fin == b) break;

		Point bestp = sc->blocks[minb][sc->blocki[minb]];
		if (isHit(rect, &bestp)) {
			out[k] = bestp;
			prank = bestp.rank;
			k++;
		}
		sc->blocki[minb]++;
	}

	return k;
}



// SEARCH IMPLEMENTATIONS -------------------------------------------------------------------------

// binary search - narrow search to points in x range, y range, and check smaller set
int32_t searchBinary(GumpSearchContext* sc, const Rect rect, const int32_t count, Point* out_points) {
	int xidxl = bsearchx(sc->xsort, true, rect.lx, 0, sc->N);
	int xidxr = bsearchx(sc->xsort, false, rect.hx, 0, sc->N);
	int nx = xidxr - xidxl + 1;
	if (nx == 0) return 0;

	int yidxl = bsearchy(sc->ysort, true, rect.ly, 0, sc->N);
	int yidxr = bsearchy(sc->ysort, false, rect.hy, 0, sc->N);
	int ny = yidxr - yidxl + 1;
	if (ny == 0) return 0;

	if ((nx < ny ? nx : ny) > BASELIMIT) return findHitsS((Rect*)&rect, sc->ranksort, sc->N, out_points, count);

	if (nx < ny) return findHitsU((Rect*)&rect, &sc->xsort[xidxl], nx, out_points, count, isHitY);
	else return findHitsU((Rect*)&rect, &sc->ysort[yidxl], ny, out_points, count, isHitX);
}


int32_t regionHits(GumpSearchContext* sc, Rect rect, Region* region, int count, Point* out_points, int depth) {
	if (region->n == 0) return 0;

	// if this is a leaf, check it
	if (region->left == NULL) {
		int hits = findHitsS((Rect*)&rect, region->ranksort, region->n, out_points, count);
		if (hits < count) return -depth;
		return hits;
	}

	// look for a child that fully contains this rect
	if (isRectInside(region->left->rect,   &rect)) return regionHits(sc, rect, region->left,   count, out_points, depth+1);
	if (isRectInside(region->right->rect,  &rect)) return regionHits(sc, rect, region->right,  count, out_points, depth+1);
	if (isRectInside(region->bottom->rect, &rect)) return regionHits(sc, rect, region->bottom, count, out_points, depth+1);
	if (isRectInside(region->top->rect,    &rect)) return regionHits(sc, rect, region->top,    count, out_points, depth+1);
	if (isRectInside(region->lrmid->rect,  &rect)) return regionHits(sc, rect, region->lrmid,  count, out_points, depth+1);
	if (isRectInside(region->btmid->rect,  &rect)) return regionHits(sc, rect, region->btmid,  count, out_points, depth+1);

	// if not fully contained in any children, check self
	int hits = findHitsS((Rect*)&rect, region->ranksort, region->n, out_points, count);
	if (hits < count) return -depth;
	return hits;
}

int ngrid = 0, nbin = 0;
int32_t searchGumption(GumpSearchContext* sc, Rect rect, const int32_t count, Point* out_points) {
	sc->trim->lx = rect.lx; sc->trim->hx = rect.hx;
	sc->trim->ly = rect.ly; sc->trim->hy = rect.hy;
	if (sc->trim->lx < sc->bounds->lx) sc->trim->lx = sc->bounds->lx;
	if (sc->trim->hx > sc->bounds->hx) sc->trim->hx = sc->bounds->hx;
	if (sc->trim->ly < sc->bounds->ly) sc->trim->ly = sc->bounds->ly;
	if (sc->trim->hy > sc->bounds->hy) sc->trim->hy = sc->bounds->hy;

	// run region search
	int hits = regionHits(sc, *sc->trim, sc->root, count, out_points, 1);
	if (hits > 0) return hits;

	// if region search fails, fall back on grid or binary
	int xidxl = bsearchx(sc->xsort, true, rect.lx, 0, sc->N);
	int xidxr = bsearchx(sc->xsort, false, rect.hx, 0, sc->N);
	int nx = xidxr - xidxl + 1;
	if (nx == 0) return 0;

	int yidxl = bsearchy(sc->ysort, true, rect.ly, 0, sc->N);
	int yidxr = bsearchy(sc->ysort, false, rect.hy, 0, sc->N);
	int ny = yidxr - yidxl + 1;
	if (ny == 0) return 0;

	// find grid block for the bottom left and top right corners of the query rect
	// subtract FLT_MIN to handle cases where bottom left is on a bottom or left grid boundary
	double di = (double)(sc->trim->lx - sc->bounds->lx) / sc->dx;
	double dj = (double)(sc->trim->ly - sc->bounds->ly) / sc->dy;
	double dp = (double)(sc->trim->hx - sc->bounds->lx) / sc->dx;
	double dq = (double)(sc->trim->hy - sc->bounds->ly) / sc->dy;
	int i = floor(di); if (i < 0) i = 0;
	int j = floor(dj); if (j < 0) j = 0;
	int p = ceil(dp); if (p > DIVS) p = DIVS;
	int q = ceil(dq); if (q > DIVS) q = DIVS;
	int w = p - i;
	int h = q - j;

	if (sc->trim->lx < sc->grect[i][j].lx) i--;
	if (sc->trim->ly < sc->grect[i][j].ly) j--;

	int maxtests = 0;
	int blocks = 0;
	for (int a = 0; a < w; a++) {
		for (int b = 0; b < h; b++) {
			int len = sc->dlen[a+i][b+j];
			if (len == 0) continue;
			if (!isRectOverlap(&rect, &sc->drect[a+i][b+j])) continue;
			sc->blocks[blocks] = sc->grid[a+i][b+j];
			sc->blocki[blocks] = 0;
			sc->blockn[blocks] = len;
			maxtests += len;
			blocks++;
		}
	}

	if (blocks == 0) return 0;

	if ((float)maxtests * GRIDFACTOR < (float)(nx < ny ? nx : ny)) {
		if (blocks == 1) hits = findHitsS((Rect*)&rect, sc->blocks[0], sc->blockn[0], out_points, count);
		else hits = findHitsB(sc, (Rect*)&rect, blocks, sc->blocks, sc->blocki, sc->blockn, out_points, count);
	} else {
		if (nx < ny) hits = findHitsU((Rect*)&rect, &sc->xsort[xidxl], nx, out_points, count, isHitY);
		else hits = findHitsU((Rect*)&rect, &sc->ysort[yidxl], ny, out_points, count, isHitX);
	}


	return hits;
}



// DLL IMPLEMENTATION -----------------------------------------------------------------------------

int regions = 0;

Region* buildRegion(GumpSearchContext* sc, Rect* rect, Region* lover, Region* lrover, Region* rover, Region* bover, Region* btover, Region* tover, int depth) {
	regions++;
	Region* region = (Region*)malloc(sizeof(Region));
	region->rect     = rect;
	region->crect    = NULL;
	region->left     = NULL;
	region->right    = NULL;
	region->lrmid    = NULL;
	region->bottom   = NULL;
	region->top      = NULL;
	region->btmid    = NULL;
	region->ranksort = NULL;

	int est = MAXLEAF;
	int blocks = -1;

	// only compute point count estimate if deep in tree
	if (depth >= 8) {
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
		for (int a = 0; a < w; a++) {
			for (int b = 0; b < h; b++) {
				int dlen = sc->dlen[a+i][b+j];
				if (dlen == 0) continue;
				if (!isRectOverlap(rect, &sc->drect[a+i][b+j])) continue;
				int p = rectOverlapPercent(&sc->drect[a+i][b+j], rect) * dlen;
				est += p;

				sc->blocks[blocks] = sc->grid[a+i][b+j];
				sc->blocki[blocks] = 0;
				sc->blockn[blocks] = dlen;
				blocks++;
			}
		}
	}

	bool isleaf = est < MAXLEAF || depth == MAXDEPTH;
	int len = isleaf ? LEAFSIZE : NODESIZE;
	region->ranksort = (Point*)calloc(len, sizeof(Point));
	if (blocks > 0) {
		if (blocks == 1) region->n = findHitsS(rect, sc->blocks[0], sc->blockn[0], region->ranksort, len);
		else region->n = findHitsB(sc, rect, blocks, sc->blocks, sc->blocki, sc->blockn, region->ranksort, len);
	} else region->n = searchBinary(sc, *rect, len, region->ranksort);

	if (isleaf) return region;

	// build child regions
	float xmid = (rect->lx + rect->hx) / 2;
	float ymid = (rect->ly + rect->hy) / 2;
	float xq1  = (rect->lx + xmid) / 2;
	float xq3  = (xmid + rect->hx) / 2;
	float yq1  = (rect->ly + ymid) / 2;
	float yq3  = (ymid + rect->hy) / 2;
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

void freeRegion(Region* region, bool left, bool lrmid, bool right, bool bottom, bool btmid, bool top) {
	if (left   && region->left)   freeRegion(region->left,   true,  true,  true,  true,  true, true);
	if (right  && region->right)  freeRegion(region->right,  true,  true,  true,  true,  true, true);
	if (lrmid  && region->lrmid)  freeRegion(region->lrmid,  false, true,  false, true,  true, true);
	if (bottom && region->bottom) freeRegion(region->bottom, false, false, false, true,  true, true);
	if (top    && region->top)    freeRegion(region->top,    false, false, false, true,  true, true);
	if (btmid  && region->btmid)  freeRegion(region->btmid,  false, false, false, false, true, false);
	if (region->crect) free(region->crect);
	if (region->ranksort) free(region->ranksort);
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
	GumpSearchContext* sc = (GumpSearchContext*)malloc(sizeof(GumpSearchContext));
	sc->N = points_end - points_begin;
	if (sc->N == 0) return (SearchContext*)sc;

	DPRINT(("Allocating and copying memory\n"));
	sc->trim = (Rect*)malloc(sizeof(Rect));
	sc->xsort = (Point*)calloc(sc->N, sizeof(Point));
	sc->ysort = (Point*)calloc(sc->N, sizeof(Point));
	sc->ranksort = (Point*)calloc(sc->N, sizeof(Point));
	sc->gridsort = (Point*)calloc(sc->N, sizeof(Point));
	memcpy(sc->xsort, points_begin, sc->N * sizeof(Point));
	memcpy(sc->ysort, points_begin, sc->N * sizeof(Point));
	memcpy(sc->ranksort, points_begin, sc->N * sizeof(Point));

	DPRINT(("Sorting points\n"));
	xsort(sc->xsort, sc->N);
	ysort(sc->ysort, sc->N);
	ranksort(sc->ranksort, sc->N);

	sc->bounds = (Rect*)malloc(sizeof(Rect));
	sc->bounds->lx = sc->xsort[1].x;
	sc->bounds->hx = sc->xsort[sc->N-2].x;
	sc->bounds->ly = sc->ysort[1].y;
	sc->bounds->hy = sc->ysort[sc->N-2].y;
	sc->area = rectArea(sc->bounds);

	DPRINT(("Building grid tree\n"));
	buildGrid(sc);
	DPRINT(("Building region tree\n"));
	sc->root = buildRegion(sc, sc->bounds, NULL, NULL, NULL, NULL, NULL, NULL, 1);

	if (WRITEFILES) {
		DPRINT(("Writing points file\n"));
		FILE *f = fopen("points.csv", "w");
		for (int i = 0; i < sc->N; i++) {
			fprintf(f, "%d,%f,%f\n", sc->gridsort[i].rank, sc->gridsort[i].x, sc->gridsort[i].y);
		}
		fclose(f);
		remove("rects.csv");
	}

	free(sc->gridsort);
	free(sc->ranksort);

	return (SearchContext*)sc;
}

__stdcall int32_t search(SearchContext* sc, Rect rect, const int32_t count, Point* out_points) {
	GumpSearchContext* context = (GumpSearchContext*)sc;
	if (context->N == 0) return 0;
	int hits = searchGumption(context, rect, count, out_points);
	// printRect(rect);
	// printPoints(out_points, hits);
	return hits;
}

__stdcall SearchContext* destroy(SearchContext* sc) {
	GumpSearchContext* context = (GumpSearchContext*)sc;
	if (context->N == 0) {
		free(context);
		return NULL;
	}

	free(context->xsort);
	free(context->ysort);
	free(context->trim);
	freeRegion(context->root, true, true, true, true, true, true);
	freeGrid(context);
	free(context);
	return NULL;
}