//Brian Thorpe 2015
#ifndef COMMON_STRUCTS
#define COMMON_STRUCTS

#include <stdint.h>

const uint32_t MAX_TREE_SIZE = (1<<31);
const float FMAX_TREE_SIZE = (float)MAX_TREE_SIZE;
const int32_t MAX_TREE_DEPTH = 16;
const int32_t MAX_TREE_POINTS = 4096;

#pragma pack(push,1)
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

struct SearchContext;

#endif