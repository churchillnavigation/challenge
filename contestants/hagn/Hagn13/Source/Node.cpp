#include "stdafx.h"
#include "Node.h"



void Node::AddPoints(int startIndex, int count, std::vector<Point> * points)
{
	Points = points;
	PointStartIndex = startIndex;
	PointsCount = count;
	if (count > 0) {
		MinimumRank = (*Points)[PointStartIndex].rank;
	}
}

void Node::MovePointsToChildren(const int pointsLimit, Allocator<Node> & nodeAllocator, std::vector<Node*> & createdNodes) {
	if (PointsCount == 0) return;

	OptimizeBounds();

	CreateChildren(nodeAllocator);

	int pointsUnderLimit = std::min(pointsLimit, (int)PointsCount);
	if (pointsUnderLimit > 0) {
		MaximumRank = (*Points)[PointStartIndex + pointsUnderLimit - 1].rank;
		std::sort(Points->begin() + PointStartIndex, Points->begin() + PointStartIndex + pointsUnderLimit, [](const Point & pointA, const Point & pointB){ return pointA.x < pointB.x; });
	}

	int childPointsStartIndex = PointStartIndex + pointsUnderLimit;
	const int childPointsLastIndex = PointStartIndex + PointsCount;
	for (Node* & child : Children) {
		auto firstPoint = Points->begin() + childPointsStartIndex;
		auto pointOfSecondGroup = std::stable_partition(firstPoint, Points->begin() + childPointsLastIndex, BoundsContains(child->Bounds));
		int pointsCount = (int)std::distance(firstPoint, pointOfSecondGroup);
		child->AddPoints(childPointsStartIndex, pointsCount, Points);
		childPointsStartIndex += pointsCount;
		createdNodes.push_back(child);
	}

	PointsCount = pointsUnderLimit;
}

int Node::CreateBlock(BlockNode* nodeBlocks, Block* pointBlocks, int & nodeBlocksCount, int & pointBlocksCount,
									std::vector<std::tuple<Node*, BlockNode*, int>> & blocksToCreate, int currentLevel) {
	BlockNode* newBlock = new(nodeBlocks + nodeBlocksCount) BlockNode();
	int nodeIndex = nodeBlocksCount;
	nodeBlocksCount++;
	const int pointCount = PointsCount;
	const int childCount = ChildCount;
	newBlock->ChildCount = childCount;
	newBlock->PointsCount = pointCount;
	newBlock->MaximumRank = MaximumRank;
	newBlock->MinimumRank = MinimumRank;
	newBlock->PointsStartIndex = -1;
	int remainingPoints = pointCount;
	for (int pointBlockIndex = 0; pointBlockIndex < pointCount; pointBlockIndex += SimdSize) {
		BlockPointCoordinates* newCoordinates = new(pointBlocks + pointBlocksCount) BlockPointCoordinates();
		if (newBlock->PointsStartIndex == -1){
			newBlock->PointsStartIndex = pointBlocksCount;
		}
		pointBlocksCount++;
		BlockPointAttributes* newAttributes = new(pointBlocks + pointBlocksCount) BlockPointAttributes();
		pointBlocksCount++;
		const int pointsMaxCount = pointBlockIndex + std::min(remainingPoints, SimdSize);
		const float floatMaximum = std::numeric_limits<float>().max();
		std::fill_n(newCoordinates->X, SimdSize, floatMaximum);
		std::fill_n(newCoordinates->Y, SimdSize, floatMaximum);
		for (int pointIndex = pointBlockIndex; pointIndex < pointsMaxCount; pointIndex++) {
			Point oldPoint = (*Points)[PointStartIndex + pointIndex];
			const int relativePointIndex = pointIndex - pointBlockIndex;
			newCoordinates->X[relativePointIndex] = oldPoint.x;
			newCoordinates->Y[relativePointIndex] = oldPoint.y;
			newAttributes->Id[relativePointIndex] = oldPoint.id;
			newAttributes->Rank[relativePointIndex] = oldPoint.rank;
		}
		remainingPoints -= SimdSize;
	}

	for (int childIndex = 0; childIndex < ChildCount; childIndex++){
		newBlock->ChildBounds[childIndex] = Children[childIndex]->Bounds;
		if (currentLevel < NodeGroupLevelLimit){
			newBlock->ChildIndex[childIndex] = Children[childIndex]->CreateBlock(nodeBlocks, pointBlocks, nodeBlocksCount, pointBlocksCount, blocksToCreate, currentLevel + 1);
		}
		else{
			blocksToCreate.push_back(std::tuple <Node*, BlockNode*, int >(Children[childIndex], newBlock, childIndex));
		}
	}
	return nodeIndex;
}


void Node::RemoveEmptyNodes() {

	bool emptyNodeFound = false;
	std::vector<Node*> newNodes;
	for (int nodeIndex = 0; nodeIndex < ChildCount; nodeIndex++) {
		Node* node = Children[nodeIndex];
		if (node->PointsCount == 0 && node->ChildCount == 0) {
			emptyNodeFound = true;
			delete node;
		}
		else {
			newNodes.push_back(node);
		}
	}

	if (emptyNodeFound) {
		ChildCount = (int)newNodes.size();
		for (int nodeIndex = 0; nodeIndex < ChildCount; nodeIndex++) {
			Children[nodeIndex] = newNodes[nodeIndex];
		}
	}

	for (int nodeIndex = 0; nodeIndex < newNodes.size(); nodeIndex++) {
		Children[nodeIndex]->RemoveEmptyNodes();
	}

	if (ChildCount < NodeChildCount) {
		emptyNodeFound = false;
		newNodes.clear();
		for (int nodeIndex = 0; nodeIndex < ChildCount; nodeIndex++) {
			Node* node = Children[nodeIndex];
			if (node->PointsCount == 0 && node->ChildCount == 1) {
				newNodes.push_back(node->Children[0]);
				node->ChildCount = 0;
				emptyNodeFound = true;
				delete node;
			}
			else {
				newNodes.push_back(node);
			}
			if (newNodes.size() >= NodeChildCount) {
				break;
			}
		}
		if (emptyNodeFound) {
			ChildCount = (int)newNodes.size();
			for (int nodeIndex = 0; nodeIndex < newNodes.size(); nodeIndex++) {
				Children[nodeIndex] = newNodes[nodeIndex];
			}
		}
	}
}

Node::~Node() {
	for (int nodeIndex = 0; nodeIndex < ChildCount; nodeIndex++) {
		delete Children[nodeIndex];
	}
}

void Node::CreateChildren(Allocator<Node> & nodeAllocator) {
	for (int i = 0; i < NodeChildCount; i++) {
		Node* newNode = nodeAllocator.GetNext();
		Children[i] = newNode;
		ChildCount++;
	}
	Rect* childBounds = GetBoundingBoxesForChildren();
	for (int childIndex = 0; childIndex < ChildCount; childIndex++) {
		Rect newBounds = childBounds[childIndex];
		Node* child = Children[childIndex];
		child->Bounds = newBounds;
	}
	delete[] childBounds;
}

void Node::OptimizeBounds()
{
	float minX = std::numeric_limits<float>().max();
	float maxX = std::numeric_limits<float>().lowest();
	float minY = std::numeric_limits<float>().max();
	float maxY = std::numeric_limits<float>().lowest();
#pragma loop count min(64)
	for (int pointIndex = PointStartIndex; pointIndex < PointStartIndex + PointsCount; pointIndex++) {
		Point point = (*Points)[pointIndex];
		minX = std::min(minX, point.x);
		maxX = std::max(maxX, point.x);
		minY = std::min(minY, point.y);
		maxY = std::max(maxY, point.y);
	}
	Bounds.lx = minX;
	Bounds.hx = maxX;
	Bounds.ly = minY;
	Bounds.hy = maxY;
}



Rect* Node::GetBoundingBoxesForChildren() const {
	std::queue<Rect> boundsToHalf;
	boundsToHalf.push(Bounds);
	while (boundsToHalf.size() < ChildCount) {
		Rect nextBounds = boundsToHalf.front();
		boundsToHalf.pop();
		Rect firstHalf;
		Rect secondHalf;
		bool isHorizontalLevel = (nextBounds.hx - nextBounds.lx) > (nextBounds.hy - nextBounds.ly);
		if (isHorizontalLevel) {
			const float halfX = (nextBounds.lx + nextBounds.hx) / 2.0f;
			firstHalf.lx = halfX;
			firstHalf.hx = nextBounds.hx;
			firstHalf.ly = nextBounds.ly;
			firstHalf.hy = nextBounds.hy;
			secondHalf.lx = nextBounds.lx;
			secondHalf.hx = halfX;
			secondHalf.ly = nextBounds.ly;
			secondHalf.hy = nextBounds.hy;
		}
		else {
			const float halfY = (nextBounds.ly + nextBounds.hy) / 2.0f;
			firstHalf.lx = nextBounds.lx;
			firstHalf.hx = nextBounds.hx;
			firstHalf.ly = halfY;
			firstHalf.hy = nextBounds.hy;
			secondHalf.lx = nextBounds.lx;
			secondHalf.hx = nextBounds.hx;
			secondHalf.ly = nextBounds.ly;
			secondHalf.hy = halfY;
		}
		boundsToHalf.push(firstHalf);
		boundsToHalf.push(secondHalf);
	}
	Rect* childBounds = new Rect[ChildCount];
	for (int childIndex = 0; childIndex < ChildCount; childIndex++) {
		childBounds[childIndex] = boundsToHalf.front();
		boundsToHalf.pop();
	}
	return childBounds;
}