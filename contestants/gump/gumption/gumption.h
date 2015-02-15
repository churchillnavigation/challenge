#ifndef GUMP_H
#define GUMP_H

#include "point_search.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef EXPORT_DLL
#define DLL_API __declspec(dllexport)
#else
#define DLL_API __declspec(dllimport)
#endif

struct Region {
	Rect* rect;
	int n;
	Point* ranksort;
	Region* left;
	Region* right;
	Region* lrmid;
	Region* bottom;
	Region* top;
	Region* btmid;
};

struct GumpSearchContext {
	int32_t N;

	// Binary search
	Point* xsort;
	Point* ysort;
	float* xvalsort;
	float* yvalsort;

	// Region search
	Region* root;
	Rect* trim;

	// Grid search
	Point* gridsort;
	Point*** grid;
	Rect** rect;
	int** rlen;
	Rect* bounds;
	double area;
	double dx, dy;
	Point** blocks;
	int* blocki;
	int* blockn;
};

SearchContext* __stdcall DLL_API create(const Point* points_begin, const Point* points_end);
int32_t __stdcall DLL_API search(SearchContext* sc, const Rect rect, const int32_t count, Point* out_points);
SearchContext* __stdcall DLL_API destroy(SearchContext* sc);

#ifdef __cplusplus
}
#endif

#endif