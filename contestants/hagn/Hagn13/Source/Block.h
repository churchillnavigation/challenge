#pragma once

#include <vector>
#include <queue>
#include <immintrin.h>
#include <stdint.h>

#include "Rect.h"
#include "Definitions.h"


struct BlockNode {
private:
	int _padding;
public:
	int PointsCount;
	int ChildCount;
	int MaximumRank;
	int MinimumRank;
	int PointsStartIndex;
	int ChildIndex[NodeChildCount];
	Rect ChildBounds[NodeChildCount];
};


struct BlockPointCoordinates {
	float X[SimdSize];
	float Y[SimdSize];
};

struct BlockPointAttributes {
	int Id[SimdSize];
	int Rank[SimdSize];
};

union Block {
	BlockPointCoordinates Coordinates;
	BlockPointAttributes Attributes;
};
