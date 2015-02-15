#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "gumption.h"

// #define DEBUG 1
#define WRITEFILES 0

#ifdef DEBUG
	#define DPRINT(x) printf x
#else
	#define DPRINT(x) do {} while (0)
#endif

// rank search parameters
#define BASELIMIT 15000

// binary search parameters
#define BINARYLIMIT 2500

// region search parameters
#define MAXDEPTH 8
#define NODESIZE 500
#define LEAFSIZE 1000

// grid search parameters
#define DIVS 50
#define GRIDFACTOR 3.0f
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



// HELPER FUNCTIONS -------------------------------------------------------------------------------

inline float rectArea(Rect* rect) {
	return (rect->hx - rect->lx) * (rect->hy - rect->ly);
}

inline bool isRectInside(Rect* r1, Rect* r2) {
	return r2->lx >= r1->lx && r2->ly >= r1->ly && r2->hx <= r1->hx && r2->hy <= r1->hy;
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
		qsort(out, hits, sizeof(Point), rankcomp);
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

	qsort(out, hits, sizeof(Point), rankcomp);
	return hits;
}

int32_t findHitsS(Rect* rect, Point* in, int n, Point* out, int count) {
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

int32_t findHitsB(GumpSearchContext* sc, Rect* rect, int bx, int by, int w, int h, Point* out, int count) {
	int32_t k = 0;

	int b = 0;
	for (int i = 0; i < w; i++) {
		for (int j = 0; j < h; j++) {
			if (sc->rlen[bx+i][by+j] == 0) continue;
			sc->blocks[b] = sc->grid[bx+i][by+j];
			sc->blocki[b] = 0;
			sc->blockn[b] = sc->rlen[bx+i][by+j];
			b++;
		}
	}

	if (b == 0) return 0;
	if (b == 1) return findHitsS(rect, sc->blocks[0], sc->blockn[0], out, count);

	int minrank = RANKMAX;
	int minb = -1;
	int fin = 0;
	while (k < count) {
		minrank = RANKMAX;
		fin = 0;

		// find min rank
		for (int i = 0; i < b; i++) {
			if (sc->blocki[i] >= sc->blockn[i]) { fin++; continue; }

			Point p = sc->blocks[i][sc->blocki[i]];
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

	if ((nx < ny ? nx : ny) > BASELIMIT) return findHitsS((Rect*)&rect, sc->gridsort, sc->N, out_points, count);

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

	// if not fully contained in any childre, check self
	int hits = findHitsS((Rect*)&rect, region->ranksort, region->n, out_points, count);
	if (hits < count) return -depth;
	return hits;
}

int32_t searchGumption(GumpSearchContext* sc, Rect rect, const int32_t count, Point* out_points) {
	sc->trim->lx = rect.lx; sc->trim->hx = rect.hx;
	sc->trim->ly = rect.ly; sc->trim->hy = rect.hy;
	if (sc->trim->lx < sc->bounds->lx) sc->trim->lx = sc->bounds->lx;
	if (sc->trim->hx > sc->bounds->hx) sc->trim->hx = sc->bounds->hx;
	if (sc->trim->ly < sc->bounds->ly) sc->trim->ly = sc->bounds->ly;
	if (sc->trim->hy > sc->bounds->hy) sc->trim->hy = sc->bounds->hy;

	// run range search
	int hits = regionHits(sc, *sc->trim, sc->root, count, out_points, 1);
	if (hits > 0) return hits;

	// if range search fails, fall back on grid or binary
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
	int i = floor(di - 0.001); if (i < 0) i = 0;
	int j = floor(dj - 0.001); if (j < 0) j = 0;
	int p = ceil(dp); if (p > DIVS) p = DIVS;
	int q = ceil(dq); if (q > DIVS) q = DIVS;
	int w = p - i;
	int h = q - j;

	int maxtests = 0;
	for (int a = 0; a < w; a++) {
		for (int b = 0; b < h; b++) {
			maxtests += sc->rlen[i+a][j+b];
		}
	}

	if ((float)maxtests * GRIDFACTOR < (float)(nx < ny ? nx : ny)) {
		if (w == 1 && h == 1) return findHitsS((Rect*)&rect, sc->grid[i][j], sc->rlen[i][j], out_points, count);
		else return findHitsB(sc, (Rect*)&rect, i, j, w, h, out_points, count);
	} else {
		if (nx < ny) return findHitsU((Rect*)&rect, &sc->xsort[xidxl], nx, out_points, count, isHitY);
		else return findHitsU((Rect*)&rect, &sc->ysort[yidxl], ny, out_points, count, isHitX);
	}
}

// hitchecks = -1;
// if (w*h < 100) {
// 	if (w == 1 && h == 1) hits = findHitsS((Rect*)&rect, sc->grid[i][j], sc->rlen[i][j], out_points, count);
// 	else hits = findHitsB(sc, (Rect*)&rect, i, j, w, h, out_points, count);
// }
// int gridchecks = hitchecks;

// hitchecks = 0;
// if (nx < ny) hits = rangeHits(sc, rect, sc->xroot, xidxl, xidxr, count, out_points, 1);
// else hits = rangeHits(sc, rect, sc->yroot, yidxl, yidxr, count, out_points, 1);
// int rangechecks = hits < 0 ? -1 : hitchecks;

// hitchecks = 0;
// if (nx < ny) hits = findHitsU((Rect*)&rect, &sc->xsort[xidxl], nx, out_points, count, isHitY);
// else hits = findHitsU((Rect*)&rect, &sc->ysort[yidxl], ny, out_points, count, isHitX);
// int binarychecks = hitchecks;

// if (WRITEFILES) {
// 	FILE *f = fopen("rects.csv", "a");
// 	fprintf(f, "%f,%f,%f,%f,%d,%d,%d,%d,%d,%d,%d,%d\n",
// 		rect.lx, rect.hx, rect.ly, rect.hy,
// 		nx, ny, w, h, maxtests,
// 		gridchecks, rangechecks, binarychecks
// 	);
// 	fclose(f);
// }



// DLL IMPLEMENTATION -----------------------------------------------------------------------------

int regions = 0;

Region* buildRegion(GumpSearchContext* sc, Rect* rect, Region* lover, Region* lrover, Region* rover, Region* bover, Region* btover, Region* tover, int depth) {
	regions++;
	Region* region = (Region*)malloc(sizeof(Region));
	region->rect     = rect;
	region->left     = NULL;
	region->right    = NULL;
	region->lrmid    = NULL;
	region->bottom   = NULL;
	region->top      = NULL;
	region->btmid    = NULL;
	region->ranksort = NULL;

	// is this a leaf
	if (depth == MAXDEPTH) {
		region->ranksort = (Point*)calloc(LEAFSIZE, sizeof(Point));
		region->n = searchBinary(sc, *rect, LEAFSIZE, region->ranksort);
		return region;
	}

	region->ranksort = (Point*)calloc(NODESIZE, sizeof(Point));
	region->n = searchBinary(sc, *rect, NODESIZE, region->ranksort);

	// build child regions
	float xmid = (rect->lx + rect->hx) / 2;
	float ymid = (rect->ly + rect->hy) / 2;
	float xq1  = (rect->lx + xmid) / 2;
	float xq3  = (xmid + rect->hx) / 2;
	float yq1  = (rect->ly + ymid) / 2;
	float yq3  = (ymid + rect->hy) / 2;
	Rect* leftrect   = (Rect*)malloc(sizeof(Rect)); leftrect->lx = rect->lx;   leftrect->hx = xmid;       leftrect->ly = rect->ly;   leftrect->hy = rect->hy;
	Rect* rightrect  = (Rect*)malloc(sizeof(Rect)); rightrect->lx = xmid;      rightrect->hx = rect->hx;  rightrect->ly = rect->ly;  rightrect->hy = rect->hy;
	Rect* lrmidrect  = (Rect*)malloc(sizeof(Rect)); lrmidrect->lx = xq1;       lrmidrect->hx = xq3;       lrmidrect->ly = rect->ly;  lrmidrect->hy = rect->hy;
	Rect* bottomrect = (Rect*)malloc(sizeof(Rect)); bottomrect->lx = rect->lx; bottomrect->hx = rect->hx; bottomrect->ly = rect->ly; bottomrect->hy = ymid;
	Rect* toprect    = (Rect*)malloc(sizeof(Rect)); toprect->lx = rect->lx;    toprect->hx = rect->hx;    toprect->ly = ymid;        toprect->hy = rect->hy;
	Rect* btmidrect  = (Rect*)malloc(sizeof(Rect)); btmidrect->lx = rect->lx;  btmidrect->hx = rect->hx;  btmidrect->ly = yq1;       btmidrect->hy = yq3;
	region->left   = lover  ? lover  : buildRegion(sc, leftrect,   NULL, NULL, NULL, NULL, NULL, NULL, depth+1);
	region->right  = rover  ? rover  : buildRegion(sc, rightrect,  NULL, NULL, NULL, NULL, NULL, NULL, depth+1);
	region->lrmid  = lrover ? lrover : buildRegion(sc, lrmidrect,  region->left->right, NULL, region->right->left, NULL, NULL, NULL, depth+1);
	region->bottom = bover  ? bover  : buildRegion(sc, bottomrect, region->left->bottom, region->lrmid->bottom, region->right->bottom, NULL, NULL, NULL, depth+1);
	region->top    = tover  ? tover  : buildRegion(sc, toprect,    region->left->top, region->lrmid->top, region->right->top, NULL, NULL, NULL, depth+1);
	region->btmid  = btover ? btover : buildRegion(sc, btmidrect,  region->left->btmid, region->lrmid->btmid, region->right->btmid, region->bottom->top, NULL, region->top->bottom, depth+1);

	return region;
}

void freeRegion(Region* region, bool left, bool lrmid, bool right, bool bottom, bool btmid, bool top) {
	if (left   && region->left)   freeRegion(region->left,   true,  true,  true,  true,  true, true);
	if (right  && region->right)  freeRegion(region->right,  true,  true,  true,  true,  true, true);
	if (lrmid  && region->lrmid)  freeRegion(region->lrmid,  false, true,  false, true,  true, true);
	if (bottom && region->bottom) freeRegion(region->bottom, false, false, false, true,  true, true);
	if (top    && region->top)    freeRegion(region->top,    false, false, false, true,  true, true);
	if (btmid  && region->btmid)  freeRegion(region->btmid,  false, false, false, false, true, false);
	if (region->ranksort) free(region->ranksort);
	free(region);
}

void buildGrid(GumpSearchContext* sc) {
	sc->blocks = (Point**)calloc(100, sizeof(Point*));
	sc->blocki = (int*)calloc(100, sizeof(int));
	sc->blockn = (int*)calloc(100, sizeof(int));

	sc->dx = (double)(sc->bounds->hx - sc->bounds->lx) / (double)DIVS;
	sc->dy = (double)(sc->bounds->hy - sc->bounds->ly) / (double)DIVS;
	DPRINT(("Bounds are [%f,%f,%f,%f]: dx=%f, dy=%f, area %f\n",
		sc->bounds->lx, sc->bounds->hx, sc->bounds->ly, sc->bounds->hy,
		sc->dx, sc->dy, sc->area
	));

	memcpy(sc->gridsort, sc->xsort, sc->N * sizeof(Point));
	sc->grid = (Point***)calloc(DIVS, sizeof(Point**));
	sc->rlen = (int**)calloc(DIVS, sizeof(int*));
	sc->rect = (Rect**)calloc(DIVS, sizeof(Rect*));
	int xidxl = 0;
	for (int i = 0; i < DIVS; i++) {
		double lx = sc->bounds->lx + (double)i * sc->dx;
		double hx = sc->bounds->lx + (double)(i+1) * sc->dx;
		if (i == DIVS - 1) hx = sc->bounds->hx;
		int xidxr = xidxl + bsearchx(&sc->gridsort[xidxl], false, hx, 0, sc->N - xidxl + 1);
		int nx = xidxr - xidxl + 1;
		qsort(&sc->gridsort[xidxl], nx, sizeof(Point), ycomp);

		sc->grid[i] = (Point**)calloc(DIVS, sizeof(Point*));
		sc->rlen[i] = (int*)calloc(DIVS, sizeof(int));
		sc->rect[i] = (Rect*)calloc(DIVS, sizeof(Rect));
		int yidxl = xidxl;
		for (int j = 0; j < DIVS; j++) {
			double ly = sc->bounds->ly + (double)j * sc->dy;
			double hy = sc->bounds->ly + (double)(j+1) * sc->dy;
			if (j == DIVS - 1) hy = sc->bounds->hy;
			int yidxr = yidxl + bsearchy(&sc->gridsort[yidxl], false, hy, 0, xidxr - yidxl + 1);
			int ny = yidxr - yidxl + 1;

			if (ny == 0) {
				sc->rlen[i][j] = 0;
				sc->grid[i][j] = NULL;
			} else {
				sc->rlen[i][j] = ny;
				sc->grid[i][j] = (Point*)calloc(ny, sizeof(Point));
				memcpy(sc->grid[i][j], &sc->gridsort[yidxl], ny * sizeof(Point));
				qsort(sc->grid[i][j], ny, sizeof(Point), rankcomp);
			}
			sc->rect[i][j].lx = lx;
			sc->rect[i][j].ly = ly;
			sc->rect[i][j].hx = hx;
			sc->rect[i][j].hy = hy;

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
	for (int i = 0; i < DIVS; i++) {
		for (int j = 0; j < DIVS; j++) {
			free(sc->grid[i][j]);
		}
		free(sc->grid[i]);
		free(sc->rlen[i]);
		free(sc->rect[i]);
	}
	free(sc->grid);
	free(sc->rlen);
	free(sc->rect);
	free(sc->bounds);
	free(sc->blocks);
	free(sc->blocki);
	free(sc->blockn);
}

__stdcall SearchContext* create(const Point* points_begin, const Point* points_end) {
	GumpSearchContext* sc = (GumpSearchContext*)malloc(sizeof(GumpSearchContext));
	sc->N = points_end - points_begin;
	if (sc->N == 0) return (SearchContext*)sc;

	sc->xsort = (Point*)calloc(sc->N, sizeof(Point));
	sc->ysort = (Point*)calloc(sc->N, sizeof(Point));
	sc->trim = (Rect*)malloc(sizeof(Rect));
	sc->gridsort = (Point*)calloc(sc->N, sizeof(Point));
	memcpy(sc->xsort, points_begin, sc->N * sizeof(Point));
	memcpy(sc->ysort, points_begin, sc->N * sizeof(Point));
	memcpy(sc->gridsort, points_begin, sc->N * sizeof(Point));
	qsort(sc->xsort, sc->N, sizeof(Point), xcomp);
	qsort(sc->ysort, sc->N, sizeof(Point), ycomp);
	qsort(sc->gridsort, sc->N, sizeof(Point), rankcomp);

	sc->bounds = (Rect*)malloc(sizeof(Rect));
	sc->bounds->lx = sc->xsort[1].x;
	sc->bounds->hx = sc->xsort[sc->N-2].x;
	sc->bounds->ly = sc->ysort[1].y;
	sc->bounds->hy = sc->ysort[sc->N-2].y;
	sc->area = rectArea(sc->bounds);

	printf("Building region tree\n");
	sc->root = buildRegion(sc, sc->bounds, NULL, NULL, NULL, NULL, NULL, NULL, 1);
	printf("Building grid tree\n");
	buildGrid(sc);
	free(sc->gridsort);

	return (SearchContext*)sc;
}

__stdcall int32_t search(SearchContext* sc, Rect rect, const int32_t count, Point* out_points) {
	GumpSearchContext* context = (GumpSearchContext*)sc;
	if (context->N == 0) return 0;
	return searchGumption(context, rect, count, out_points);
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