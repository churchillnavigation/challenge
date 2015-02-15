// ZeroEpsilon.cpp : Defines the exported functions for the DLL application.
//

#include "ZeroEpsilon.h"
#include "point_search.h"

#include <stdio.h>
#include <algorithm>
#include <vector>
#include <mmintrin.h>


extern "C" {
	ZEROEPSILON_API SearchContext* __stdcall create(const Point* points_begin, const Point* points_end);
	ZEROEPSILON_API int32_t __stdcall search(SearchContext* sc, const Rect rect, const int32_t count, Point* out_points);
	ZEROEPSILON_API SearchContext* __stdcall destroy(SearchContext* sc);
}


#define NUM_GRID_LEVELS 3
#define MAX_SEARCH_CELLS 5000

#define SUB_THRESHOLD 8
#define COMMON_LEVEL_MULT 11
#define LEVEL_MULT_TO(i) COMMON_LEVEL_MULT
#define LEVEL_MULT_FROM(i) COMMON_LEVEL_MULT
#define LEVEL_MULT_0 COMMON_LEVEL_MULT
#define LEVEL_MULT_1 COMMON_LEVEL_MULT
#define LEVEL_MULT_2 COMMON_LEVEL_MULT

#define finestNumCells (LEVEL_MULT_0*LEVEL_MULT_1*LEVEL_MULT_2)


#define MIN_HEAP_SIZE 1000
#define MIN_BUBBLE_SIZE 1000
#define MIN_PARTIAL_SORT_SIZE 25
#define PARTIAL_SORT_FRAC 1.0f


#define ADD_POINT								\
{												\
	np++;										\
	op->id = p.id;								\
	op->x = p.x;								\
	op->y = p.y;								\
	op->rank = bestElement.rank;				\
	op++;										\
	if (np == count)							\
		done = true;							\
}


#define CHECK_AND_ADD_POINT_ALL \
	if (p.x >= rect.lx &&		\
		p.x <= rect.hx &&		\
		p.y >= rect.ly &&		\
		p.y <= rect.hy) 		\
		ADD_POINT


#define CHECK_AND_ADD_POINT CHECK_AND_ADD_POINT_ALL

#define ADD_HEADER							\
	int hi = pg->axisCells[0] * yc + xc;	\
	int *hp = pg->headerInfo[hi].pointer;	\
	if (hp) {								\
		int dhp = pg->headerInfo[hi].value;	\
		searchPointList->next = hp;			\
		searchPointList->rank = dhp;		\
		searchPointList++;					\
	}


#pragma pack(push, 1)
struct MyPoint
{
	int8_t id;
	float x;
	float y;
};

struct HeaderInfo
{
	int *pointer;
	int value;
};
#pragma pack(pop)


struct PointGrid {
	int axisCells[2];

	std::vector<int> pointLists;
	std::vector<HeaderInfo> headerInfo;
};


struct SearchListInfo {
	SearchListInfo() { } // noop constructor makes vector resize fast

	int rank; // rank == index!
	int *next;
};

#define BOUND_CHECK_XMIN 1
#define BOUND_CHECK_XMAX 2
#define BOUND_CHECK_YMIN 4
#define BOUND_CHECK_YMAX 8

bool operator<(const SearchListInfo &lhs, const SearchListInfo &rhs) {
	return (lhs.rank < rhs.rank);
}

bool heapLessThan(const SearchListInfo &lhs, const SearchListInfo &rhs) {
	return (lhs.rank > rhs.rank); // sorts backwards because the heap puts the largest value at the front
}


bool operator<(const Point &lhs, const Point &rhs)
{
	return (lhs.rank < rhs.rank);
}


struct SearchContext {
	std::vector<MyPoint> points;
	int nIgnoredPoints;
	Rect bounds;
	PointGrid pointGrids[NUM_GRID_LEVELS][NUM_GRID_LEVELS];
	std::vector<SearchListInfo> searchPointList;
};



void buildPointGrid(int axis, int npoints, int levelMult,
	const PointGrid *from, PointGrid *to,
	const std::vector<int> finestCells[2])
{

	to->axisCells[axis] = from->axisCells[axis] * levelMult;
	to->axisCells[1 - axis] = from->axisCells[1 - axis];

	int plsize = 0;
	to->pointLists.resize(npoints + to->axisCells[0]*to->axisCells[1]);
	to->headerInfo.resize(to->axisCells[0]*to->axisCells[1]);

	std::vector< std::vector<int> > tmplists(levelMult);
	for (int yc = 0; yc < from->axisCells[1]; yc++) {
		for (int xc = 0; xc < from->axisCells[0]; xc++) {
			for (int i = 0; i < levelMult; i++)
				tmplists[i].clear();

			if (from->headerInfo[yc*from->axisCells[0] + xc].pointer) {
				for (int *pl = from->headerInfo[yc*from->axisCells[0] + xc].pointer;
					*pl >= 0;
					++pl) {
					int s = ((finestCells[axis][*pl] / (finestNumCells / to->axisCells[axis])) %
						(to->axisCells[axis] / from->axisCells[axis]));
					tmplists[s].push_back(*pl);
				}
			}

			for (int j = 0; j < levelMult; j++) {
				if (axis == 0)
					to->headerInfo[yc*to->axisCells[0] + xc * levelMult + j].pointer = &to->pointLists[plsize];
				else
					to->headerInfo[(yc * levelMult + j)*to->axisCells[0] + xc].pointer = &to->pointLists[plsize];

				for (int i = 0; i < tmplists[j].size(); i++)
					to->pointLists[plsize++] = tmplists[j][i];
				to->pointLists[plsize++] = -1;
			}
		}
	}

	//replace empty lists with null pointers - saves a dereference later
	for (int i = 0; i<to->headerInfo.size(); i++) {
		if (*to->headerInfo[i].pointer < 0) {
			to->headerInfo[i].pointer = NULL;
			to->headerInfo[i].value = -1;
		}
		else {
			to->headerInfo[i].value = *to->headerInfo[i].pointer;
		}
	}

	if (plsize > to->pointLists.size())
		printf("over ran point list size!\n");
}


void validatePointGrid(const SearchContext *sc,
	const PointGrid *pg)
{
	printf("validating point grid %d %d\n", pg->axisCells[0], pg->axisCells[1]);
	int npoints = 0;
	for (int yc = 0; yc < pg->axisCells[1]; yc++) {
		for (int xc = 0; xc < pg->axisCells[0]; xc++) {
			if (!pg->headerInfo[yc*pg->axisCells[0] + xc].pointer)
				continue;

			bool first = true;
			int32_t lastRank = 0;

			for (int *pi = pg->headerInfo[yc*pg->axisCells[0] + xc].pointer;
				*pi >= 0;
				++pi) {
				npoints++;

				int rank = *pi;
				const MyPoint *p = &sc->points[rank];
				float nx = (p->x - sc->bounds.lx) / (sc->bounds.hx - sc->bounds.lx);
				float ny = (p->y - sc->bounds.ly) / (sc->bounds.hy - sc->bounds.ly);

				int fxcell = (int)(nx * finestNumCells);
				int fycell = (int)(ny * finestNumCells);
				
				int xcell = fxcell / (finestNumCells / pg->axisCells[0]);
				int ycell = fycell / (finestNumCells / pg->axisCells[1]);


				if (xcell != xc)
					printf("point in wrong x cell!\n");

				if (ycell != yc)
					printf("point in wrong y cell!\n");

				if (!first) {
					if (lastRank > rank)
						printf("incorrect rank order!\n");
				}
				lastRank = rank;
				first = false;
			}
		}
	}

	if (npoints != sc->points.size()-sc->nIgnoredPoints)
		printf("incorrect total number of points!n");
}


ZEROEPSILON_API SearchContext* __stdcall create(const Point* points_begin, const Point* points_end)
{
	if (points_begin == NULL ||
		points_end == NULL)
		return NULL;

	int npoints = (int)(points_end - points_begin);

	SearchContext *sc = new SearchContext;
	sc->searchPointList.reserve(MAX_SEARCH_CELLS);

#if 0
	// copy points into our own array
	sc->points.reserve(npoints);
	for (const Point *i = points_begin; i != points_end; ++i) {
		sc->points.push_back(*i);
	}

	// sort our array of points in place
	std::sort(sc->points.begin(), sc->points.end());
#else

	std::pair<int32_t, int> *tmpSort = new std::pair<int32_t, int>[npoints];
	for (int i = 0; i < npoints; i++) {
		tmpSort[i].first = points_begin[i].rank;
		tmpSort[i].second = i;
	}
	std::sort(tmpSort, tmpSort + npoints);
	sc->points.resize(npoints);
	for (int i = 0; i < npoints; i++) {
		sc->points[i].id = points_begin[tmpSort[i].second].id;
		sc->points[i].x = points_begin[tmpSort[i].second].x;
		sc->points[i].y = points_begin[tmpSort[i].second].y;
	}
	delete[] tmpSort;
#endif

	// compute bounding box
	sc->bounds.lx = sc->points[0].x;
	sc->bounds.ly = sc->points[0].y;
	sc->bounds.hx = sc->points[0].x;
	sc->bounds.hy = sc->points[0].y;
	sc->nIgnoredPoints = 0;
	for (int i = 1; i < npoints; i++) {
		// ignore these points that are waaay out there
		if (std::abs(sc->points[i].x) == 1e10 ||
			std::abs(sc->points[i].y) == 1e10) {
			sc->nIgnoredPoints++;
			continue;
		}

		sc->bounds.lx = std::min(sc->bounds.lx, sc->points[i].x);
		sc->bounds.ly = std::min(sc->bounds.ly, sc->points[i].y);
		sc->bounds.hx = std::max(sc->bounds.hx, sc->points[i].x);
		sc->bounds.hy = std::max(sc->bounds.hy, sc->points[i].y);
	}

	// slightly expand the bound so that all points map less than the max bound
	float xfactor = 1e-5f;
	while (1) {
		float newb = (1 + xfactor) * (sc->bounds.hx - sc->bounds.lx) + sc->bounds.lx;
		float norm = (sc->bounds.hx - sc->bounds.lx) / (newb - sc->bounds.lx);
		if (norm < 1.0f) {
			sc->bounds.hx = newb;
			break;
		}
		xfactor *= 2;
	}

	float yfactor = 1e-5f;
	while (1) {
		float newb = (1 + yfactor) * (sc->bounds.hy - sc->bounds.ly) + sc->bounds.ly;
		float norm = (sc->bounds.hy - sc->bounds.ly) / (newb - sc->bounds.ly);
		if (norm < 1.0f) {
			sc->bounds.hy = newb;
			break;
		}
		yfactor *= 2;
	}
	//printf("bound scales: %g %g\n", xfactor, yfactor);


	sc->bounds.hx = (1 + 1e-2f) * (sc->bounds.hx - sc->bounds.lx) + sc->bounds.lx;
	sc->bounds.hy = (1 + 1e-2f) * (sc->bounds.hy - sc->bounds.ly) + sc->bounds.ly;

	// map each point into the finist grid
	std::vector<int> finestCell[2];
	finestCell[0].resize(npoints);
	finestCell[1].resize(npoints);

	for (int i = 0; i < npoints; i++) {
		// ignore these points that are waaay out there
		if (std::abs(sc->points[i].x) == 1e10 ||
			std::abs(sc->points[i].y) == 1e10) {
			continue;
		}


		float xn = (sc->points[i].x - sc->bounds.lx) / (sc->bounds.hx - sc->bounds.lx);
		float yn = (sc->points[i].y - sc->bounds.ly) / (sc->bounds.hy - sc->bounds.ly);

		int xcell = (int)(xn * finestNumCells);
		int ycell = (int)(yn * finestNumCells);

		if (xcell < 0 || xcell >= finestNumCells ||
			ycell < 0 || ycell >= finestNumCells) {
			printf("strange grid cell mapping!\n");
			printf("cell: %d %d %d\n", xcell, ycell, finestNumCells);
			printf("bounding box: %g %g %g %g\n", sc->bounds.lx, sc->bounds.ly, sc->bounds.hx, sc->bounds.hy);
			printf("point: %g %g\n", sc->points[i].x, sc->points[i].y);
			return NULL;
		}

		finestCell[0][i] = xcell;
		finestCell[1][i] = ycell;
	}


	// setup grid level 0
	sc->pointGrids[0][0].axisCells[0] = 1;
	sc->pointGrids[0][0].axisCells[1] = 1;

	sc->pointGrids[0][0].pointLists.reserve(npoints + 1);
	for (int i = 0; i < npoints; i++) {
		// ignore these points that are waaay out there
		if (std::abs(sc->points[i].x) == 1e10 ||
			std::abs(sc->points[i].y) == 1e10) {
			continue;
		}

		sc->pointGrids[0][0].pointLists.push_back(i); // points are already sorted globally
	}
	sc->pointGrids[0][0].pointLists.push_back(-1); // end of list

	sc->pointGrids[0][0].headerInfo.resize(1);
	sc->pointGrids[0][0].headerInfo[0].pointer = &sc->pointGrids[0][0].pointLists[0];

	// setup the initial level
	buildPointGrid(0, npoints, LEVEL_MULT_TO(0),
		&sc->pointGrids[0][0],
		&sc->pointGrids[0][1],
		finestCell);
	buildPointGrid(1, npoints, LEVEL_MULT_TO(0),
		&sc->pointGrids[0][1],
		&sc->pointGrids[0][0],
		finestCell);


	// build the rest of the grids from the one next to it
	for (int ly = 0; ly < NUM_GRID_LEVELS; ly++) {
		for (int lx = 0; lx < NUM_GRID_LEVELS; lx++) {
			if (lx > 0)
				buildPointGrid(0, npoints, LEVEL_MULT_TO(lx),
								&sc->pointGrids[lx - 1][ly],
								&sc->pointGrids[lx][ly],
								finestCell);
			else if (ly > 0)
				buildPointGrid(1, npoints, LEVEL_MULT_TO(ly),
								&sc->pointGrids[lx][ly-1],
								&sc->pointGrids[lx][ly],
								finestCell);
		}
	}


#if 0
	for (int ly = 0; ly < NUM_GRID_LEVELS; ly++) {
		for (int lx = 0; lx < NUM_GRID_LEVELS; lx++) {
			validatePointGrid(sc, &sc->pointGrids[lx][ly]);
		}
	}
#endif

	return sc;
}


__declspec(noinline)
void searchLinear(SearchListInfo *searchPointList, int splSize, const MyPoint *my_points,
const Rect rect, const int32_t count, Point* out_points, int &np)
{
	if (splSize == 0)
		return;
	/*
	for (int i = 0; i < splSize; i++) {
	_mm_prefetch((char*)&my_points[searchPointList[i].rank], _MM_HINT_T0);
	}
	*/

	Point *op = &out_points[np];

	while (true) {

		SearchListInfo *splEnd = &searchPointList[splSize];

		/*
		for (SearchListInfo *spl = &searchPointList[1]; spl != splEnd; ++spl) {
		_mm_prefetch((char*)spl, _MM_HINT_T0);
		}
		*/

		SearchListInfo *splBest = searchPointList;
		int bestRank = searchPointList[0].rank;

		for (SearchListInfo *spl = &searchPointList[1]; spl != splEnd; ++spl) {
			if (spl->rank >= bestRank) {
			}
			else {
				_mm_prefetch((char*)spl->next, _MM_HINT_T0);
				bestRank = spl->rank;
				splBest = spl;
			}
		}
		SearchListInfo &bestElement = *splBest;


		int *hp = bestElement.next;
		hp++;

		const MyPoint &p = my_points[bestElement.rank];

		bool done = false;
		CHECK_AND_ADD_POINT;
		if (done)
			break;

		int dhp = *hp;
		if (dhp >= 0) {
			bestElement.rank = dhp; // sc->points[dhp];
			bestElement.next = hp;

			_mm_prefetch((char*)&my_points[bestElement.rank], _MM_HINT_T0);
		}
		else {
			// remove this element from the list
			splSize--;
			*splBest = searchPointList[splSize];

			if (splSize == 0)
				break;
		}
	}
}



__declspec(noinline)
void searchPartialSort(SearchListInfo *searchPointList, int splSize, int sortedSize, const MyPoint *my_points,
	const Rect rect, const int32_t count, Point* out_points, int &np)
{
	if (splSize == 0)
		return;

	Point *op = &out_points[np];

	SearchListInfo *sortedHead = searchPointList;
	SearchListInfo *sortedTail = sortedHead + sortedSize;

	int worstSortedRank = (sortedTail - 1)->rank;

	int unsortedSize = splSize - sortedSize;
	SearchListInfo *unsortedHead = searchPointList + sortedSize;
	SearchListInfo *unsortedTail = unsortedHead + unsortedSize;


	while (true) {

		// if we're out of sorted points, revert to linear search
		if (sortedSize == 0) {
			searchLinear(unsortedHead, unsortedSize, my_points, rect, count, out_points, np);
			return;
		}

		SearchListInfo &bestElement = *sortedHead;
		int *hp = bestElement.next;
		hp++;

		const MyPoint &p = my_points[bestElement.rank];

		bool done = false;
		CHECK_AND_ADD_POINT;
		if (done)
			break;

		int dhp = *hp;
		if (dhp > worstSortedRank) {
			// move to end of unsorted list
			unsortedTail->rank = dhp;
			unsortedTail->next = hp;
			unsortedTail++;
			unsortedSize++;

			sortedHead++;
			sortedSize--;
		}

		else if (dhp >= 0) {
			// update the sorted list
			SearchListInfo *ipos = sortedHead + 1;
			for (; ipos != sortedTail; ++ipos) {
				if (ipos->rank > dhp)
					break;

				ipos[-1] = ipos[0];
			}

			ipos--;
			ipos->rank = dhp; // sc->points[dhp];
			ipos->next = hp;
		}

		else {
			// remove this element from the list
			sortedHead++;
			sortedSize--;
		}
	}
}



void searchBubble(std::vector<SearchListInfo> *searchPointList_p, const MyPoint *my_points, 
	const Rect rect, const int32_t count, Point* out_points, int &np)
{
	std::vector<SearchListInfo> &searchPointList = *searchPointList_p;
	if (searchPointList.empty())
		return;

	Point *op = &out_points[np];

	while (true) {

		int bestIndex = (int)searchPointList.size()-1;

		SearchListInfo &bestElement = searchPointList[bestIndex];
		int *hp = bestElement.next;
		hp++;
		int dhp = *hp;

		const MyPoint &p = my_points[bestElement.rank];

		bool done = false;
		CHECK_AND_ADD_POINT;
		if (done)
			break;

		if (dhp < 0) {
			// remove this element from the list
			searchPointList[bestIndex] = searchPointList.back();
			searchPointList.pop_back();

			if (searchPointList.empty())
				break;
		}
		else {
			bestElement.rank = dhp; // sc->points[dhp];
			bestElement.next = hp;

			while (bestIndex > 0) {
				if (searchPointList[bestIndex - 1] < searchPointList[bestIndex])
					break;

				std::swap(searchPointList[bestIndex - 1], searchPointList[bestIndex]);
				bestIndex--;
			}
		}
	}
}


void searchHeap(std::vector<SearchListInfo> *searchPointList_p, const MyPoint *my_points,
	const Rect rect, const int32_t count, Point* out_points, int &np)
{
	std::vector<SearchListInfo> &searchPointList = *searchPointList_p;
	if (searchPointList.empty())
		return;

	Point *op = &out_points[np];

	while (true) {

		SearchListInfo &bestElement = searchPointList.front();
		int *hp = bestElement.next;
		hp++;
		int dhp = *hp;


		const MyPoint &p = my_points[bestElement.rank];

		bool done = false;
		CHECK_AND_ADD_POINT;
		if (done)
			break;

		// update the heap
		if (dhp >= 0) {
			// move the head to the end, replace it, then move back into heap
			std::pop_heap(searchPointList.begin(), searchPointList.end(), heapLessThan);
			searchPointList.back().rank = dhp; // sc->points[dhp];
			searchPointList.back().next = hp;
			std::push_heap(searchPointList.begin(), searchPointList.end(), heapLessThan);
		}
		else {

			// just move the head to the end and remove it
			std::pop_heap(searchPointList.begin(), searchPointList.end(), heapLessThan);
			searchPointList.pop_back();

			// if we're small enough now, switch to linear search
			if (searchPointList.size() <= MIN_HEAP_SIZE) {
				searchLinear(&searchPointList[0], (int)searchPointList.size(), my_points, rect, count, out_points, np);
				return;
			}

			if (searchPointList.empty())
				break;
		}
	}
}

void searchStart(std::vector<SearchListInfo> *searchPointList_p, const MyPoint *my_points,
	const Rect rect, const int32_t count, Point* out_points, int &np)
{
	np = 0;
	if (searchPointList_p->empty())
		return;


	std::vector<SearchListInfo> &searchPointList = *searchPointList_p;

	// use search list size to choose which seach method to use
	/*
	if (searchPointList.size() >= MIN_HEAP_SIZE) {
		std::make_heap(searchPointList.begin(), searchPointList.end(), heapLessThan);
		searchHeap(&searchPointList, my_points, rect, count, out_points, np);
	}
	else if (searchPointList.size() >= MIN_BUBBLE_SIZE) {
		std::sort(searchPointList.begin(), searchPointList.end(), heapLessThan);
		searchBubble(&searchPointList, my_points, rect, count, out_points, np);
	}
	else */ if (searchPointList.size() >= MIN_PARTIAL_SORT_SIZE) {
		int sortedSize = std::min((int)searchPointList.size(), (int)(PARTIAL_SORT_FRAC * count));
		std::partial_sort(searchPointList.begin(), searchPointList.begin() + sortedSize, searchPointList.end());
		searchPartialSort(&searchPointList[0], (int)searchPointList.size(), sortedSize,
			my_points, rect, count, out_points, np);
	}
	else {
		searchLinear(&searchPointList[0], (int)searchPointList.size(), my_points, rect, count, out_points, np);
	}

}


ZEROEPSILON_API int32_t __stdcall search(SearchContext* sc, const Rect rect, const int32_t count, Point* out_points)
{ 
	if (sc == NULL ||
		out_points == NULL ||
		count == 0)
		return 0;

	float width = rect.hx - rect.lx;
	float height = rect.hy - rect.ly;
	float haspect = height / width;
	float vaspect = width / height;

	int cellMinX[NUM_GRID_LEVELS];
	int cellMinY[NUM_GRID_LEVELS];
	int cellMaxX[NUM_GRID_LEVELS];
	int cellMaxY[NUM_GRID_LEVELS];
	int cellSpanX[NUM_GRID_LEVELS];
	int cellSpanY[NUM_GRID_LEVELS];

	Rect normRect;
	normRect.lx = std::max(0.f, std::min(1.f, ((rect.lx - sc->bounds.lx) / (sc->bounds.hx - sc->bounds.lx))));
	normRect.hx = std::max(0.f, std::min(1.f, ((rect.hx - sc->bounds.lx) / (sc->bounds.hx - sc->bounds.lx))));
	normRect.ly = std::max(0.f, std::min(1.f, ((rect.ly - sc->bounds.ly) / (sc->bounds.hy - sc->bounds.ly))));
	normRect.hy = std::max(0.f, std::min(1.f, ((rect.hy - sc->bounds.ly) / (sc->bounds.hy - sc->bounds.ly))));

	cellMinX[NUM_GRID_LEVELS - 1] = std::max((int)(normRect.lx * finestNumCells), 0);
	cellMaxX[NUM_GRID_LEVELS - 1] = std::min((int)(normRect.hx * finestNumCells), finestNumCells - 1);
	cellMinY[NUM_GRID_LEVELS - 1] = std::max((int)(normRect.ly * finestNumCells), 0);
	cellMaxY[NUM_GRID_LEVELS - 1] = std::min((int)(normRect.hy * finestNumCells), finestNumCells - 1);
	cellSpanX[NUM_GRID_LEVELS - 1] = cellMaxX[NUM_GRID_LEVELS - 1] - cellMinX[NUM_GRID_LEVELS - 1] + 1;
	cellSpanY[NUM_GRID_LEVELS - 1] = cellMaxY[NUM_GRID_LEVELS - 1] - cellMinY[NUM_GRID_LEVELS - 1] + 1;

	for (int i = NUM_GRID_LEVELS - 2; i >= 0; i--) {
		cellMinX[i] = cellMinX[i + 1] / LEVEL_MULT_FROM(i);
		cellMaxX[i] = cellMaxX[i + 1] / LEVEL_MULT_FROM(i);
		cellMinY[i] = cellMinY[i + 1] / LEVEL_MULT_FROM(i);
		cellMaxY[i] = cellMaxY[i + 1] / LEVEL_MULT_FROM(i);
		cellSpanX[i] = cellMaxX[i] - cellMinX[i] + 1;
		cellSpanY[i] = cellMaxY[i] - cellMinY[i] + 1;
	}

	// find the largest level we can find for each axis that has a span of at least 2
	int bestLevelX = NUM_GRID_LEVELS-1;
	for (int i = 0; i < NUM_GRID_LEVELS; i++) {
		if (cellSpanX[i] > 2) {
			bestLevelX = i;
			break;
		}
	}

	int bestLevelY = NUM_GRID_LEVELS-1;
	for (int i = 0; i < NUM_GRID_LEVELS; i++) {
		if (cellSpanY[i] > 2) {
			bestLevelY = i;
			break;
		}
	}


	// add internal cells on the best level, and cells from the next level around the edge
	int levelMinX = cellMinX[bestLevelX];
	int levelMaxX = cellMaxX[bestLevelX];
	int levelMinY = cellMinY[bestLevelY];
	int levelMaxY = cellMaxY[bestLevelY];
	int nextLevelMinX = cellMinX[bestLevelX] * LEVEL_MULT_FROM(bestLevelX);
	int nextLevelMaxX = (cellMaxX[bestLevelX] + 1) * LEVEL_MULT_FROM(bestLevelX) - 1;
	int nextLevelMinY = cellMinY[bestLevelY] * LEVEL_MULT_FROM(bestLevelY);
	int nextLevelMaxY = (cellMaxY[bestLevelY] + 1) * LEVEL_MULT_FROM(bestLevelY) - 1;

	bool subMinX = (bestLevelX != NUM_GRID_LEVELS - 1 && nextLevelMinX + SUB_THRESHOLD < cellMinX[bestLevelX + 1]);
	bool subMaxX = (bestLevelX != NUM_GRID_LEVELS - 1 && nextLevelMaxX - SUB_THRESHOLD > cellMaxX[bestLevelX + 1]);
	bool subMinY = (bestLevelY != NUM_GRID_LEVELS - 1 && nextLevelMinY + SUB_THRESHOLD < cellMinY[bestLevelY + 1]);
	bool subMaxY = (bestLevelY != NUM_GRID_LEVELS - 1 && nextLevelMaxY - SUB_THRESHOLD > cellMaxY[bestLevelY + 1]);


	if (subMinX) {
		levelMinX++;
		nextLevelMinX += LEVEL_MULT_FROM(bestLevelX);
	}
	if (subMaxX) {
		levelMaxX--;
		nextLevelMaxX -= LEVEL_MULT_FROM(bestLevelX);
	}
	if (subMinY) {
		levelMinY++;
		nextLevelMinY += LEVEL_MULT_FROM(bestLevelY);
	}
	if (subMaxY) {
		levelMaxY--;
		nextLevelMaxY -= LEVEL_MULT_FROM(bestLevelY);
	}


	sc->searchPointList.resize(MAX_SEARCH_CELLS);
	SearchListInfo *searchPointList = &sc->searchPointList[0];


	// internal cells from the best level
	{
		const PointGrid *pg = &sc->pointGrids[bestLevelX][bestLevelY];
		for (int yc = levelMinY; yc <= levelMaxY; ++yc) {
			for (int xc = levelMinX; xc <= levelMaxX; ++xc) {
				ADD_HEADER;
			}
		}
	}

	// next level cells to the left
	if (subMinX) {
		const PointGrid *pg = &sc->pointGrids[bestLevelX + 1][bestLevelY];
		for (int yc = levelMinY; yc <= levelMaxY; ++yc) {
			for (int xc = cellMinX[bestLevelX + 1]; xc < nextLevelMinX; ++xc) {
				ADD_HEADER;
			}
		}
	}

	// next level cells to the right
	if (subMaxX) {
		const PointGrid *pg = &sc->pointGrids[bestLevelX + 1][bestLevelY];
		for (int yc = levelMinY; yc <= levelMaxY; ++yc) {
			for (int xc = nextLevelMaxX + 1; xc <= cellMaxX[bestLevelX + 1]; ++xc) {
				ADD_HEADER;
			}
		}
	}

	// next level cells below
	if (subMinY) {
		const PointGrid *pg = &sc->pointGrids[bestLevelX][bestLevelY + 1];
		for (int yc = cellMinY[bestLevelY + 1]; yc < nextLevelMinY; ++yc) {
			for (int xc = levelMinX; xc <= levelMaxX; ++xc) {
				ADD_HEADER;
			}
		}
	}

	// next level cells above
	if (subMaxY) {
		const PointGrid *pg = &sc->pointGrids[bestLevelX][bestLevelY + 1];
		for (int yc = nextLevelMaxY + 1; yc <= cellMaxY[bestLevelY + 1]; ++yc) {
			for (int xc = levelMinX; xc <= levelMaxX; ++xc) {
				ADD_HEADER;
			}
		}
	}

	// left and below
	if (subMinX && subMinY) {
		const PointGrid *pg = &sc->pointGrids[bestLevelX + 1][bestLevelY + 1];
		for (int yc = cellMinY[bestLevelY + 1]; yc < nextLevelMinY; ++yc) {
			for (int xc = cellMinX[bestLevelX + 1]; xc < nextLevelMinX; ++xc) {
				ADD_HEADER;
			}
		}
	}

	// right and below
	if (subMaxX && subMinY) {
		const PointGrid *pg = &sc->pointGrids[bestLevelX + 1][bestLevelY + 1];
		for (int yc = cellMinY[bestLevelY + 1]; yc < nextLevelMinY; ++yc) {
			for (int xc = nextLevelMaxX + 1; xc <= cellMaxX[bestLevelX + 1]; ++xc) {
				ADD_HEADER;
			}
		}
	}

	// left and above
	if (subMinX && subMaxY) {
		const PointGrid *pg = &sc->pointGrids[bestLevelX + 1][bestLevelY + 1];
		for (int yc = nextLevelMaxY + 1; yc <= cellMaxY[bestLevelY + 1]; ++yc) {
			for (int xc = cellMinX[bestLevelX + 1]; xc < nextLevelMinX; ++xc) {
				ADD_HEADER;
			}
		}
	}

	// right and above
	if (subMaxX && subMaxY) {
		const PointGrid *pg = &sc->pointGrids[bestLevelX + 1][bestLevelY + 1];
		for (int yc = nextLevelMaxY + 1; yc <= cellMaxY[bestLevelY + 1]; ++yc) {
			for (int xc = nextLevelMaxX + 1; xc <= cellMaxX[bestLevelX + 1]; ++xc) {
				ADD_HEADER;
			}
		}
	}


	int numSearchCells = (int)(searchPointList - &sc->searchPointList[0]);
	if (numSearchCells > MAX_SEARCH_CELLS) {
		printf("whoops!\n");
		exit(1);
	}
	sc->searchPointList.resize(numSearchCells);


	int np = 0;
	searchStart(&sc->searchPointList, &sc->points[0], rect, count, out_points, np);

	return np;
}


/* Release the resources associated with the context. Return nullptr if successful, "sc" otherwise. */
ZEROEPSILON_API SearchContext* __stdcall destroy(SearchContext* sc)
{
	if (sc) {
		delete sc;
	}

	return NULL;
}

