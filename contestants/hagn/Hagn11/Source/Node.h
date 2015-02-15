#pragma once

#include <vector>
#include <ppl.h>
#include <algorithm>
#include <limits>
#include <queue>

#include "Definitions.h"
#include "Rect.h"
#include "Point.h"
#include "Allocator.h"
#include "Block.h"
#include "Helpers.h"

class Node {

public:
	std::vector<Point> * Points;
	int ChildCount = 0;
	int PointsCount = 0;
	int MinimumRank = 0;
	int MaximumRank = 0;
	int PointStartIndex = 0;
	Rect Bounds;
	Node* Children[NodeChildCount];

	void AddPoints(int startIndex, int count, std::vector<Point> * points);

	void MovePointsToChildren(const int rankLimit, Allocator<Node> & nodeAllocator, std::vector<Node*> & createdNodes);

	BlockNode* CreateBlock(std::vector<Block> & blocks, std::vector<std::pair<const Node*, BlockNode**>> & blocksToCreate) const;

	inline int CalculateBlockCount() const {
		int count = 1;
		count += (int)(std::ceil((double)PointsCount / SimdSize)) * 2;
		for (int nodeIndex = 0; nodeIndex < ChildCount; nodeIndex++) {
			count += Children[nodeIndex]->CalculateBlockCount();
		}
		return count;
	}

	void RemoveEmptyNodes();

	~Node();
private:
	void CreateChildren(Allocator<Node> & nodeAllocator);

	void OptimizeBounds();

	Rect GetBoundsForHalf(int index) const;

};

