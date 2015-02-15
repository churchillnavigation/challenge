#include "point_search.h"
#include "stdlib.h"

#define MAXSTACK 1024
#define BUFF_SIZE 4096
typedef int32_t Tsize;

struct SearchContext{
	Tsize count;
	Tsize *buff;
	Point *ind_r;
    float *i_x,
		  *i_y;
};

static SearchContext *context_create(const Point *points_begin, const Point *points_end)
{


	Tsize count = points_end - points_begin;
	//if (count < 1) return nullptr;

	Tsize *buff = (Tsize *)calloc(BUFF_SIZE, sizeof(Tsize));
	Point *ind_r = (Point *)calloc(count, sizeof(Point));

	ind_r[0:count] = points_begin[0:count];

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




	float *i_x = (float *)calloc(count, sizeof(float));
	float *i_y = (float *)calloc(count, sizeof(float));
	i_x[0:count] = ind_r[0:count].x;
	i_y[0:count] = ind_r[0:count].y;

SearchContext *sc = (SearchContext *)malloc(sizeof(SearchContext));

sc->buff = buff;
sc->count = count;
sc->i_x = i_x;
sc->i_y = i_y;
sc->ind_r = ind_r;
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
	//if (sc){
		free(sc->buff);
		free(sc->i_x);
		free(sc->i_y);
		free(sc->ind_r);
		free(sc);
	//}
	return nullptr;
}
/////////////////////////////////
////////////////////////////////


extern "C" __declspec(dllexport) int32_t __stdcall search(SearchContext* sc, const Rect rect, const int32_t count, Point* out_points)
{
	//if (!sc) return 0;

	Tsize pcount = 0;
	Tsize grsize = BUFF_SIZE;

	for (auto i = 0; i < sc->count; i += BUFF_SIZE){
		if (i + BUFF_SIZE > sc->count) grsize = sc->count - i;
		sc->buff[0:grsize] = rect.lx <= sc->i_x[i:grsize] && rect.hx >= sc->i_x[i:grsize] && rect.ly <= sc->i_y[i:grsize] && rect.hy >= sc->i_y[i:grsize];
		for (auto j = 0; j < grsize && pcount != count; j++)
			if (*(sc->buff+j)){
				*out_points = *(sc->ind_r + i + j);
				out_points++;
				pcount++;
			}
		if (pcount == count) return pcount;
	}

	return pcount;

}
