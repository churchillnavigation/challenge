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

	int CreateBlock(BlockNode* nodeBlocks, Block* pointBlocks, int & nodeBlocksCount, int & pointBlocksCount, 
						std::vector<std::tuple<Node*, BlockNode*, int>> & blocksToCreate, int currentLevel);

	inline void CalculateBlockCount(int & nodeBlocksCount, int & pointBlocksCount) const {
		nodeBlocksCount++;
		pointBlocksCount += (int)(std::ceil((double)PointsCount / SimdSize)) * 2;
		for (int nodeIndex = 0; nodeIndex < ChildCount; nodeIndex++) {
			Children[nodeIndex]->CalculateBlockCount(nodeBlocksCount, pointBlocksCount);
		}
	}

	void RemoveEmptyNodes();

	~Node();
private:
	void CreateChildren(Allocator<Node> & nodeAllocator);

	void OptimizeBounds();

	Rect* GetBoundingBoxesForChildren() const;

};

