#include "stdafx.h"
//Solution to Churchill Navigation point search challenge from by321@hotmail.com

/*Assumptions (in addition to what's in Churchill's readme.txt):
 *Point rank values are less than 2^24.
 *Point x,y values are less than FLT_MAX
 *search() is always called with count parameter set to 20.
*/
#include "point_search.h"

//#define PRINT_POINTS

const int GRIDS = 80; //32
const int CELLS = (GRIDS*GRIDS); //number of cells
const int SRCH_COUNT = 20; //search() is always called with count parameter set to 20

struct Point2 { unsigned int ri; float x, y; }; //pack rank and id into ri
struct IXY { int idx; float x, y; };

struct isrchresults { int x[SRCH_COUNT]; };
inline bool operator==(const isrchresults& a, const isrchresults& b) _NOEXCEPT{ return 0 == memcmp(a.x, b.x, sizeof(a.x)); }
struct hash_isrchresults { //not sure if this is a good hash
    unsigned int operator()(const isrchresults &x) const
    {   unsigned int h = 0;
        for (int i = 0; i < SRCH_COUNT; i++) {
            h = ( (h << 1) | (h >> 31) ) ^ x.x[i];
        }
        return h;
    }
};

static void merge2(int*__restrict p, const int*__restrict p1, const int*__restrict p2) {
    int n = 0, i1 = 0, i2 = 0;
    while (n != SRCH_COUNT && i1 != SRCH_COUNT && i2 != SRCH_COUNT) {
        if (p1[i1] < p2[i2]) {
            p[n] = p1[i1]; n++; i1++;
        } else if (p1[i1] == p2[i2]) {
            p[n] = p1[i1]; n++; i1++; i2++;
        } else {
            p[n] = p2[i2]; n++; i2++;
        }
    }
    while (n != SRCH_COUNT && i1 != SRCH_COUNT) {
        p[n] = p1[i1]; n++; i1++;
    }
    while (n != SRCH_COUNT && i2 != SRCH_COUNT) {
        p[n] = p2[i2]; n++; i2++;
    }
    while (n != SRCH_COUNT) {
        p[n] = INT_MAX; n++;
    }
}

struct CELL {
    int lowest[SRCH_COUNT+1]; //1 extra guard unit/scratch space
    int nLowest=0, pointIdx0 = 0, pointIdx1 = 0;
    float xmin = 0, xmax = 0, ymin = 0, ymax = 0;
    //precalc data
    int nx = 0, ny = 0,preCalcIndexCell=0;
};


struct SearchContext {
private:
    CELL cells[CELLS];
    int nPoints = 0;
    float rowYrange[GRIDS][2];
    float colXrange[GRIDS][2];
    std::vector<Point2> all_points;
	std::vector<IXY> ibuf;
    
    float xmin,xmax,ymin,ymax, xspacing,yspacing;
    //deal with floating point inaccuracies when converting float x/y positions to integer cell positions
    //should not be a problem with precise FP model, but just in case
    float gridMinX[GRIDS+1], gridMinY[GRIDS+1];
    int XYToCell(float x, float y, int*pcy) {
        int ix = (int)((x - xmin) / xspacing);
        if (ix < 0) ix = 0;
        if (ix >= GRIDS) ix = GRIDS - 1;
        if (ix > 0 && x < gridMinX[ix]) ix--;
        if (x >= gridMinX[ix + 1]) ix++;

        int iy = (int)((y - ymin) / yspacing);
        if (iy < 0) iy = 0;
        if (iy >= GRIDS) iy = GRIDS - 1;
        if (iy > 0 && y < gridMinY[iy]) iy--;
        if (y >= gridMinY[iy + 1]) iy++;

        *pcy = iy; return ix;
    }

    std::vector<int>preCalcIndex;
    std::vector<int>preCalc;

    void SawRawImage()
    {   if (!nPoints) return;
    
        const int idim=8192;//64M grayscale pixels
        unsigned char*p = (unsigned char*)malloc(idim * idim);
        memset(p, 0, idim*idim);
        float xs = (float)idim / (xmax - xmin);
        float ys = (float)idim / (ymax - ymin);
        float rs = 255.0f / float((all_points[nPoints - 1].ri >> 8) - (all_points[0].ri >> 8));
        for (int i = nPoints-1; i >=0; i--) { //from highest ranked to lowest ranked
            int x = min(idim - 1, int((all_points[i].x-xmin)*xs));
            int y = min(idim - 1, int((all_points[i].y-ymin)*ys));
            unsigned char g = 0xff - (unsigned char)(rs*((all_points[i].ri >> 8) - (all_points[0].ri >> 8)));
            p[y*idim + x] = g;
        }
        FILE*fp = fopen("image.raw", "wb"); fwrite(p, idim, idim, fp); fclose(fp);
        free(p);
    }
    void SetupCells();

    int PrecalcOneCell(int y0, int x0, vector<int>&wb) {
        int nx = GRIDS - x0, ny = GRIDS - y0, cidx = y0*GRIDS + x0;
        CELL*pcel = cells + cidx;
        cells[cidx].nx = nx; cells[cidx].ny = ny;
        //cells[cidx].cbufindex.resize(nx*ny);
        int writeLoc = 0; //offset into wb
        int oneLeft, oneAbove,tmp[SRCH_COUNT];
        //do first row differently
        //first cell, or upperleft corner cell
        memcpy(&wb[writeLoc], cells[cidx].lowest, SRCH_COUNT*sizeof(*cells[cidx].lowest));
        writeLoc += SRCH_COUNT;
        for (int x = 1; x < nx; x++) { //first row after first cell
            oneLeft = writeLoc-SRCH_COUNT;
            merge2(&wb[writeLoc], &wb[oneLeft], cells[cidx + x].lowest);
            writeLoc += SRCH_COUNT;
        }
        //now for the rest
        for (int y = 1; y < ny; y++) {
            cidx += GRIDS;
            //left most is special
            oneAbove = writeLoc - SRCH_COUNT*nx;
            merge2(&wb[writeLoc], &wb[oneAbove], cells[cidx].lowest);
            writeLoc += SRCH_COUNT;
            for (int x = 1; x < nx; x++) {
                oneAbove = writeLoc - SRCH_COUNT*nx;
                oneLeft = writeLoc-SRCH_COUNT;
                merge2(tmp, &wb[oneAbove], &wb[oneLeft]);
                merge2(&wb[writeLoc], tmp, cells[cidx + x].lowest);
                writeLoc += SRCH_COUNT;
            }
        }
        return writeLoc;
    }

    void PrecalcRegionMins() {

        unordered_map<isrchresults, int, hash_isrchresults>u;
        vector<int>workbuf(SRCH_COUNT*CELLS);
        for (int y = 0; y < GRIDS; y++) {
            for (int x = 0; x < GRIDS; x++) {
                int n=PrecalcOneCell(y, x, workbuf);
                //int offset0 = (int)preCalc.size();
                CELL*p = cells + y*GRIDS + x;
                for (int i = 0; i < p->nx*p->ny; i++) {
                    isrchresults*pi = (isrchresults*)(workbuf.data()+i*SRCH_COUNT);
                    int us = (int)u.size();
                    unordered_map<isrchresults, int, hash_isrchresults>::const_iterator uci = u.find(*pi);
                    if (uci == u.end()) {
                        u[*pi] = us;
                        preCalcIndex[i + p->preCalcIndexCell] = us*SRCH_COUNT;
                    } else {
                        preCalcIndex[i + p->preCalcIndexCell] = uci->second*SRCH_COUNT;
                    }
                }
            }
        }
        preCalc.resize(u.size()*SRCH_COUNT);
        unordered_map<isrchresults, int, hash_isrchresults>::const_iterator uci;
        for (uci = u.begin(); uci != u.end(); uci++) {
            memcpy(&preCalc[uci->second*SRCH_COUNT], uci->first.x, sizeof(uci->first.x));
        }
    }

    int inline MergePartialCell(int cy, int cx, int*__restrict p, int np, const Rect*__restrict r) {
        int i = cy*GRIDS + cx;
        if (r->lx>cells[i].xmax || r->hx<cells[i].xmin || r->ly>cells[i].ymax || r->hy < cells[i].ymin) return np;
        for (int j = cells[i].pointIdx0; j < cells[i].pointIdx1; j++) {
            int k = ibuf[j].idx;
            if (np == SRCH_COUNT && k>p[np - 1]) break;
            if (ibuf[j].x >= r->lx && ibuf[j].x <= r->hx && ibuf[j].y >= r->ly && ibuf[j].y <= r->hy) {
#if 0
                int i3;
                for (i3 = np - 1; i3 >= 0; i3--) {
                    if (p[i3] < k) break;
                    p[i3 + 1] = p[i3];
                }
                p[i3 + 1] = k;
#else
                int*ins = lower_bound(p, p + np, k); //20 items at most, is binary search really better ?
                for (int*p2 = p + np; p2 != ins; p2--) *p2 = p2[-1];
                *ins = k;
#endif
                if (np != SRCH_COUNT) np++;
            }
        }
        return np;
    }
    //int _nS = 0;
    int _fulls = 0, _areas = 0, _parts = 0;
    LONGLONG li0 = 0, li1 = 0, li2 = 0;

public:
    void reportStats() const {
        printf("\n*** regions: fulls %d parts %d total %d", _fulls, _parts, _areas);
        LARGE_INTEGER li; QueryPerformanceFrequency(&li); double d = (double)li.QuadPart;
        printf("\n*** timing: %f %f %f ms\n", (double)li0 / d * 1000, (double)li1 / d * 1000, (double)li2 / d * 1000);

        printf("*** pc values %d, %d, data %d %d MB, compression ratio %.2f\n", (int)preCalcIndex.size(), (int)preCalc.size(),
            (int)(preCalcIndex.size()*(sizeof(preCalcIndex[0])) >> 20), (int)(preCalc.size()*sizeof(preCalc[0]) >> 20),
            preCalc.size() ? double(preCalcIndex.size()) / (double)(preCalc.size() / SRCH_COUNT) : 0);
    }
    void init(const Point* points_begin, const Point* points_end);
    int32_t search(const Rect&rect, const int32_t count, Point* out_points);
};

void SearchContext::SetupCells()
{
    ibuf.resize(nPoints);

    vector<int>*vs = new vector<int>[CELLS]();
    for (int i = 0; i < CELLS; i++) vs[i].reserve(nPoints / CELLS * 120 / 100);

    for (int i = 0; i < nPoints; i++) {
        int xi, yi; xi = XYToCell(all_points[i].x, all_points[i].y, &yi);
        vs[yi*GRIDS + xi].push_back(i);
    }

    CELL*pc = cells;
    int j = 0;
    for (int i = 0; i < CELLS; i++) {
        pc[i].pointIdx0 = j;
        for (int k = 0; k < (int)vs[i].size(); k++, j++) {
            ibuf[j].idx = vs[i][k];
            ibuf[j].x = all_points[vs[i][k]].x;
            ibuf[j].y = all_points[vs[i][k]].y;
        }
        pc[i].pointIdx1 = j;
    }
    delete[]vs;
    assert(j == nPoints);

    for (int i = 0; i < CELLS; i++) { //get min x/y and smallest ranked points of cells
        float x0, x1, y0, y1;
        x0 = y0 = FLT_MAX; x1 = y1 = -FLT_MAX;
        for (int j = pc[i].pointIdx0; j < pc[i].pointIdx1; j++) {
            x0 = min(x0, ibuf[j].x); x1 = max(x1, ibuf[j].x);
            y0 = min(y0, ibuf[j].y); y1 = max(y1, ibuf[j].y);
        }
        pc[i].xmin = x0; pc[i].xmax = x1; pc[i].ymin = y0; pc[i].ymax = y1;

        pc[i].nLowest = min(SRCH_COUNT, pc[i].pointIdx1 - pc[i].pointIdx0);
        for (int j = 0; j < pc[i].nLowest; j++)
            pc[i].lowest[j] = ibuf[pc[i].pointIdx0 + j].idx;
        for (int j = pc[i].nLowest; j <= SRCH_COUNT; j++) 
            pc[i].lowest[j] = INT_MAX; //INT_MAX as terminator
    }

    for (int y = 0; y < GRIDS; y++) { //get y ranges of rows
        float y0 = cells[y*GRIDS].ymin, y1 = cells[y*GRIDS].ymax;
        for (int x = 1; x < GRIDS; x++) {
            y0 = min(y0, cells[y*GRIDS + x].ymin);
            y1 = max(y1, cells[y*GRIDS + x].ymax);
        }
        rowYrange[y][0] = y0; rowYrange[y][1] = y1;
    }
    for (int x = 0; x< GRIDS; x++) { //get x ranges of columns
        float x0 = cells[x].xmin, x1 = cells[x].xmax;
        for (int y = 1; y< GRIDS; y++) {
            x0 = min(x0, cells[y*GRIDS + x].xmin);
            x1 = max(x1, cells[y*GRIDS + x].xmax);
        }
        colXrange[x][0] = x0; colXrange[x][1] = x1;
    }
}

int32_t SearchContext::search(const Rect&r, const int32_t count, Point* pOut)
{   if (0 == nPoints) return 0;
    //_nS++;
    if (r.lx > xmax || r.ly > ymax || r.hx < xmin || r.hy < ymin) return 0;
    //if (_nS == 189)
    //    _nS = 189;
    int p[1+SRCH_COUNT],np=0;
    CELL*pc = cells;

    //these cell indices are inclusive on both sides
    int x0, y0; x0 = XYToCell(r.lx, r.ly, &y0);
    int x1, y1; x1 = XYToCell(r.hx, r.hy, &y1);
    //_areas += (x1 - x0 + 1)*(y1 - y0 + 1);

    //LARGE_INTEGER t0, t1; QueryPerformanceCounter(&t0);

    int fullx0 = x0; if (r.lx > colXrange[x0][0]) fullx0++;
    int fullx1 = x1; if (r.hx < colXrange[x1][1]) fullx1--;
    int fully0 = y0; if (r.ly > rowYrange[y0][0]) fully0++;
    int fully1 = y1; if (r.hy < rowYrange[y1][1]) fully1--;
    int ny = fully1 - fully0, nx = fullx1 - fullx0;
    //int tmp = max(0,(ny + 1)*(nx + 1)); _fulls += tmp; _parts += (x1 - x0 + 1)*(y1 - y0 + 1) - tmp;

    if (nx >= 0 && ny >= 0) {
        int idx0 = fully0*GRIDS + fullx0;
        int ys = cells[idx0].ny, xs = cells[idx0].nx;
        assert(ys > ny && xs > nx);
        int offset = preCalcIndex[cells[idx0].preCalcIndexCell+ny*xs + nx];
        memcpy(p, &preCalc[offset], sizeof(*p)*SRCH_COUNT);
        for (np = 0; np < SRCH_COUNT; np++) if (p[np] == INT_MAX) break;
    } else {
        fully0 = fullx0 = INT_MAX;
        fully1 = fullx1 = INT_MIN;
    }
    //QueryPerformanceCounter(&t1); li0 += t1.QuadPart - t0.QuadPart; t0 = t1;

    bool runT = y0 < fully0;
    bool runB = y1 > fully1 && y1 != y0;
    bool runL = x0 < fullx0;
    bool runR = x1 > fullx1 && x1 != x0;
    if (runT) {
        for (int x = x0; x <= x1; x++) np = MergePartialCell(y0, x, p, np, &r);
        y0++;
    }
    if (runB) {
        for (int x = x0; x <= x1; x++) np = MergePartialCell(y1, x, p, np, &r);
        y1--;
    }
    if (runL) {
        for (int y = y0; y <= y1; y++) np = MergePartialCell(y, x0, p, np, &r);
    }
    if (runR) {
        for (int y = y0; y <= y1; y++) np = MergePartialCell(y, x1, p, np, &r);
    }

    //QueryPerformanceCounter(&t1); li1 += t1.QuadPart - t0.QuadPart; t0 = t1;

    for (int i = 0; i < np; i++) {
        int idx = p[i];
        pOut[i].id = (char)(all_points[idx].ri&255);
        pOut[i].rank = (int)(all_points[idx].ri>>8);
        pOut[i].x = all_points[idx].x; pOut[i].y = all_points[idx].y;
    }
    //QueryPerformanceCounter(&t1); li2 += t1.QuadPart - t0.QuadPart;
    return np;
}

void SearchContext::init(const Point* p0, const Point* p1)
{
    if (0 == p0 || p0 == p1) return;

    int s = 0;
    for (int y = 0; y < GRIDS; y++) {
        for (int x = 0; x < GRIDS; x++) {
            cells[y*GRIDS + x].preCalcIndexCell = s;
            s += (GRIDS - y)*(GRIDS - x);
        }
    }
    preCalcIndex.resize(s); //printf("precalc index size %d\n",s);

    nPoints = int(p1 - p0);
    all_points.resize(nPoints);
    for (int i = 0; i < nPoints; i++) {
        all_points[i].ri = (((unsigned int)p0[i].rank) << 8) | (unsigned char)(p0[i].id);
        all_points[i].x = p0[i].x; all_points[i].y = p0[i].y;
    }

    sort(all_points.begin(), all_points.end(), [](Point2 x, Point2 y) {return x.ri < y.ri; });

    xmin = xmax = p0->x; ymin = ymax = p0->y;
    while (p0 != p1) {
        xmin = min(xmin, p0->x); xmax = max(xmax, p0->x);
        ymin = min(ymin, p0->y); ymax = max(ymax, p0->y);
        p0++;
    }
    xspacing = (xmax - xmin) / GRIDS; yspacing = (ymax - ymin) / GRIDS;
    gridMinX[0] = xmin; gridMinX[GRIDS] = FLT_MAX;
    gridMinY[0] = ymin; gridMinY[GRIDS] = FLT_MAX;
    for (int i = 1; i < GRIDS; i++) {
        gridMinX[i] = xmin + (xmax - xmin) / GRIDS*i;
        gridMinY[i] = ymin + (ymax - ymin) / GRIDS*i;
    }
    
    SetupCells();
    PrecalcRegionMins();
}

extern "C" __declspec(dllexport)
SearchContext* __stdcall create(const Point* points_begin, const Point* points_end)
{
#ifdef PRINT_POINTS
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

    //printf("srch rect.lx=%f; rect.ly=%f; rect.hx=%f; rect.hy=%f;\n", rect.lx, rect.ly, rect.hx, rect.hy);

    int n= sc->search(rect, count, out_points);
    //puts("srch results");
    //for (int i = 0; i < n; i++) printf("    %d %f %f\n", out_points[i].rank, out_points[i].x, out_points[i].y);
    return n;
}


extern "C" __declspec(dllexport)
SearchContext* __stdcall destroy(SearchContext* sc) {
    sc->reportStats();
    delete sc;
    return nullptr;
}
