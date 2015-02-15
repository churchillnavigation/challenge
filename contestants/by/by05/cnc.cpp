#include "stdafx.h"
//07 latest QuadTree breadth search
//use INT_MAX terminator for lowest[]
//use IXY to reduce cache miss
#include "point_search.h"

const int LEVELS =5; //levels range: [0,LEVELS]
const int GRIDS = (1<<LEVELS);
const int CELLS = (GRIDS*GRIDS); //number of cells in the lowest layer
const int SRCH_COUNT = 20; //search() is always called with count parameter set to 20

struct XY { float x, y;  };
struct IXY { int idx; float x, y; };

static int CalcQuadTreeCells() { //constexpr not supported in VC++ 2013
    int s = 0;
    for (int i = 0; i <= LEVELS; i++)
        s += (1 << i)*(1 << i);
    return s; 
}

static int merge_lowest(int*__restrict pDst, int nDst, const int*__restrict pSrc)
{ //both arrays are already sorted, merge by bubble sort with early exit
    
    if (nDst < SRCH_COUNT) {
        while (*pSrc!=INT_MAX) {
            int j = nDst - 1;
            for (; j >= 0; j--) {
                if (*pSrc > pDst[j]) break;
                pDst[j + 1] = pDst[j];
            }
            pDst[j + 1] = *pSrc; pSrc++;
            nDst++; if (nDst == SRCH_COUNT) break;
        }
    }
    assert(*pSrc==INT_MAX || nDst == SRCH_COUNT);
    while (*pSrc<pDst[SRCH_COUNT - 1]) { //if we get into this loop, then *pnDst==SRCH_COUNT
        int j = SRCH_COUNT-2;
        for (; j >= 0; j--) {
            if (*pSrc > pDst[j]) break;
            pDst[j + 1] = pDst[j];
        }
        pDst[j + 1] = *pSrc; pSrc++;
    }
    return nDst;
}


struct CELL {
	float xmin=0, xmax=0, ymin=0, ymax=0; //inclusive values
    int nLowest=0,lowest[SRCH_COUNT+1]; //1 extra guard unit/scratch space
    int pointIdx0 = 0, pointIdx1 = 0;
    int next4[4];// = { 0, 0, 0, 0 }; //error C2536: cannot specify explicit initializer for arrays
    CELL() { next4[0] = next4[1] = next4[2] = next4[3] = 0; }
};

static int trimquadtree(int idx,CELL*const p0) {
    int s = 0;

    for (int i = 0; i < 4; i++) {
        int i2 = p0[idx].next4[i];
        if (0 == i2) continue;
        if (p0[i2].nLowest == 0) {
            p0[idx].next4[i] = 0; s++;
            continue;
        }
        s += trimquadtree(i2, p0);
    }
    return s;
}

static int count_nonempty_nodes(int idx, const CELL*const p0)
{   int s = 0;
    for (int i = 0; i < 4; i++) {
        if (p0[idx].next4[i]) s = s + 1 + count_nonempty_nodes(p0[idx].next4[i], p0);
    }
    return s;
}

struct SearchContext
{
    void setup_quadtree();
    void AssignPointsToCells();
    void init(const Point* points_begin, const Point* points_end);
    int32_t search(const Rect&rect, const int32_t count, Point* out_points);
    int32_t QuadTreeSearchB(const Rect&r, const int32_t count, Point* out_points); //breadth first

    int nPoints=0;
    std::vector<Point> all_points;
	std::vector<IXY> ibuf;
	//int cellIndexOffsets[CELLS + 1]; //offsets into ibuf
    CELL*cells = 0; int nCells=0;

    float xmin,xmax,ymin,ymax, xspacing,yspacing;

	int _fulls = 0, _ones=0, _areas=0, _parts=0;

    //float _Slx, _Shx, _Sly, _Shy;
    int _SpartialCells[GRIDS * 4];// , _SpartialCellsN = 0;
    int*_cellPtrBuf = nullptr;
    LONGLONG li0 = 0, li1 = 0, li2 = 0;
    ~SearchContext() {
        if (cells) delete[]cells;
        if (_cellPtrBuf) free(_cellPtrBuf);
    }
};

void SearchContext::AssignPointsToCells() {
    ibuf.resize(nPoints);

    xspacing = (xmax - xmin) / GRIDS;
    yspacing = (ymax - ymin) / GRIDS;

	vector<int>*vs = new vector<int>[CELLS]();
	for (int i = 0; i < CELLS; i++) vs[i].reserve(nPoints / CELLS * 120 / 100);

	for (int i = 0; i < nPoints; i++) {
		int xi = int((all_points[i].x - xmin) / xspacing); if (xi == GRIDS) xi--;
        int yi = int((all_points[i].y - ymin) / yspacing); if (yi == GRIDS) yi--;
		vs[yi*GRIDS+xi].push_back(i);
	}

    CELL*pc = cells + nCells - CELLS;
    int j = 0;
	for (int i = 0; i < CELLS; i++) {
        pc[i].pointIdx0 = j;
        for (int k = 0; k< (int)vs[i].size(); k++,j++) {
            ibuf[j].idx = vs[i][k];
            ibuf[j].x = all_points[vs[i][k]].x;
            ibuf[j].y = all_points[vs[i][k]].y;
        }
        pc[i].pointIdx1 = j;
    }
	delete[]vs;
	assert(j == nPoints);

	for (int i = 0; i < CELLS; i++) { //get min x/y of cells
		float x0, x1, y0, y1;
		x0 = y0 = FLT_MAX; x1 = y1 = -FLT_MAX;
		for (int j = pc[i].pointIdx0; j < pc[i].pointIdx1; j++) {
            x0 = min(x0, ibuf[j].x); x1 = max(x1, ibuf[j].x);
            y0 = min(y0, ibuf[j].y); y1 = max(y1, ibuf[j].y);
		}
        pc[i].xmin = x0; pc[i].xmax = x1; pc[i].ymin = y0; pc[i].ymax = y1;
        pc[i].nLowest= min(SRCH_COUNT, pc[i].pointIdx1 - pc[i].pointIdx0);
        for (int j = 0; j < pc[i].nLowest; j++)
            pc[i].lowest[j] = ibuf[pc[i].pointIdx0 + j].idx;
        pc[i].lowest[pc[i].nLowest] = INT_MAX;
    }
}

void SearchContext::setup_quadtree() {
    int curgrid = GRIDS;  CELL*pcur = cells + nCells - CELLS;
    for (int i = 0; i < LEVELS; i++) {
        int nxtgrid = curgrid >> 1; CELL*pnxt = pcur - nxtgrid*nxtgrid;
        for (int y = 0; y < nxtgrid; y++) {
            for (int x = 0; x < nxtgrid; x++) {
                CELL*p = pnxt + y*nxtgrid + x;
                int nxtoffset = (y + y)*curgrid + x + x;
                CELL*p0 = pcur + nxtoffset;
                CELL*p1 = pcur + nxtoffset + 1;
                CELL*p2 = pcur + nxtoffset + curgrid;
                CELL*p3 = pcur + nxtoffset + curgrid + 1;
                p->xmin = min(min(p0->xmin, p1->xmin), min(p2->xmin, p3->xmin));
                p->ymin = min(min(p0->ymin, p1->ymin), min(p2->ymin, p3->ymin));
                p->xmax = max(max(p0->xmax, p1->xmax), max(p2->xmax, p3->xmax));
                p->ymax = max(max(p0->ymax, p1->ymax), max(p2->ymax, p3->ymax));
                p->next4[0] = (int)(p0 - cells);
                p->next4[1] = (int)(p1 - cells); 
                p->next4[2] = (int)(p2 - cells);
                p->next4[3] = (int)(p3 - cells);

                for (int i = 0; i < 4; i++) {
                    p->nLowest=merge_lowest(p->lowest, p->nLowest, cells[p->next4[i]].lowest);
                    p->lowest[p->nLowest] = INT_MAX;
                }
            }
        }
        pcur = pnxt; curgrid = nxtgrid;
    }
#if 0
    for (int i = 0; i < nCells; i++) {
        CELL*p = cells + i;
        printf("cel %4d, %4d %4d %4d %4d, %d, %d %d\n", i,
            p->next4[0],p->next4[1],p->next4[2],p->next4[3],
            p->nLowest,p->pointIdx0,p->pointIdx1);
        for (int j = 0; j < p->nLowest; j++) {
            int k = p->lowest[j];
            printf("    %d %f %f, %f %f %f %f\n", all_points[k].rank, all_points[k].x, all_points[k].y, p->xmin, p->ymin, p->xmax, p->ymax);
        }
    }
#endif
    assert(xmin == cells->xmin);
    assert(xmax == cells->xmax);
    assert(ymin == cells->ymin);
    assert(ymax == cells->ymax);
    
#if 0
    printf("\n## %f %f %f %f\n", xmin, xmax, ymin, ymax);
    printf("## %f %f %f %f\n", cells->xmin, cells->xmax, cells->ymin, cells->ymax);

    float x0, x1, y0, y1;
    x0 = y0 = FLT_MAX; x1 = y1 = -FLT_MAX;
    for (int i = 0; i < CELLS; i++) {
        CELL*p = cells + nCells - CELLS + i;
        x0 = min(x0, p->xmin);
        y0 = min(y0, p->ymin);
        x1 = max(x1, p->xmax);
        y1 = max(y1, p->ymax);
    }
    printf("## %f %f %f %f\n", x0,x1,y0,y1);
#endif
}

void SearchContext::init(const Point* p0, const Point* p1) {
    if (0 == p0 || p0 == p1) {
        cells = new CELL(); nCells = 1;
        return;
    }

    nCells = CalcQuadTreeCells();
    cells = new CELL[nCells];
    _cellPtrBuf = (int*)malloc(sizeof(*_cellPtrBuf)*nCells);

    nPoints = int(p1 - p0);
    std::copy(p0, p1, back_inserter(all_points));
    sort(all_points.begin(), all_points.end(), [](Point x, Point y) {return x.rank < y.rank; });

    xmin = xmax = p0->x;
    ymin = ymax = p0->y;
    while (p0 != p1) {
        xmin = min(xmin, p0->x); xmax = max(xmax, p0->x);
        ymin = min(ymin, p0->y); ymax = max(ymax, p0->y);
        p0++;
    }

    
    AssignPointsToCells();
    setup_quadtree();
    printf("ncells %d, kills %d, ", nCells, trimquadtree(0,cells) );
    printf("remaining %d\n", 1 + count_nonempty_nodes(0,cells));
}

int32_t SearchContext::QuadTreeSearchB(const Rect&r,const int32_t count, Point* out_points)
{ //breadth-first search
    int _foundIdx[SRCH_COUNT + 1];
    LARGE_INTEGER qpc0,qpc1;
    QueryPerformanceCounter(&qpc0);
    int foundIdxN =0, _SpartialCellsN = 0;
    
    int*ph = _cellPtrBuf,*pt=_cellPtrBuf; //ph:head/write, pt:tail/read
    const CELL*__restrict p = cells;
    while (true) { //breadth-first search using _cellPtrBuf
        if (r.lx > p->xmax || r.ly > p->ymax || r.hx < p->xmin || r.hy < p->ymin)  {
            //no overlap
        } else {
            if (p->xmin >= r.lx && p->xmax <= r.hx && p->ymin >= r.ly && p->ymax <= r.hy) { //fully contained
                _fulls++;
                foundIdxN = merge_lowest(_foundIdx, foundIdxN, p->lowest);
            } else { //overlapping
                if (p->pointIdx0 != p->pointIdx1) { //at lowest level
                    _SpartialCells[_SpartialCellsN] = (int)(p - cells);
                    _SpartialCellsN++;
                } else {
                    if (p->next4[0]) *ph++ = p->next4[0];
                    if (p->next4[1]) *ph++ = p->next4[1];
                    if (p->next4[2]) *ph++ = p->next4[2];
                    if (p->next4[3]) *ph++ = p->next4[3];
                }
            } //if (p->xmin >= _Slx && p->xmax <= _Shx && p->ymin >= _Sly && p->ymax <= _Shy) { //fully contained
        }

        if (ph == pt) break;
        p = *pt + cells; pt++;
    } //while (true) { //breadth-first search using _cellPtrBuf

    QueryPerformanceCounter(&qpc1); li0 += qpc1.QuadPart - qpc0.QuadPart; qpc0 = qpc1;
    //CheckCell(cells);
    _parts += _SpartialCellsN;
    for (int i = 0; i< _SpartialCellsN; i++) {
        int idx0 = cells[_SpartialCells[i]].pointIdx0;
        int idx1 = cells[_SpartialCells[i]].pointIdx1;
        for (int j = idx0; j<idx1; j++) {
            int k = ibuf[j].idx, i3;
            if (foundIdxN == SRCH_COUNT && k>_foundIdx[foundIdxN - 1]) break;
            if (ibuf[j].x >= r.lx && ibuf[j].x <= r.hx && ibuf[j].y >= r.ly && ibuf[j].y <= r.hy) {
                for (i3 = foundIdxN - 1; i3 >= 0; i3--) {
                    if (_foundIdx[i3] < k) break;
                    _foundIdx[i3 + 1] = _foundIdx[i3];
                }
                _foundIdx[i3 + 1] = k;
                if (foundIdxN != SRCH_COUNT) foundIdxN++;
            }
        }
    }
    QueryPerformanceCounter(&qpc1); li1 += qpc1.QuadPart - qpc0.QuadPart; qpc0 = qpc1;
    for (int i = 0; i < foundIdxN; i++) {
        int idx = _foundIdx[i];
        out_points[i]= all_points[idx];
    }
    QueryPerformanceCounter(&qpc1); li2 += qpc1.QuadPart - qpc0.QuadPart; qpc0 = qpc1;
    return foundIdxN;
}

int32_t SearchContext::search(const Rect&r, const int32_t count, Point* out_points)
{   if (0 == nPoints) return 0;
    
    int*p = (int*)alloca(sizeof(int)*(1+count)),np=0;
    CELL*pc = cells + nCells - CELLS;

	float lx = r.lx; if (lx > xmax) return 0;
	float ly = r.ly; if (ly > ymax) return 0;
	float hx = r.hx; if (hx < xmin) return 0;
	float hy = r.hy; if (hy < ymin) return 0;

	if (lx < xmin) lx = xmin;
	if (ly < ymin) ly = ymin;

	int x0 = int((lx - xmin) / xspacing); if (x0 == GRIDS) x0--;
	int y0 = int((ly - ymin) / yspacing); if (y0 == GRIDS) y0--;
	int x1 = int((hx - xmin) / xspacing); if (x1 >= GRIDS) x1 = GRIDS - 1;
	int y1 = int((hy - ymin) / yspacing); if (y1 >= GRIDS) y1 = GRIDS - 1;
	if (x0 == x1 && y0 == y1) _ones++;
	_areas += (x1 - x0 + 1)*(y1 - y0 + 1);
	for (int y = y0; y<= y1; y++) {
		for (int x = x0; x <= x1; x++) {
			int i = y*GRIDS + x;
			if (pc[i].pointIdx1 == pc[i].pointIdx0) continue;
            if (pc[i].xmin >= lx && pc[i].xmax <= hx && pc[i].ymin >= ly && pc[i].ymax <= hy) {
				_fulls++;
                //printf("_fulls %d\n",pc+i-cells);
                np=merge_lowest(p, np, pc[i].lowest);
			} else {
                //printf("_parts %d\n", pc + i - cells);
                _parts++;
                for (int j = pc[i].pointIdx0; j < pc[i].pointIdx1; j++) {
					int k = ibuf[j].idx,i3;
					if (np == count && k>p[np - 1]) break;
                    if (all_points[k].x >= lx && all_points[k].x <= hx && all_points[k].y >= ly && all_points[k].y <= hy) {
						for (i3 = np - 1; i3 >= 0; i3--) {
							if (p[i3] < k) break;
							p[i3 + 1] = p[i3];
						}
						p[i3 + 1] = k;
						if (np != count) np++;
					}
				}
			}
		}//for (int x = x0; x <= x1; x++) {
	}//for (int y = y0; y<= y1; y++) {

    for (int i = 0; i < np; i++) {
        int idx = p[i];
        out_points[i] = all_points[idx];
    }
    return np;
}

extern "C" __declspec(dllexport)
SearchContext* __stdcall create(const Point* points_begin, const Point* points_end)
{
#if 0
    if (points_begin) {
        puts("points:");
        for (const Point*p = points_begin; p != points_end; p++)
            printf("    %d %f %f\n", p->rank, p->x, p->y);
    }
#endif
    SearchContext*sc = new SearchContext();
    //TODO: special handling for last 4 ?
    sc->init(points_begin, points_begin ? points_end - 4 : points_end);
    //printf("create %p\n", sc);

    return sc;
}

extern "C" __declspec(dllexport)
int32_t __stdcall search(SearchContext* sc, const Rect rect, const int32_t count, Point* out_points)
{
	if (count != 20) {
		MessageBox(0,"search() called with count!=20","unexpected parameter values",MB_OK);
		exit(-1);
	}
    //printf("srch %f %f %f %f\n", rect.lx, rect.ly, rect.hx, rect.hy);
    //int n= sc->search(rect, count, out_points);
    //int n = sc->QuadTreeSearch(rect, count, out_points);
    int n = sc->QuadTreeSearchB(rect, count, out_points);
    //puts("srch results");
    //for (int i = 0; i < n; i++) printf("    %d %f %f\n", out_points[i].rank, out_points[i].x, out_points[i].y);
    return n;
}

extern "C" __declspec(dllexport)
SearchContext* __stdcall destroy(SearchContext* sc) {
    printf("\ndestroy fulls %d parts %d ones %d areas %d", sc->_fulls, sc->_parts, sc->_ones,sc->_areas);
    LARGE_INTEGER li; QueryPerformanceFrequency(&li); double d = (double)li.QuadPart ;
    printf("\ndestroy %f %f %f\n", (double)(sc->li0) / d*1000, (double)(sc->li1) / d*1000, (double)(sc->li2) / d*1000);

    delete sc;
    return nullptr;
}
