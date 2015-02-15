#include <stdint.h>
#include <iostream>
#include <algorithm>

#pragma pack(push, 1)

struct Point
{
	int8_t id;
	int32_t rank;
	float x;
	float y;
};


struct Rect
{
	float lx;
	float ly;
	float hx;
	float hy;
};
#pragma pack(pop)

struct SearchContext{
};

#define DLL_EXP __declspec(dllexport)

#ifdef __linux__ 

#define __stdcall  
#define DLL_EXP 

#endif


typedef SearchContext* (__stdcall* T_create)(const Point* points_begin, const Point* points_end);

typedef int32_t (__stdcall* T_search)(SearchContext* sc, const Rect rect, const int32_t count, Point* out_points);

typedef SearchContext* (__stdcall* T_destroy)(SearchContext* sc);
extern "C"{//EXTERN C RAZEM Z DECLSPEC ZAPEWNIA BRAK DEKORACJI FUNKCJI W DLL'U


SearchContext* __stdcall DLL_EXP create(const Point* points_begin, const Point* points_end);

int32_t __stdcall DLL_EXP search(SearchContext* sc, const Rect rect, const int32_t count, Point* out_points);

SearchContext* __stdcall DLL_EXP destroy(SearchContext* sc);
}


