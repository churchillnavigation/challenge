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
	Rect* crect;
	int n;
	Point* ranksort;
	Region* left;
	Region* right;
	Region* lrmid;
	Region* bottom;
	Region* top;
	Region* btmid;

	float subw, subh;
};

struct GumpSearchContext {
	int32_t N;

	// Binary search
	Point* xsort;
	Point* ysort;

	// Region search
	Region* root;
	Point* ranksort;
	Rect* trim;

	// Grid search
	Point* gridsort;
	Point*** grid;
	Rect** grect;
	Rect** drect;
	int** dlen;
	Rect* bounds;
	double area;
	double dx, dy;

	// Current search
	Point** blocks;
	int* blocki;
	int* blockn;
	float w;
	float h;
};

SearchContext* __stdcall DLL_API create(const Point* points_begin, const Point* points_end);
int32_t __stdcall DLL_API search(SearchContext* sc, const Rect rect, const int32_t count, Point* out_points);
SearchContext* __stdcall DLL_API destroy(SearchContext* sc);

#ifdef __cplusplus
}
#endif