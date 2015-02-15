#pragma once

#include <vector>
#include <queue>

#include "Rect.h"
#include "Definitions.h"

struct BlockNode {
	int MinimumRank;
	int MaximumRank;
	int PointsCount;
	int ChildCount;
	BlockNode* Children[NodeChildCount];
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
	BlockNode Node;
	BlockPointCoordinates Coordinates;
	BlockPointAttributes Attributes;
};


class BlockNodeLevelCompare
{
public:
	bool operator() (BlockNode* & nodeA, BlockNode* & nodeB) const;
};



#define NodeQueue std::priority_queue<BlockNode*, std::vector<BlockNode*>, BlockNodeLevelCompare>