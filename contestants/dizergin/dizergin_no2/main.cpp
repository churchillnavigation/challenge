#include "point_search.h"
#include "stdlib.h"



#define MAXSTACK 1024
#define BUFF_SIZE 5000
#define BASIC_CUT 500000
#define ADVANCED_CUT 20000
#define MP_COUNT 40
typedef int32_t Tsize;

struct SearchContext{
	Tsize count;
	Tsize *buff;
	Point *ind_r,
		  *ind_x,
		  *ind_y,
		  **ppbuff;
	float *i_x;
	float *i_y;
	Rect bounds;
	float s;
};

static SearchContext *context_create(const Point *points_begin, const Point *points_end)
{


	Tsize count = points_end - points_begin;
	if (count < 1) return nullptr;

	Tsize *buff = (Tsize *)calloc(BUFF_SIZE, sizeof(Tsize));
	Point *ind_x = (Point *)calloc(count, sizeof(Point));
	Point *ind_y = (Point *)calloc(count, sizeof(Point));
	Point *ind_r = (Point *)calloc(count, sizeof(Point));
	Point **ppbuff = (Point **)calloc(ADVANCED_CUT, sizeof(Point*));

	ind_y[0:count] = points_begin[0:count];
	ind_x[0:count] = points_begin[0:count];
	ind_r[0:count] = points_begin[0:count];

/// Hate slow program!!! so let it go  in  parallel.....
#pragma omp parallel sections
	{
#pragma omp section
	{
		Tsize  i, j;   			// указатели, участвующие в разделении

		Tsize lb, ub;  		// границы сортируемого в цикле фрагмента

		Tsize lbstack[MAXSTACK], ubstack[MAXSTACK]; // стек запросов

		Tsize stackpos;   	// текуща€ позици€ стека
		Tsize ppos;            // середина массива

		Point pivot;              // опорный элемент
		Point temp;

		stackpos = 1;
		lbstack[1] = 0;
		ubstack[1] = count - 1;
		do {

			// ¬з€ть границы lb и ub текущего массива из стека.

			lb = lbstack[stackpos];
			ub = ubstack[stackpos];
			stackpos--;

			do {
				// Ўаг 1. –азделение по элементу pivot

				ppos = lb + ((ub - lb) >> 1);
				i = lb; j = ub; pivot = ind_r[ppos];

				do {
					while (ind_r[i].rank < pivot.rank) i++;
					while (pivot.rank < ind_r[j].rank) j--;

					if (i <= j) {
						temp = ind_r[i]; ind_r[i] = ind_r[j]; ind_r[j] = temp;
						i++; j--;
					}
				} while (i <= j);

				if (i < ppos) {     // права€ часть больше

					if (i < ub) {     //  если в ней больше 1 элемента - нужно 
						stackpos++;       //  сортировать, запрос в стек
						lbstack[stackpos] = i;
						ubstack[stackpos] = ub;
					}
					ub = j;

				}
				else {       	    // лева€ часть больше

					if (j > lb) {
						stackpos++;
						lbstack[stackpos] = lb;
						ubstack[stackpos] = j;
					}
					lb = i;
				}

			} while (lb < ub);        // пока в меньшей части более 1 элемента

		} while (stackpos != 0);    // пока есть запросы в стеке
	}

#pragma omp section
	{
	Tsize  i, j;   			// указатели, участвующие в разделении

	Tsize lb, ub;  		// границы сортируемого в цикле фрагмента

	Tsize lbstack[MAXSTACK], ubstack[MAXSTACK]; // стек запросов

	Tsize stackpos;   	// текуща€ позици€ стека
	Tsize ppos;            // середина массива

	Point pivot;              // опорный элемент
	Point temp;

	stackpos = 1;
	lbstack[1] = 0;
	ubstack[1] = count - 1;
	do {

		// ¬з€ть границы lb и ub текущего массива из стека.

		lb = lbstack[stackpos];
		ub = ubstack[stackpos];
		stackpos--;

		do {
			// Ўаг 1. –азделение по элементу pivot

			ppos = lb + ((ub - lb) >> 1);
			i = lb; j = ub; pivot = ind_x[ppos];

			do {
				while (ind_x[i].x < pivot.x) i++;
				while (pivot.x < ind_x[j].x) j--;

				if (i <= j) {
					temp = ind_x[i]; ind_x[i] = ind_x[j]; ind_x[j] = temp;
					i++; j--;
				}
			} while (i <= j);

			if (i < ppos) {     // права€ часть больше

				if (i < ub) {     //  если в ней больше 1 элемента - нужно 
					stackpos++;       //  сортировать, запрос в стек
					lbstack[stackpos] = i;
					ubstack[stackpos] = ub;
				}
				ub = j;

			}
			else {       	    // лева€ часть больше

				if (j > lb) {
					stackpos++;
					lbstack[stackpos] = lb;
					ubstack[stackpos] = j;
				}
				lb = i;
			}

		} while (lb < ub);        // пока в меньшей части более 1 элемента

	} while (stackpos != 0);    // пока есть запросы в стеке
	}

#pragma omp section
	{
	Tsize  i, j;   			// указатели, участвующие в разделении

	Tsize lb, ub;  		// границы сортируемого в цикле фрагмента

	Tsize lbstack[MAXSTACK], ubstack[MAXSTACK]; // стек запросов

	Tsize stackpos;   	// текуща€ позици€ стека
	Tsize ppos;            // середина массива

	Point pivot;              // опорный элемент
	Point temp;

	stackpos = 1;
	lbstack[1] = 0;
	ubstack[1] = count - 1;
	do {

		// ¬з€ть границы lb и ub текущего массива из стека.

		lb = lbstack[stackpos];
		ub = ubstack[stackpos];
		stackpos--;

		do {
			// Ўаг 1. –азделение по элементу pivot

			ppos = lb + ((ub - lb) >> 1);
			i = lb; j = ub; pivot = ind_y[ppos];

			do {
				while (ind_y[i].y < pivot.y) i++;
				while (pivot.y < ind_y[j].y) j--;

				if (i <= j) {
					temp = ind_y[i]; ind_y[i] = ind_y[j]; ind_y[j] = temp;
					i++; j--;
				}
			} while (i <= j);

			if (i < ppos) {     // права€ часть больше

				if (i < ub) {     //  если в ней больше 1 элемента - нужно 
					stackpos++;       //  сортировать, запрос в стек
					lbstack[stackpos] = i;
					ubstack[stackpos] = ub;
				}
				ub = j;

			}
			else {       	    // лева€ часть больше

				if (j > lb) {
					stackpos++;
					lbstack[stackpos] = lb;
					ubstack[stackpos] = j;
				}
				lb = i;
			}

		} while (lb < ub);        // пока в меньшей части более 1 элемента

	} while (stackpos != 0);    // пока есть запросы в стеке
	}

	}
	float *i_x = (float *)calloc(count, sizeof(float));
	float *i_y = (float *)calloc(count, sizeof(float));
	i_x[0:count] = ind_r[0:count].x;
	i_y[0:count] = ind_r[0:count].y;


float   minx = ind_x[0].x,
		miny = ind_y[0].y,
		maxx = ind_x[count - 1].x,
		maxy = ind_y[count - 1].y;

Point *pminx=ind_x;
while (minx == (*pminx).x) pminx++;

Point *pminy = ind_y;
while (miny == (*pminy).y) pminy++;

Point *pmaxx = &ind_x[count-1];
while (maxx == (*pmaxx).x) pmaxx--;

Point *pmaxy = &ind_y[count - 1];
while (maxy == (*pmaxy).y) pmaxy--;

Rect bounds{ pminx->x, pminy->y, pmaxx->x, pmaxy->y };

float s = (bounds.hx - bounds.lx) * (bounds.hy - bounds.ly);

SearchContext *sc = (SearchContext *)malloc(sizeof(SearchContext));
sc->bounds = bounds;
sc->buff = buff;
sc->ppbuff = ppbuff;
sc->count = count;
sc->ind_x = ind_x;
sc->ind_y = ind_y;
sc->ind_r = ind_r;
sc->i_x = i_x;
sc->i_y = i_y;
sc->s = s;
return sc;
}



//////////////////////////////////
/////////////////////////////////
extern "C" __declspec(dllexport) SearchContext* __stdcall create(const Point* points_begin, const Point* points_end)
{
	return context_create(points_begin, points_end);
}



extern "C" __declspec(dllexport) SearchContext* __stdcall destroy(SearchContext* sc)
{
	if (sc){
		free(sc->ppbuff);
		free(sc->buff);
		free(sc->ind_x);
		free(sc->ind_y);
		free(sc->ind_r);
		free(sc);
	}
	return nullptr;
}
/////////////////////////////////
////////////////////////////////

inline float get_int_square(const Rect &rout, const Rect &rin)
{
	float dx = (rout.hx < rin.hx ? rout.hx : rin.hx) - (rout.lx > rin.lx ? rout.lx : rin.lx);
	float dy = (rout.hy < rin.hy ? rout.hy : rin.hy) - (rout.ly > rin.ly ? rout.ly : rin.ly);
	return dx > 0 && dy > 0 ? dx * dy : 0;
}

inline int32_t basicsolution(SearchContext* sc, const Rect rect, const int32_t count, Point* out_points)
{
	Tsize pcount = 0;
	Tsize grsize = BUFF_SIZE;

	for (auto i = 0; i < sc->count; i += BUFF_SIZE){
		if (i + BUFF_SIZE > sc->count) grsize = sc->count - i;
		sc->buff[0:grsize] = rect.lx <= sc->i_x[i:grsize] && rect.hx >= sc->i_x[i:grsize] && rect.ly <= sc->i_y[i:grsize] && rect.hy >= sc->i_y[i:grsize];
		for (auto j = 0; j < grsize && pcount != count; j++)
			if (sc->buff[j]){
				*out_points = *(sc->ind_r + i + j);
				out_points++;
				pcount++;
			}
		if (pcount == count) return pcount;
	}
	return pcount;
}

inline void FindM(Point **P, int32_t count, int32_t k, int32_t L, int32_t R){
	Point *x, *w;
	while (L < R) {
		x = P[k];
		register int32_t i = L, j = R;
		do
		{
			while (P[i]->rank < x->rank) i++;
			while (x->rank < P[j]->rank) j--;
			if (i <= j)
			{
				w = P[i]; P[i] = P[j]; P[j] = w; i++; j--;
			}
		} while (i <= j);
		if (j < k) L = i;
		if (k < i) R = j;
	}
}


extern "C" __declspec(dllexport) int32_t __stdcall search(SearchContext* sc, const Rect rect, const int32_t count, Point* out_points)
{
	if (!sc) return 0;

	if (get_int_square(sc->bounds, rect) / sc->s * sc->count > BASIC_CUT) return basicsolution(sc, rect, count, out_points);

	Tsize maxX,
		maxY,
		minX,
		minY; 

		//Find maxX
	{
		register	Tsize left, right;
		if (rect.hx < sc->ind_x[0].x) maxX = -1;
		else if (rect.hx >= sc->ind_x[sc->count - 1].x) maxX = sc->count - 1;
		else {
			left = 0; right = sc->count - 1;
			while (left < right - 1) {
				register int node = (left + right) >> 1;
				if (rect.hx >= sc->ind_x[node].x) left = node;
				else right = node;
			}
			maxX = left;
		}
	}
	//Find maxY
	{
		register	Tsize left, right;
		if (rect.hy < sc->ind_y[0].y) maxY = -1;
		else if (rect.hy >= sc->ind_y[sc->count - 1].y) maxY = sc->count - 1;
		else {
			left = 0; right = sc->count - 1;
			while (left < right - 1) {
				register int node = (left + right) >> 1;
				if (rect.hy >= sc->ind_y[node].y) left = node;
				else right = node;
			}
			maxY = left;
		}
	}
	
	//Find minX
	{ register	Tsize left, right;
	if (rect.lx <= sc->ind_x[0].x) minX = 0;
	else if (rect.lx > sc->ind_x[sc->count - 1].x) minX = sc->count;
	else {
		left = 0; right = sc->count - 1;
		while (left < right - 1) {
			register int node = (left + right) >> 1;
			if (rect.lx <= sc->ind_x[node].x) right = node;
			else left = node;
		}
		minX = right;
	}
	}

	//Find minY
	{ register	Tsize left, right;
	if (rect.ly <= sc->ind_y[0].y) minY = 0;
	else if (rect.ly > sc->ind_y[sc->count - 1].y) minY = sc->count;
	else {
		left = 0; right = sc->count - 1;
		while (left < right - 1) {
			register int node = (left + right) >> 1;
			if (rect.ly <= sc->ind_y[node].y) right = node;
			else left = node;
		}
		minY = right;
	}
	}
	

	int32_t numx = maxX - minX,
			numy = maxY - minY;

	if ((numx < 0) || (numy < 0)) return 0;

	int32_t minXY = numx < numy ? numx : numy;
	if (minXY > ADVANCED_CUT) return basicsolution(sc, rect, count, out_points);

	Tsize	k = 0;

	if (numx<numy)
		for  (Tsize i = minX; i < maxX+1 ; i++){
		if (rect.ly <= sc->ind_x[i].y && rect.hy >= sc->ind_x[i].y)
			{
				sc->ppbuff[k] = &sc->ind_x[i];
				k++;
			}
	}  else  
		for (Tsize i = minY; i < maxY + 1; i++){
			if (rect.lx <= sc->ind_y[i].x && rect.hx >= sc->ind_y[i].x)
			{
				sc->ppbuff[k] = &sc->ind_y[i];
				k++;
			}
		}

	Tsize pcount = count < k ? count : k;
	

		for (Tsize i = 0; i < pcount; i++){
		FindM(sc->ppbuff, k, i, i, k - 1);
		out_points[i] =  *(sc->ppbuff[i]);		
	}  
	static Tsize l = 0;

	return pcount;

}
