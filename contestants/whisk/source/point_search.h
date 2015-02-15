#ifndef POINT_SEARCH_HEADER
#define POINT_SEARCH_HEADER
#include <stdint.h>

#pragma pack(push, 1)
struct Point {
	int8_t  id;
	int32_t rank;
	float x;
	float y;
};

struct Rect {
	float lx;
	float ly;
	float hx;
	float hy;
};
#pragma pack(pop)

struct SearchContext;

#endif