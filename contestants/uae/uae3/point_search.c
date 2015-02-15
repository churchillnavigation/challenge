#include "point_search.h"
#include <stdio.h>
#include <stdlib.h>
#include <float.h>
#include <windows.h>

#define assert(x) if (!(x)) {printf("\nASSERT line %d\n", __LINE__); exit(0);}
#define MIN(x, y) (((x) < (y))?(x):(y))
#define MAX(x, y) (((x) > (y))?(x):(y))

#define DIRECTSEARCH 0.45
#define DEPTH 9
#define WIDTH 2
#define WIDTH2 (WIDTH * WIDTH)
#define ALIGN(x) ((x + 127) & (~127))
#define MAXREQUEST 29
#define LENGTH1 2200 // must bigger than WIDTH ^ DEPTH
#define FAILURE (-1)

// start address for malloc
char *MemBegin;

struct Node {
	int id;
	int rank;
	float x, y;
} *node;
int n;

int *rank[DEPTH + 1];

#define XYBIT 12
#define XYBIT2 (XYBIT + XYBIT)
#define XYMASK ((1 << XYBIT) - 1)
#define DBIT (32 - XYBIT2)
#define DMASK ((1 << DBIT) - 1)
#define GENID(depth, x, y) (((depth) << XYBIT2) | ((x) << XYBIT) | (y))
#define IDDEPTH(d) (((d) >> XYBIT2) & DMASK)
#define IDX(x) (((x) >> XYBIT) & XYMASK)
#define IDY(y) ((y) & XYMASK)
struct Square {
	int id, begin, end;
	float p;
} *square;
int m;

// Global values
Rect gRect;
int gCount;
struct PoolSquare{
	int v, s, idx;
} p1[MAXREQUEST + 2];
int pool[MAXREQUEST + 2];
#define BUFFERLEN 100000
int *squareBuffer;

// Ruler
float rx[LENGTH1], ry[LENGTH1];
int size[DEPTH + 1], square0[DEPTH + 1], length1;

typedef struct SearchContext {
	void *nothing;
};

void sortCount(int *p, int l, int r) {
	int i = l;
	int j = r;
	int mid = p[(l + r) / 2];
	while (i <= j) {
		while (p[i] < mid) i++;
		while (p[j] > mid) j--;
		if (i <= j) {
			int tmp = p[i];
			p[i] = p[j];
			p[j] = tmp;
			i++;
			j--;
		}
	}
	if (i < r) sortCount(p, i, r);
	if (l < j) sortCount(p, l, j);
}

void sortNode(struct Node *p, int l, int r) {
	int i = l;
	int j = r;
	int mid = p[(l + r) / 2].rank;
	while (i <= j) {
		while (p[i].rank < mid) i++;
		while (p[j].rank > mid) j--;
		if (i <= j) {
			struct Node tmp = p[i];
			p[i] = p[j];
			p[j] = tmp;
			i++;
			j--;
		}
	}
	if (i < r) sortNode(p, i, r);
	if (l < j) sortNode(p, l, j);
}

void sortFloat (float *p, int l, int r) {
	int i = l;
	int j = r;
	float mid = p[(l + r) / 2];
	while (i <= j) {
		while (p[i] < mid) i++;
		while (p[j] > mid) j--;
		if (i <= j) {
			float tmp = p[i];
			p[i] = p[j];
			p[j] = tmp;
			i++;
			j--;
		}
	}
	if (i < r) sortFloat(p, i, r);
	if (l < j) sortFloat(p, l, j);
}

int inSquare(int ids, int idn) {
	int idx = IDX(square[ids].id);
	int idy = IDY(square[ids].id);
	int l = size[IDDEPTH(square[ids].id)];
	float xx = node[idn].x;
	float yy = node[idn].y;
	return (rx[idx] <= xx && xx < rx[idx + l] && ry[idy] <= yy && yy < ry[idy + l]);
}

int squareTest(int s) {
	int depth, flag;
	float lx, ly, hx, hy;
	if (square[s].end == square[s].begin) return FAILURE;
	depth = IDDEPTH(square[s].id);
	if (p1[0].s == gCount && rank[depth][square[s].begin] >= p1[1].v) return FAILURE;
	lx = rx[IDX(square[s].id)];
	hx = rx[IDX(square[s].id) + size[depth]];
	ly = ry[IDY(square[s].id)];
	hy = ry[IDY(square[s].id) + size[depth]];
	if (hx <= gRect.lx || lx > gRect.hx || hy <= gRect.ly || ly > gRect.hy) return FAILURE;
	flag = 0;
	if (gRect.lx <= lx && hx <= gRect.hx && gRect.ly <= ly && hy <= gRect.hy) flag = 1;
	if (depth == DEPTH || (MIN(hx, gRect.hx) - MAX(lx, gRect.lx)) * (MIN(hy, gRect.hy) - MAX(ly, gRect.ly)) > square[s].p * DIRECTSEARCH) flag |= 2;
	return flag;
}

void updateP1(int zz, int s) {
	int i, j;
	if (p1[0].s < gCount) {
		p1[0].s += 1;
		i = p1[0].s;
		j = i >> 1;
		while (j && p1[j].v < zz) {
			p1[i] = p1[j];
			i = j;
			j = i >> 1;
		}
		p1[i].v = zz;
		p1[i].s = s;
	}
	else {
		i = 1;
		while ((j = (i << 1)) <= p1[0].s) {
			if (j < p1[0].s && p1[j].v < p1[j + 1].v) j += 1;
			if (p1[j].v <= zz) break;
			p1[i] = p1[j];
			i = j;
		}
		p1[i].v = zz;
		p1[i].s = s;
	}
}

void updatePool(int zz) {
	int i, j;
	if (pool[0] < gCount) {
		pool[0] += 1;
		i = pool[0];
		j = i >> 1;
		while (j && pool[j] < zz) {
			pool[i] = pool[j];
			i = j;
			j = i >> 1;
		}
		pool[i] = zz;
	}
	else {
		i = 1;
		while ((j = (i << 1)) <= pool[0]) {
			if (j < pool[0] && pool[j] < pool[j + 1]) j += 1;
			if (pool[j] <= zz) break;
			pool[i] = pool[j];
			i = j;
		}
		pool[i] = zz;
	}
}

void dfs(int s) {
	int depth = IDDEPTH(square[s].id) + 1;
	int ch, x;
	ch = (s - square0[depth - 1]) * WIDTH2 + square0[depth];
	x = ch + WIDTH2;
	for (; ch < x; ch++) {
		int y = squareTest(ch);
		if (y == FAILURE) continue;
		else
		if (y == 0) dfs(ch);
		else if (y & 1) {
			updateP1(rank[depth][square[ch].begin], ch);
		}
		else {
			squareBuffer[0]++;
			squareBuffer[squareBuffer[0]] = ch;
		}
	}
}

__declspec(dllexport) SearchContext* create(const Point* points_begin, const Point* points_end)
{
	const Point *p;
	int i, NodeSize, SquareSize, RankSize, BufferSize;

	// Check
	for (length1 = 1, i = 0; i < DEPTH; i++)
		length1 *= WIDTH;
	assert(length1 < LENGTH1);
	assert(length1 <= XYMASK + 1)
	assert(DEPTH <= DMASK)

	// Size Counting
	n = 0;
	for (p = points_begin; p < points_end; p++)
		n++;
	for (m = 1, i = 0; i <= DEPTH; i++)
		m *= WIDTH2;
	m = (m - 1) / (WIDTH2 - 1);
	RankSize = ALIGN(DEPTH * n * sizeof(int));
	NodeSize = ALIGN(n * sizeof(struct Node));
	SquareSize = ALIGN(m * sizeof(struct Square));
	BufferSize = BUFFERLEN * sizeof(int);

	// Ruler
	float *tmp = malloc(sizeof(float) * n);

	for (i = 0; i < n; i++)
		tmp[i] = points_begin[i].x;
	sortFloat(tmp, 0, n - 1);
	for (i = 0; i < length1; i++)
		rx[i] = tmp[(long long)n * i / length1];
	rx[length1] = FLT_MAX;

	for (i = 0; i < n; i++)
		tmp[i] = points_begin[i].y;
	sortFloat(tmp, 0, n - 1);
	for (i = 0; i < length1; i++)
		ry[i] = tmp[(long long)n * i / length1];
	ry[length1] = FLT_MAX;

	free(tmp);

	// Malloc
	MemBegin = malloc(RankSize + NodeSize + SquareSize + BufferSize);
	//memset(MemBegin, 0, RankSize + NodeSize + SquareSize);
	rank[0] = 0;
	for (i = 1; i <= DEPTH; i++)
		rank[i] = (int*)(MemBegin + (i - 1) * n * sizeof(int));
	node = (struct Node*)(MemBegin + RankSize);
	square = (struct Square*)(MemBegin + RankSize + NodeSize);
	squareBuffer = (int *)(MemBegin + RankSize + NodeSize + SquareSize);

	// Data initialize
	for (i = 0; i < n; i++) {
		node[i].id = points_begin[i].id;
		node[i].rank = points_begin[i].rank;
		node[i].x = points_begin[i].x;
		node[i].y = points_begin[i].y;
	}

	size[DEPTH] = 1;
	for (i = DEPTH - 1; i >= 0; i--)
		size[i] = size[i + 1] * WIDTH;
	assert(size[0] == length1); 

	// Data pre-analyze
	sortNode(node, 0, n - 1);

	square0[0] = 0;
	square[0].id = GENID(0, 0, 0);
	square[0].begin = 0;
	square[0].end = n;

	for (i = 1; i <= DEPTH; i++) {
		int j, k;
		int last = size[DEPTH + 1 - i];
		last = last * last + square0[i - 1];
		square0[i] = last;
		for (j = square0[i - 1], k = last; j < last; j++) {
			int x, y, z, ln[WIDTH][WIDTH], ch, count;
			ch = k;
			for (x = 0; x < WIDTH; x++)
			for (y = 0; y < WIDTH; y++, ch++) {
				square[ch].id = GENID(i, IDX(square[j].id) + x * size[i], IDY(square[j].id) + y *size[i]);
				ln[x][y] = 0;
			}
			// count nodes in each sub-square
			for (z = square[j].begin; z < square[j].end; z++) {
				int id;
				if (i == 1) id = z;
				else id = rank[i - 1][z];
				assert(inSquare(j, id));
				ch = k;
				count = 0;
				for (x = 0; x < WIDTH; x++)
				for (y = 0; y < WIDTH; y++, ch++)
				if (inSquare(ch, id)) {
					ln[x][y]++;
					count++;
				}
				assert(count == 1);
			}
			// allocate space in rank array
			ch = k;
			count = square[j].begin;
			for (x = 0; x < WIDTH; x++)
			for (y = 0; y < WIDTH; y++, ch++) {
					square[ch].begin = square[ch].end = count;
					count += ln[x][y];
					square[ch].p = ((rx[IDX(square[ch].id) + size[i]] - rx[IDX(square[ch].id)]) * 
							        (ry[IDY(square[ch].id) + size[i]] - ry[IDY(square[ch].id)]));
			}
			assert(count == square[j].end);
			// allocation
			for (z = square[j].begin; z < square[j].end; z++) {
				int id;
				if (i == 1) id = z;
				else id = rank[i - 1][z];
				ch = k;
				for (x = 0; x < WIDTH; x++)
				for (y = 0; y < WIDTH; y++, ch++)
				if (inSquare(ch, id)) {
					rank[i][square[ch].end] = id;
					square[ch].end += 1;
					ln[x][y]--;
				}
			}
			// check
			ch = k;
			count = square[j].begin;
			for (x = 0; x < WIDTH; x++)
			for (y = 0; y < WIDTH; y++, ch++) {
				assert(ln[x][y] == 0);
				assert(square[ch].begin == count);
				count = square[ch].end;
			}
			assert(count == square[j].end);
			// next iteration
			k += WIDTH2;
		}
	}
	return NULL;
}

__declspec(dllexport) int32_t search(SearchContext* sc, const Rect rect, const int32_t count, Point* out_points)
{
	int i, j, k, tmp;
	gRect = rect;
	gCount = count;
	pool[0] = 0;
	p1[0].s = 0;
	squareBuffer[0] = 0;
	//v1 = v2 = v3 = 0;
	dfs(0);
	tmp = p1[0].s + 1;
	for (i = 1; i <= (p1[0].s >> 1); i++) {
		p1[tmp] = p1[i];
		p1[i] = p1[tmp - i];
		p1[tmp - i] = p1[tmp];
	}
	p1[0].v = INT32_MIN;
	for (i = 2; i < p1[0].s; i++) {
		j = i >> 1;
		k = i;
		p1[tmp] = p1[k];
		while (p1[tmp].v < p1[j].v) {
			p1[k] = p1[j];
			k = j;
			j = j >> 1;
		}
		p1[k] = p1[tmp];
	}
	for (i = 1; i <= p1[0].s; i++)
		p1[i].idx = square[p1[i].s].begin;
	p1[tmp].v = INT32_MAX;
	while ((pool[0] < gCount || p1[1].v < pool[1]) && p1[1].v < INT32_MAX) {
		updatePool(p1[1].v);
		p1[1].idx += 1;
		if (p1[1].idx >= square[p1[1].s].end) p1[1].v = INT32_MAX;
		else p1[1].v = rank[IDDEPTH(square[p1[1].s].id)][p1[1].idx];
		i = 1;
		p1[tmp] = p1[1];
		while ((j = (i << 1)) <= p1[0].s) {
			if (j < p1[0].s && p1[j].v > p1[j + 1].v) j += 1;
			if (p1[j].v >= p1[tmp].v) break;
			p1[i] = p1[j];
			i = j;
		}
		p1[i] = p1[tmp];
	}
	for (i = 1; i <= squareBuffer[0]; i++) {
		int k, depth;
		j = squareBuffer[i];
		depth = IDDEPTH(square[j].id);
		for (k = square[j].begin; k < square[j].end; k++) {
			int zz = rank[depth][k];
			if (pool[0] == gCount && zz >= pool[1]) break;
			if (node[zz].x < gRect.lx || node[zz].x > gRect.hx || node[zz].y < gRect.ly || node[zz].y > gRect.hy)
				continue;
			updatePool(zz);
		}
	}
	if (pool[0] > 1) sortCount(pool, 1, pool[0]);
	for (i = 0; i < pool[0]; i++) {
		j = pool[i + 1];
		out_points[i].id = node[j].id;
		out_points[i].rank = node[j].rank;
		out_points[i].x = node[j].x;
		out_points[i].y = node[j].y;
	}
	return pool[0];
}

__declspec(dllexport) SearchContext* destroy(SearchContext* sc)
{
	free(MemBegin);
	return NULL;
}
