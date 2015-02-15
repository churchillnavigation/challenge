// OLD CODE

void printContext(GumpSearchContext* sc) {
	int i = 0;
	printf("\n%d Points\n", sc->N);
	printf("xsort:\n");
	for (i = 0; i < sc->N; i++) {
		printf("%d, %d, %f, %f\n", sc->xsort[i].id, sc->xsort[i].rank, sc->xsort[i].x, sc->xsort[i].y);
	}
	printf("ysort:\n");
	for (i = 0; i < sc->N; i++) {
		printf("%d, %d, %f, %f\n", sc->ysort[i].id, sc->ysort[i].rank, sc->ysort[i].x, sc->ysort[i].y);
	}
	printf("ranksort:\n");
	for (i = 0; i < sc->N; i++) {
		printf("%d, %d, %f, %f\n", sc->ranksort[i].id, sc->ranksort[i].rank, sc->ranksort[i].x, sc->ranksort[i].y);
	}
}

int _rankmax = 0;

inline bool isHitXMinRank(const Rect* r, Point* p) {
	return p->rank < _rankmax && p->x >= r->lx && p->x <= r->hx;
}

inline bool isHitYMinRank(const Rect* r, Point* p) {
	return p->rank < _rankmax && p->y >= r->ly && p->y <= r->hy;
}

inline bool isRectInside(Rect* r1, Rect* r2) {
	return r2->lx >= r1->lx && r2->lx <= r1->hx && r2->ly >= r1->ly && r2->ly <= r1->hy &&
	       r2->hx >= r1->lx && r2->hx <= r1->hx && r2->hy >= r1->ly && r2->hy <= r1->hy;
}

inline bool isRectOverlap(Rect* r1, Rect* r2) {
	return r1->lx < r2->hx && r1->hx > r2->lx && r1->ly < r2->hy && r1->hy > r2->ly;
}

int intersect(Point p1[], Point p2[], int n1, int n2, int max, Point* out) {
	int i = 0, j = 0, n = 0;
	qsort(p1, n1, sizeof(Point), rankCompare);
	qsort(p2, n2, sizeof(Point), rankCompare);
	while (i < n1 && j < n2) {
		int rank1 = p1[i].rank;
		int rank2 = p2[j].rank;
		if (rank1 < rank2) i++;
		else if (rank2 < rank1) j++;
		else {
			out[n] = p1[i];
			i++; j++; n++;
			if (n >= max) return n;
		}
	}
	return n;
}

// tree search - build quadtree and narrow use precomputed solutions as starting point
void treeHitsA(GumpSearchContext* sc, TreeNode* node, const Rect rect) {
	// if this node is fully contained within the rect, return this nodes hits
	if (isRectInside((Rect*)(&rect), node->rect)) {
		memcpy(node->childPoints, node->myPoints, node->myN * sizeof(Point));
		node->childN = node->myN;
		if (sc->treeximin == -1 || node->ximin < sc->treeximin) sc->treeximin = node->ximin;
		if (sc->treeximax == -1 || node->ximax > sc->treeximax) sc->treeximax = node->ximax;
		if (sc->treeyimin == -1 || node->yimin < sc->treeyimin) sc->treeyimin = node->yimin;
		if (sc->treeyimax == -1 || node->yimax > sc->treeyimax) sc->treeyimax = node->yimax;
		return;
	}

	// if this node doesn't overlap the rect at all, return null
	if (!isRectOverlap((Rect*)(&rect), node->rect)) {
		node->childN = 0;
		return;
	}

	// recurse on the subtrees
	if (node->tr) treeHitsA(sc, node->tr, rect);
	if (node->tl) treeHitsA(sc, node->tl, rect);
	if (node->bl) treeHitsA(sc, node->bl, rect);
	if (node->br) treeHitsA(sc, node->br, rect);

	// merge results of children
	int tri = 0, tli = 0, bli = 0, bri = 0;
	int trr = RANK_MAX;
	int tlr = RANK_MAX;
	int blr = RANK_MAX;
	int brr = RANK_MAX;
	node->childN = 0;
	while (node->childN < MAXRESLEN) {
		if (node->tr && tri < node->tr->childN) trr = node->tr->childPoints[tri].rank; else trr = RANK_MAX;
		if (node->tl && tli < node->tl->childN) tlr = node->tl->childPoints[tli].rank; else tlr = RANK_MAX;
		if (node->bl && bli < node->bl->childN) blr = node->bl->childPoints[bli].rank; else blr = RANK_MAX;
		if (node->br && bri < node->br->childN) brr = node->br->childPoints[bri].rank; else brr = RANK_MAX;

		if (trr == RANK_MAX && tlr == RANK_MAX && blr == RANK_MAX && brr == RANK_MAX) break;

		if      (trr < tlr && trr < blr && trr < brr) { node->childPoints[node->childN] = node->tr->childPoints[tri]; tri++; }
		else if (tlr < trr && tlr < blr && tlr < brr) { node->childPoints[node->childN] = node->tl->childPoints[tli]; tli++; }
		else if (blr < trr && blr < tlr && blr < brr) { node->childPoints[node->childN] = node->bl->childPoints[bli]; bli++; }
		else if (brr < trr && brr < tlr && brr < blr) { node->childPoints[node->childN] = node->br->childPoints[bri]; bri++; }

		node->childN++;
	}
}

int32_t searchTreeA(GumpSearchContext* sc, const Rect rect, const int32_t count, Point* out_points) {
	// find x and y index bounds
	int ximin = bsearch(sc->xsort, true, true, rect.lx, 0, sc->N);
	int ximax = bsearch(sc->xsort, true, false, rect.hx, 0, sc->N);
	int yimin = bsearch(sc->ysort, false, true, rect.ly, 0, sc->N);
	int yimax = bsearch(sc->ysort, false, false, rect.hy, 0, sc->N);
	int nx = ximax - ximin + 1;
	int ny = yimax - yimin + 1;

	if (nx == 0 || ny == 0) return 0;

	int smallside = nx < ny ? nx : ny;
	float ratio = (float)smallside / (float)sc->N;
	if (ratio > .1f) return searchBaseline(sc, rect, count, out_points);
	else if (smallside < 2*MINBOX) {
		if (nx < ny) return findHitsU(&rect, &sc->xsort[ximin], nx, out_points, count, isHitY);
		else return findHitsU(&rect, &sc->ysort[yimin], ny, out_points, count, isHitX);
	} else {
		sc->treeximin = -1;
		sc->treeximax = -1;
		sc->treeyimin = -1;
		sc->treeyimax = -1;

		// find hits in tree
		treeHitsA(sc, sc->root, rect);

		// TODO results incorrect if no fully contained node found

		// find the leftover hits - the ones outside of the treenodes
		_rankmax = sc->root->childN > 0 ? sc->root->childPoints[sc->root->childN-1].rank : RANK_MAX;
		sc->lrect->lx = sc->xsort[ximin].x,
		sc->lrect->ly = sc->ysort[yimin].y,
		sc->lrect->hx = sc->xsort[sc->treeximin-1].x,
		sc->lrect->hy = sc->ysort[yimax].y;
		int lnx = sc->treeximin - ximin;
		int lny = yimax - yimin + 1;
		int ln = 0;
		if (lnx < lny) ln = findHitsU(sc->lrect, &sc->xsort[ximin], lnx, sc->lbox, count, isHitYMinRank);
		else ln = findHitsU(sc->lrect, &sc->ysort[yimin], lny, sc->lbox, count, isHitXMinRank);

		sc->rrect->lx = sc->xsort[sc->treeximax+1].x,
		sc->rrect->ly = sc->ysort[yimin].y,
		sc->rrect->hx = sc->xsort[ximax].x,
		sc->rrect->hy = sc->ysort[yimax].y;
		int rnx = ximax - sc->treeximax + 2;
		int rny = yimax - yimin + 1;
		int rn = 0;
		if (rnx < rny) rn = findHitsU(sc->rrect, &sc->xsort[sc->treeximax+1], rnx, sc->rbox, count, isHitYMinRank);
		else rn = findHitsU(sc->rrect, &sc->ysort[yimin], rny, sc->rbox, count, isHitYMinRank);

		sc->trect->lx = sc->xsort[sc->treeximin].x,
		sc->trect->ly = sc->ysort[sc->treeyimax+1].y,
		sc->trect->hx = sc->xsort[sc->treeximax].x,
		sc->trect->hy = sc->ysort[yimax].y;
		int tnx = sc->treeximax - sc->treeximin + 1;
		int tny = yimax - sc->treeyimax + 2;
		int tn = 0;
		if (tnx < tny) tn = findHitsU(sc->trect, &sc->xsort[sc->treeximin], tnx, sc->tbox, count, isHitYMinRank);
		else tn = findHitsU(sc->trect, &sc->ysort[sc->treeyimax+1], tny, sc->tbox, count, isHitXMinRank);

		sc->brect->lx = sc->xsort[sc->treeximin].x,
		sc->brect->ly = sc->ysort[yimin].y,
		sc->brect->hx = sc->xsort[sc->treeximax].x,
		sc->brect->hy = sc->ysort[sc->treeyimin-1].y;
		int bnx = sc->treeximax - sc->treeximin + 1;
		int bny = sc->treeyimin - yimin;
		int bn = 0;
		if (bnx < bny) bn = findHitsU(sc->brect, &sc->xsort[sc->treeximin], bnx, sc->bbox, count, isHitYMinRank);
		else bn = findHitsU(sc->brect, &sc->ysort[yimin], bny, sc->bbox, count, isHitXMinRank);

		int N = 0;
		int ni = 0, li = 0, ri = 0, ti = 0, bi = 0;
		int nr = RANK_MAX, lr = RANK_MAX, rr = RANK_MAX, tr = RANK_MAX, br = RANK_MAX;
		while (N < MAXRESLEN) {
			if (ni < sc->root->childN) nr = sc->root->childPoints[ni].rank; else nr = RANK_MAX;
			if (li < ln) lr = sc->lbox[li].rank; else lr = RANK_MAX;
			if (ri < rn) rr = sc->rbox[ri].rank; else rr = RANK_MAX;
			if (ti < tn) tr = sc->tbox[ti].rank; else tr = RANK_MAX;
			if (bi < bn) br = sc->bbox[bi].rank; else br = RANK_MAX;

			if (nr == RANK_MAX && lr == RANK_MAX && rr == RANK_MAX && tr == RANK_MAX && br == RANK_MAX) break;

			if      (nr < lr && nr < rr && nr < tr && nr < br) { out_points[N] = sc->root->childPoints[ni]; ni++; }
			else if (lr < nr && lr < rr && lr < tr && lr < br) { out_points[N] = sc->lbox[li]; li++; }
			else if (rr < nr && rr < lr && rr < tr && rr < br) { out_points[N] = sc->rbox[ri]; ri++; }
			else if (tr < nr && tr < lr && tr < rr && tr < br) { out_points[N] = sc->tbox[ti]; ti++; }
			else if (br < nr && br < lr && br < rr && br < tr) { out_points[N] = sc->bbox[bi]; bi++; }

			N++;
		}

		return N;
	}
}

int32_t treeHitsB(GumpSearchContext* sc, TreeNode* node, const Rect rect, int32_t count, Point* out_points) {
	// check immediate children
	if (isRectInside(node->tr->rect, (Rect*)(&rect))) return treeHitsB(sc, node->tr, rect, count, out_points);
	else if (isRectInside(node->tl->rect, (Rect*)(&rect))) return treeHitsB(sc, node->tl, rect, count, out_points);
	else if (isRectInside(node->bl->rect, (Rect*)(&rect))) return treeHitsB(sc, node->bl, rect, count, out_points);
	else if (isRectInside(node->br->rect, (Rect*)(&rect))) return treeHitsB(sc, node->br, rect, count, out_points);

	// if not contained in any immediate children, check self
	if (isRectInside(node->rect, (Rect*)(&rect))) {
		int hits = findHitsS(&rect, node->my5xPoints, node->my5xN, out_points, count, isHit);
		if (hits < count && node->my5xN > count) {
			int nx = node->ximax - node->ximin + 1;
			int ny = node->yimax - node->yimin + 1;
			if (nx < ny) return findHitsU(&rect, &sc->xsort[node->ximin], nx, out_points, count, isHit);
			else return findHitsU(&rect, &sc->ysort[node->yimin], ny, out_points, count, isHit);
		} else return hits;
	}

	return searchBaseline(sc, rect, count, out_points);
}

int32_t searchTreeB(GumpSearchContext* sc, const Rect rect, const int32_t count, Point* out_points) {
	// find x and y index bounds
	int ximin = bsearch(sc->xsort, true, true, rect.lx, 0, sc->N);
	int ximax = bsearch(sc->xsort, true, false, rect.hx, 0, sc->N);
	int yimin = bsearch(sc->ysort, false, true, rect.ly, 0, sc->N);
	int yimax = bsearch(sc->ysort, false, false, rect.hy, 0, sc->N);
	int nx = ximax - ximin + 1;
	int ny = yimax - yimin + 1;

	// printf("Query rect [%f,%f,%f,%f] (%d,%d,%d,%d)\n",
	// 	rect.lx, rect.hx, rect.ly, rect.hy,
	// 	ximin, ximax, yimin, yimax
	// );

	if (nx == 0 || ny == 0) return 0;

	int smallside = nx < ny ? nx : ny;
	float ratio = (float)smallside / (float)sc->N;
	if (ratio > .1f) return searchBaseline(sc, rect, count, out_points);
	else if (smallside < 2*MINBOX) {
		if (nx < ny) return findHitsU(&rect, &sc->xsort[ximin], nx, out_points, count, isHitY);
		else return findHitsU(&rect, &sc->ysort[yimin], ny, out_points, count, isHitX);
	} else return treeHitsB(sc, sc->root, rect, count, out_points);
}




int nodes = 0;

TreeNode* buildNode(GumpSearchContext* sc, int ximin, int ximax, int yimin, int yimax) {
	nodes++;
	TreeNode* node = (TreeNode*)malloc(sizeof(TreeNode));
	node->ximin = ximin;
	node->ximax = ximax;
	node->yimin = yimin;
	node->yimax = yimax;
	node->tr = NULL;
	node->tl = NULL;
	node->bl = NULL;
	node->br = NULL;

	// compute top MAXRESLEN in this region
	int nx = ximax - ximin + 1;
	int ny = yimax - yimin + 1;
	const struct Rect r = {
		.lx = sc->xsort[ximin].x,
		.ly = sc->ysort[yimin].y,
		.hx = sc->xsort[ximax].x,
		.hy = sc->ysort[yimax].y
	};

	node->rect = (Rect*)malloc(sizeof(Rect));
	node->rect->lx = r.lx;
	node->rect->ly = r.ly;
	node->rect->hx = r.hx;
	node->rect->hy = r.hy;
	//node->myPoints = (Point*)calloc(MAXRESLEN, sizeof(Point));
	//node->myN = searchBinary(sc, r, MAXRESLEN, node->myPoints);
	//node->childPoints = (Point*)calloc(MAXRESLEN, sizeof(Point));
	//node->childN = 0;
	node->my5xPoints = (Point*)calloc(5*MAXRESLEN, sizeof(Point));
	node->my5xN = searchBinary(sc, r, 5*MAXRESLEN, node->my5xPoints);

	// stop recursion if fewer than 100 points in x or y array
	if (nx < MINBOX || ny < MINBOX) return node;

	int ximed = (ximin + ximax) / 2;
	int yimed = (yimin + yimax) / 2;
	// don't split on same x or y val
	float curx = sc->xsort[ximed].x;
	float nextx = sc->xsort[ximed+1].x;
	while (ximed < ximax && curx == nextx) {
		ximed++;
		curx = sc->xsort[ximed].x;
		nextx = sc->xsort[ximed+1].x;
	}
	float cury = sc->ysort[yimed].y;
	float nexty = sc->ysort[yimed+1].y;
	while (yimed < yimax && cury == nexty) {
		yimed++;
		cury = sc->ysort[yimed].y;
		nexty = sc->ysort[yimed+1].y;
	}

	// build out children
	node->tr = buildNode(sc, ximed + 1, ximax, yimed + 1, yimax);
	node->tl = buildNode(sc, ximin,     ximed, yimed + 1, yimax);
	node->bl = buildNode(sc, ximin,     ximed, yimin,     yimed);
	node->br = buildNode(sc, ximed + 1, ximax, yimin,     yimed);

	return node;
}

void freeNode(TreeNode* node) {
	if (node->tr) freeNode(node->tr);
	if (node->tl) freeNode(node->tl);
	if (node->bl) freeNode(node->bl);
	if (node->br) freeNode(node->br);
	free(node->myPoints);
	free(node->childPoints);
	free(node->my5xPoints);
	free(node->rect);
	free(node);
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
	qsort(sc->xsort, sc->N, sizeof(Point), xCompare);
	qsort(sc->ysort, sc->N, sizeof(Point), yCompare);
	qsort(sc->ranksort, sc->N, sizeof(Point), rankCompare);
	sc->root = buildNode(sc, 0, sc->N-1, 0, sc->N-1);
	sc->lrect = (Rect*)malloc(sizeof(Rect));
	sc->rrect = (Rect*)malloc(sizeof(Rect));
	sc->trect = (Rect*)malloc(sizeof(Rect));
	sc->brect = (Rect*)malloc(sizeof(Rect));
	sc->lbox = (Point*)calloc(MAXRESLEN, sizeof(Point));
	sc->rbox = (Point*)calloc(MAXRESLEN, sizeof(Point));
	sc->tbox = (Point*)calloc(MAXRESLEN, sizeof(Point));
	sc->bbox = (Point*)calloc(MAXRESLEN, sizeof(Point));

	printf("\nBuilt %d ranges\n", ranges);

	// FILE *f = fopen("points.csv", "w");
	// for (int i = 0; i < sc->N; i++) {
	// 	fprintf(f, "%d,%f,%f\n", sc->ranksort[i].rank, sc->ranksort[i].x, sc->ranksort[i].y);
	// }
	// fclose(f);

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
	free(context->ranksort);
	freeNode(context->root);
	free(context->lrect);
	free(context->rrect);
	free(context->trect);
	free(context->brect);
	free(context->lbox);
	free(context->rbox);
	free(context->tbox);
	free(context->bbox);
	return NULL;
}



struct TreeNode {
	int ximin, ximax;
	int yimin, yimax;
	Rect* rect;
	TreeNode* tr;
	TreeNode* tl;
	TreeNode* bl;
	TreeNode* br;
	Point* myPoints;
	int myN;
	Point* childPoints;
	int childN;
	Point* my5xPoints;
	int my5xN;
};

struct GumpSearchContext {
	int32_t N;
	Point* xsort;
	Point* ysort;
	Point* ranksort;
	TreeNode* root;
	Rect *lrect, *rrect, *trect, *brect;
	Point *lbox, *rbox, *tbox, *bbox;
	int treeximin, treeximax;
	int treeyimin, treeyimax;
};