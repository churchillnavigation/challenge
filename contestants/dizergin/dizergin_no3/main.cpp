#include "point_search.h"
#include "stdlib.h"



#define MAXSTACK 1024
#define BUFF_SIZE 2000
#define BASIC_CUT 1000000
#define ADVANCED_CUT 72500
#define ADVANCED_CUT1 12500
#define MP_COUNT 40
typedef int32_t Tsize;

struct SearchContext{
	Tsize count;
	Tsize *buff;
	Tsize *buff1;

	int8_t *r_id;
	int32_t *r_rank;
	float *r_x;
	float *r_y;

	int8_t *x_id;
	int32_t *x_rank;
	float *x_x;
	float *x_y;

	int8_t *y_id;
	int32_t *y_rank;
	float *y_x;
	float *y_y;

	int32_t **ppbuff;

	Rect bounds;
	float s;
};

static SearchContext *context_create(const Point *points_begin, const Point *points_end)
{


	Tsize count = points_end - points_begin;
	if (count < 1) return nullptr;


	Point *ind_r = (Point *)calloc(count, sizeof(Point));
	ind_r[0:count] = points_begin[0:count];
	
//#pragma omp section
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
	int8_t *r_id = (int8_t *)calloc(count, sizeof(int8_t));
	int32_t *r_rank = (int32_t *)calloc(count, sizeof(int32_t));
	float *r_x = (float *)calloc(count, sizeof(float));
	float *r_y = (float *)calloc(count, sizeof(float));
	r_id[0:count] = ind_r[0:count].id;
	r_rank[0:count] = ind_r[0:count].rank;
	r_x[0:count] = ind_r[0:count].x;
	r_y[0:count] = ind_r[0:count].y;
	free(ind_r);

	Point *ind_x = (Point *)calloc(count, sizeof(Point));
	ind_x[0:count] = points_begin[0:count];
//#pragma omp section
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

	int8_t *x_id = (int8_t *)calloc(count, sizeof(int8_t));
	int32_t *x_rank = (int32_t *)calloc(count, sizeof(int32_t));
	float *x_x = (float *)calloc(count, sizeof(float));
	float *x_y = (float *)calloc(count, sizeof(float));
	x_id[0:count] = ind_x[0:count].id;
	x_rank[0:count] = ind_x[0:count].rank;
	x_x[0:count] = ind_x[0:count].x;
	x_y[0:count] = ind_x[0:count].y;
	free(ind_x);


	Point *ind_y = (Point *)calloc(count, sizeof(Point));
	ind_y[0:count] = points_begin[0:count];

//#pragma omp section
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

	int8_t *y_id = (int8_t *)calloc(count, sizeof(int8_t));
	int32_t *y_rank = (int32_t *)calloc(count, sizeof(int32_t));
	float *y_x = (float *)calloc(count, sizeof(float));
	float *y_y = (float *)calloc(count, sizeof(float));
	y_id[0:count] = ind_y[0:count].id;
	y_rank[0:count] = ind_y[0:count].rank;
	y_x[0:count] = ind_y[0:count].x;
	y_y[0:count] = ind_y[0:count].y;
	free(ind_y);


float   minx = x_x[0],
		miny = y_y[0],
		maxx = x_x[count - 1],
		maxy = y_y[count - 1];

float *pminx=x_x;
while (minx == (*pminx)) pminx++;

float *pminy = y_y;
while (miny == (*pminy)) pminy++;

float *pmaxx = &x_x[count-1];
while (maxx == (*pmaxx)) pmaxx--;

float *pmaxy = &y_y[count - 1];
while (maxy == (*pmaxy)) pmaxy--;

Rect bounds{ *pminx, *pminy, *pmaxx, *pmaxy };

float s = (bounds.hx - bounds.lx) * (bounds.hy - bounds.ly);



Tsize *buff = (Tsize *)calloc(BUFF_SIZE, sizeof(Tsize));
Tsize *buff1 = (Tsize *)calloc(ADVANCED_CUT, sizeof(Tsize));
int32_t **ppbuff = (int32_t **)calloc(ADVANCED_CUT, sizeof(int32_t*));



SearchContext *sc = (SearchContext *)malloc(sizeof(SearchContext));
sc->bounds = bounds;
sc->buff = buff;
sc->buff1 = buff1;
sc->ppbuff = ppbuff;
sc->count = count;

sc->r_id = r_id;
sc->r_x = r_x;
sc->r_y = r_y;
sc->r_rank = r_rank;

sc->x_id = x_id;
sc->x_x = x_x;
sc->x_y = x_y;
sc->x_rank = x_rank;

sc->y_id = y_id;
sc->y_x = y_x;
sc->y_y = y_y;
sc->y_rank = y_rank;

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
		free(sc->buff1);

		free(sc->r_id);
		free(sc->r_rank);
		free(sc->r_x);
		free(sc->r_y);

		free(sc->x_id);
		free(sc->x_rank);
		free(sc->x_x);
		free(sc->x_y);

		free(sc->y_id);
		free(sc->y_rank);
		free(sc->y_x);
		free(sc->y_y);

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
		sc->buff[0:grsize] = rect.lx <= sc->r_x[i:grsize] && rect.hx >= sc->r_x[i:grsize] && rect.ly <= sc->r_y[i:grsize] && rect.hy >= sc->r_y[i:grsize];
		for (auto j = 0; j < grsize && pcount != count; j++)
			if (sc->buff[j]){
				out_points->id= *(sc->r_id + i + j);
				out_points->rank = *(sc->r_rank + i + j);
				out_points->x = *(sc->r_x + i + j);
				out_points->y = *(sc->r_y + i + j);
				out_points++;
				pcount++;
			}
		if (pcount == count) return pcount;
	}
	return pcount;
}

inline void FindM(int32_t **P, int32_t count, int32_t k, int32_t L, int32_t R){
	int32_t *x, *w;
	while (L < R) {
		x = P[k];
		register int32_t i = L, j = R;
		do
		{
			while (*P[i] < *x) i++;
			while (*x < *P[j]) j--;
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
		if (rect.hx < sc->x_x[0]) maxX = -1;
		else if (rect.hx >= sc->x_x[sc->count - 1]) maxX = sc->count - 1;
		else {
			left = 0; right = sc->count - 1;
			while (left < right - 1) {
				register int node = (left + right) >> 1;
				if (rect.hx >= sc->x_x[node]) left = node;
				else right = node;
			}
			maxX = left;
		}
	}
	//Find maxY
	{
		register	Tsize left, right;
		if (rect.hy < sc->y_y[0]) maxY = -1;
		else if (rect.hy >= sc->y_y[sc->count - 1]) maxY = sc->count - 1;
		else {
			left = 0; right = sc->count - 1;
			while (left < right - 1) {
				register int node = (left + right) >> 1;
				if (rect.hy >= sc->y_y[node]) left = node;
				else right = node;
			}
			maxY = left;
		}
	}
	
	//Find minX
	{ register	Tsize left, right;
	if (rect.lx <= sc->x_x[0]) minX = 0;
	else if (rect.lx > sc->x_x[sc->count - 1]) minX = sc->count;
	else {
		left = 0; right = sc->count - 1;
		while (left < right - 1) {
			register int node = (left + right) >> 1;
			if (rect.lx <= sc->x_x[node]) right = node;
			else left = node;
		}
		minX = right;
	}
	}

	//Find minY
	{ register	Tsize left, right;
	if (rect.ly <= sc->y_y[0]) minY = 0;
	else if (rect.ly > sc->y_y[sc->count - 1]) minY = sc->count;
	else {
		left = 0; right = sc->count - 1;
		while (left < right - 1) {
			register int node = (left + right) >> 1;
			if (rect.ly <= sc->y_y[node]) right = node;
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
	int flag;
	if (numx < numy){
		flag = 0;
		sc->buff1[0:numx + 1] = rect.ly <= sc->x_y[minX:maxX + 1] && rect.hy >= sc->x_y[minX:maxX + 1];
		for  (Tsize i = 0; i < numx+1 ; i++)
			if (sc->buff1[i])
			{
				sc->ppbuff[k] = &sc->x_rank[i+minX];
				k++;
			}
	} else  {
		flag = 1;
		sc->buff1[0:numy + 1] = rect.lx <= sc->y_x[minY:maxY + 1] && rect.hx >= sc->y_x[minY:maxY + 1];
		for (Tsize i = 0; i < numy + 1; i++)
			if (sc->buff1[i])
			{
				sc->ppbuff[k] = &sc->y_rank[i + minY];
				k++;
			}
		}
	if (k > ADVANCED_CUT1) return basicsolution(sc, rect, count, out_points);

	Tsize pcount = count < k ? count : k;
	int ind;
	if (flag)
		for (Tsize i = 0; i < pcount; i++){
		FindM(sc->ppbuff, k, i, i, k - 1);
		ind = sc->ppbuff[i] - sc->y_rank;
		out_points->id = *(sc->y_id + ind);
		out_points->rank = *(sc->y_rank + ind);
		out_points->x = *(sc->y_x + ind);
		out_points->y = *(sc->y_y + ind);
		out_points++;

	}  else   
			for (Tsize i = 0; i < pcount; i++){
			FindM(sc->ppbuff, k, i, i, k - 1);
			ind = sc->ppbuff[i] - sc->x_rank;
			out_points->id = *(sc->x_id + ind);
			out_points->rank = *(sc->x_rank + ind);
			out_points->x = *(sc->x_x + ind);
			out_points->y = *(sc->x_y + ind);
			out_points++;
		}
	return pcount;

}
