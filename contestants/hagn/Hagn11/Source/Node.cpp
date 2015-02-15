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
		MaximumRank = (*Points)[PointStartIndex + pointsUnderLimit -1].rank;
		std::reverse(Points->begin() + PointStartIndex, Points->begin() + PointStartIndex + pointsUnderLimit);
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

BlockNode* Node::CreateBlock(std::vector<Block> & blocks, std::vector<std::pair<const Node*, BlockNode**>> & blocksToCreate) const {
	blocks.push_back(Block());
	BlockNode* newBlock = &(blocks.back().Node);
	const int pointCount = PointsCount;
	const int childCount = ChildCount;
	newBlock->ChildCount = childCount;
	newBlock->PointsCount = pointCount;
	newBlock->MinimumRank = MinimumRank;
	newBlock->MaximumRank = MaximumRank;
	int remainingPoints = pointCount;
	for (int pointBlockIndex = 0; pointBlockIndex < pointCount; pointBlockIndex += SimdSize) {
		blocks.push_back(Block());
		BlockPointCoordinates* newCoordinates = &(blocks.back().Coordinates);
		blocks.push_back(Block());
		BlockPointAttributes* newAttributes = &(blocks.back().Attributes);
		const int pointsMaxCount = pointBlockIndex + std::min(remainingPoints, SimdSize);
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

	for (int childIndex = 0; childIndex < childCount; childIndex++) {
		newBlock->ChildBounds[childIndex] = Children[childIndex]->Bounds;
		blocksToCreate.push_back(std::pair<const Node*, BlockNode**>(Children[childIndex], &newBlock->Children[childIndex]));
	}
	return newBlock;
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
		Rect newBounds = GetBoundsForHalf(i);
		Node* newNode = nodeAllocator.GetNext();
		newNode->Bounds = newBounds;
		Children[i] = newNode;
		ChildCount++;
	}
}

void Node::OptimizeBounds()
{
	float minX = std::numeric_limits<float>().max();
	float maxX = std::numeric_limits<float>().lowest();
	float minY = std::numeric_limits<float>().max();
	float maxY = std::numeric_limits<float>().lowest();

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



Rect Node::GetBoundsForHalf(int index) const {
	Rect bounds;
	bool isHorizontalLevel = (Bounds.hx - Bounds.lx) > (Bounds.hy - Bounds.ly);
	if (isHorizontalLevel) {
		if (index == 0) {
			bounds.lx = (Bounds.lx + Bounds.hx) / 2.0f;
			bounds.hx = Bounds.hx;
			bounds.ly = Bounds.ly;
			bounds.hy = Bounds.hy;
		}
		else {
			bounds.lx = Bounds.lx;
			bounds.hx = (Bounds.lx + Bounds.hx) / 2.0f;
			bounds.ly = Bounds.ly;
			bounds.hy = Bounds.hy;
		}
	}
	else {
		if (index == 0) {
			bounds.lx = Bounds.lx;
			bounds.hx = Bounds.hx;
			bounds.ly = (Bounds.ly + Bounds.hy) / 2.0f;
			bounds.hy = Bounds.hy;
		}
		else {
			bounds.lx = Bounds.lx;
			bounds.hx = Bounds.hx;
			bounds.ly = Bounds.ly;
			bounds.hy = (Bounds.ly + Bounds.hy) / 2.0f;
		}
	}
	return bounds;
}