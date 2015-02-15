#include "stdafx.h"

#include <stdint.h>
#include <immintrin.h>
#include <vector>
#include <algorithm>
#include <limits>
#include <ppl.h>
#include <queue>
#include <random>

#include "Point.h"
#include "Rect.h"
#include "Definitions.h"
#include "InputData.h"
#include "Allocator.h"
#include "Block.h"
#include "Node.h"
#include "Helpers.h"


struct SearchContext {
	BlockNode* NodeBlocks;
	Block* PointBlocks;
};


const int NodeBufferSize = 4096;
const int PointBufferSize = 1024;

const BlockNode** nodesToSearch = (const BlockNode**)_aligned_malloc(NodeBufferSize * sizeof(BlockNode*), 64);
int nodesToSearchCount = 0;
const BlockNode** nodesToSearchWithHigherRank = (const BlockNode**)_aligned_malloc(NodeBufferSize * sizeof(BlockNode*), 64);
int nodesToSearchWithHigherRankCount = 0;
const BlockNode** nodesToSearchWithHigherRankQueue = (const BlockNode**)_aligned_malloc(NodeBufferSize * sizeof(BlockNode*), 64);
int nodesToSearchWithHigherRankQueueCount = 0;

int* nodesToSearchWithHigherRankMaximumRank = (int*)_aligned_malloc(NodeBufferSize * sizeof(int), 64);
int* nodesToSearchWithHigherRankQueueMaximumRank = (int*)_aligned_malloc(NodeBufferSize * sizeof(int), 64);

Point* possibleFoundPoints = (Point*)_aligned_malloc(PointBufferSize * sizeof(Point), 64);
int possibleFoundPointsCount = 0;
Point* possibleFoundPointsTooHigh = (Point*)_aligned_malloc(PointBufferSize * sizeof(Point), 64);
int possibleFoundPointsTooHighCount = 0;


int maximumRankForPoints = 0;


inline bool PointOverRankLimit(const Point & point){
	return point.rank > maximumRankForPoints;
}

const __m256 zero = _mm256_setzero_ps();
__m256 boundsLx;
__m256 boundsHx;
__m256 boundsLy;
__m256 boundsHy;

inline void FindPointsInNode(const BlockNode* node, const Block* & pointNodes){
	const int pointsSize = node->PointsCount;
	bool anyFound = false;
	int pointBlockIndex = 0;
	const BlockPointCoordinates* pointCoordinates = (BlockPointCoordinates*)pointNodes + node->PointsStartIndex;
	const BlockPointAttributes* pointAttributes = (BlockPointAttributes*)pointNodes + node->PointsStartIndex + 1;
	for (int pointStartIndex = 0; pointStartIndex < pointsSize; pointStartIndex += SimdSize) {
		const __m256 x = _mm256_load_ps(pointCoordinates->X);
		const __m256 minX = _mm256_sub_ps(x, boundsLx);
		const __m256 maxX = _mm256_sub_ps(x, boundsHx);
		const __m256 multiX = _mm256_mul_ps(minX, maxX);
		const __m256 greaterOrEqualToZeroX = _mm256_cmp_ps(zero, multiX, _CMP_NLT_UQ);
		const int resultMaskX = _mm256_movemask_ps(greaterOrEqualToZeroX);
		if (resultMaskX != 0){
			anyFound = true;
			const __m256 y = _mm256_load_ps(pointCoordinates->Y);
			const __m256 minY = _mm256_sub_ps(y, boundsLy);
			const __m256 maxY = _mm256_sub_ps(y, boundsHy);
			const __m256 multiY = _mm256_mul_ps(minY, maxY);
			const __m256 greaterOrEqualToZeroY = _mm256_cmp_ps(zero, multiY, _CMP_NLT_UQ);
			const int resultMaskY = _mm256_movemask_ps(greaterOrEqualToZeroY);
			const int resultMask = resultMaskX & resultMaskY;
			if (resultMask != 0) {
				__declspec(align(32)) float xCoordinates[SimdSize];
				__declspec(align(32)) float yCoordinates[SimdSize];
				_mm256_store_ps(xCoordinates, x);
				_mm256_store_ps(yCoordinates, y);
				for (int pointIndex = 0; pointIndex < SimdSize; pointIndex++) {
					if ((resultMask & (1 << pointIndex)) != 0) {
						int pointRank = pointAttributes->Rank[pointIndex];
						Point* point;
						if (pointRank > maximumRankForPoints) {
							point = &possibleFoundPointsTooHigh[possibleFoundPointsTooHighCount];
							possibleFoundPointsTooHighCount++;
						}
						else {
							point = &possibleFoundPoints[possibleFoundPointsCount];
							possibleFoundPointsCount++;
						}
						point->rank = pointRank;
						point->id = pointAttributes->Id[pointIndex];
						point->x = xCoordinates[pointIndex];
						point->y = yCoordinates[pointIndex];
					}
				}
			}
		}
		else if (anyFound){
			return;
		}
		pointCoordinates += 2;
		pointAttributes += 2;
	}
}

inline void FindNodesToCheck(const Rect & searchBounds, const BlockNode* node, const BlockNode* nodeBlocks, const Block* pointBlocks) {
	const int nodeChildCount = node->ChildCount;
	for (int childIndex = 0; childIndex < NodeChildCount; childIndex++){
		if (searchBounds.Collides(node->ChildBounds[childIndex])) {
			const BlockNode* child = nodeBlocks + node->ChildIndex[childIndex];
			_mm_prefetch((char*)pointBlocks + child->PointsStartIndex, _MM_HINT_T0);
			if (child->MinimumRank <= maximumRankForPoints) {
				nodesToSearch[nodesToSearchCount] = child;
				nodesToSearchCount++;
				FindNodesToCheck(searchBounds, child, nodeBlocks, pointBlocks);
			}
			else {
				nodesToSearchWithHigherRankQueue[nodesToSearchWithHigherRankQueueCount] = child;
				nodesToSearchWithHigherRankQueueMaximumRank[nodesToSearchWithHigherRankQueueCount] = child->MaximumRank;
				nodesToSearchWithHigherRankQueueCount++;
			}
		}
	}
}


inline void MovePossibleFoundPoints(){
	auto secondPartition = std::partition(possibleFoundPointsTooHigh, possibleFoundPointsTooHigh + possibleFoundPointsTooHighCount, PointOverRankLimit);
	int newCount = std::distance(possibleFoundPointsTooHigh, secondPartition);
	std::copy(secondPartition, possibleFoundPointsTooHigh + possibleFoundPointsTooHighCount, possibleFoundPoints + possibleFoundPointsCount);
	possibleFoundPointsCount += (possibleFoundPointsTooHighCount - newCount);
	possibleFoundPointsTooHighCount = newCount;
}


extern "C" __declspec(dllexport) int32_t search(SearchContext* sc, const DataRect rect, const int32_t count, DataPoint* out_points) {
	const BlockNode* nodeBlocks = sc->NodeBlocks;
	const Block* pointBlocks = sc->PointBlocks;
	nodesToSearchWithHigherRank[nodesToSearchWithHigherRankCount] = nodeBlocks;
	nodesToSearchWithHigherRankMaximumRank[nodesToSearchWithHigherRankCount] = nodeBlocks->MaximumRank;
	nodesToSearchWithHigherRankCount++;
	int foundPointsCount = 0;
	boundsLx = _mm256_broadcast_ss(&rect.lx);
	boundsHx = _mm256_broadcast_ss(&rect.hx);
	boundsLy = _mm256_broadcast_ss(&rect.ly);
	boundsHy = _mm256_broadcast_ss(&rect.hy);

	const Rect searchBounds = rect.toRect();
	while (foundPointsCount < count && nodesToSearchWithHigherRankCount != 0) {
		maximumRankForPoints = *std::min_element(nodesToSearchWithHigherRankMaximumRank, nodesToSearchWithHigherRankMaximumRank + nodesToSearchWithHigherRankCount);
		MovePossibleFoundPoints();

		for (int nodeIndex = 0; nodeIndex < nodesToSearchWithHigherRankCount; nodeIndex++) {
			const BlockNode* node = nodesToSearchWithHigherRank[nodeIndex];
			if (node->MinimumRank <= maximumRankForPoints) {
				nodesToSearch[nodesToSearchCount] = node;
				nodesToSearchCount++;
				FindNodesToCheck(searchBounds, node, nodeBlocks, pointBlocks);
			}
			else {
				nodesToSearchWithHigherRankQueue[nodesToSearchWithHigherRankQueueCount] = node;
				nodesToSearchWithHigherRankQueueMaximumRank[nodesToSearchWithHigherRankQueueCount] = nodesToSearchWithHigherRankMaximumRank[nodeIndex];
				nodesToSearchWithHigherRankQueueCount++;
			}
		}

		nodesToSearchWithHigherRankCount = nodesToSearchWithHigherRankQueueCount;
		nodesToSearchWithHigherRankQueueCount = 0;
		std::swap(nodesToSearchWithHigherRank, nodesToSearchWithHigherRankQueue);
		std::swap(nodesToSearchWithHigherRankMaximumRank, nodesToSearchWithHigherRankQueueMaximumRank);


		for (int nodeIndex = 0; nodeIndex < nodesToSearchCount; nodeIndex++) {
			const BlockNode* node = nodesToSearch[nodeIndex];
			FindPointsInNode(node, pointBlocks);
		}
		nodesToSearchCount = 0;

		std::sort(possibleFoundPoints, possibleFoundPoints + possibleFoundPointsCount, PointRankCompareInstance);
		while (possibleFoundPointsCount != 0) {
			Point* pointToCopy = possibleFoundPoints + possibleFoundPointsCount - 1;
			DataPoint* outPoint = out_points + foundPointsCount;
			outPoint->id = pointToCopy->id;
			outPoint->rank = pointToCopy->rank;
			outPoint->x = pointToCopy->x;
			outPoint->y = pointToCopy->y;
			possibleFoundPointsCount--;
			foundPointsCount++;
			if (foundPointsCount >= count) {
				break;
			}
		}
	}

	std::copy(possibleFoundPointsTooHigh, possibleFoundPointsTooHigh + possibleFoundPointsTooHighCount, possibleFoundPoints + possibleFoundPointsCount);
	possibleFoundPointsCount += possibleFoundPointsTooHighCount;

	std::sort(possibleFoundPoints, possibleFoundPoints + possibleFoundPointsCount, PointRankCompareInstance);
	while (foundPointsCount < count && possibleFoundPointsCount != 0) {
		Point* pointToCopy = possibleFoundPoints + possibleFoundPointsCount - 1;
		DataPoint* outPoint = out_points + foundPointsCount;
		outPoint->id = pointToCopy->id;
		outPoint->rank = pointToCopy->rank;
		outPoint->x = pointToCopy->x;
		outPoint->y = pointToCopy->y;
		possibleFoundPointsCount--;
		foundPointsCount++;
		if (foundPointsCount >= count) {
			break;
		}
	}

	possibleFoundPointsCount = 0;
	possibleFoundPointsTooHighCount = 0;
	nodesToSearchWithHigherRankCount = 0;
	nodesToSearchCount = 0;

	return std::min((int32_t)foundPointsCount, count);
}

inline void WarmupSearch(SearchContext* context, Rect & bounds){
	std::rand();
	std::default_random_engine generator;
	generator.seed(12345);
	std::uniform_real_distribution<float> distributionX(bounds.lx, bounds.hx);
	std::uniform_real_distribution<float> distributionY(bounds.ly, bounds.hy);
	const int testPointsCount = 20;
	DataPoint testPoints[testPointsCount];
	for (int testCase = 0; testCase < 1000000; testCase++){
		float x = distributionX(generator);
		float x2 = distributionX(generator);
		float y = distributionY(generator);
		float y2 = distributionY(generator);
		DataRect rect;
		rect.lx = std::min(x, x2);
		rect.hx = std::max(x, x2);
		rect.ly = std::min(y, y2);
		rect.hy = std::max(y, y2);
		search(context, rect, testPointsCount, testPoints);
	}
}

extern "C" __declspec(dllexport) SearchContext* create(const DataPoint* points_begin, const DataPoint* points_end) {
	int pointsCount = (int)std::distance(points_begin, points_end);

	std::vector<Point> points;
	points.reserve(pointsCount);
	float minX = std::numeric_limits<float>::max();
	float maxX = std::numeric_limits<float>::lowest();
	float minY = std::numeric_limits<float>::max();
	float maxY = std::numeric_limits<float>::lowest();
	DataPoint* currentPoint = (DataPoint*)points_begin;
	while (currentPoint != points_end) {
		points.push_back(currentPoint->toPoint());
		maxX = std::max(maxX, currentPoint->x);
		minX = std::min(minX, currentPoint->x);
		maxY = std::max(maxY, currentPoint->y);
		minY = std::min(minY, currentPoint->y);
		currentPoint++;
	}

	Rect bounds;
	bounds.lx = minX;
	bounds.hx = maxX;
	bounds.ly = minY;
	bounds.hy = maxY;
	Allocator<Node>* nodeAllocator = new Allocator<Node>();
	Node* root = nodeAllocator->GetNext();
	root->Bounds = bounds;
	concurrency::parallel_sort(points.begin(), points.end());
	root->AddPoints(0, (int)points.size(), &points);

	std::vector<Node*> createdNodes;
	std::vector<Node*> createdNodesQueue;
	createdNodes.reserve((int)std::sqrt(pointsCount));
	createdNodesQueue.reserve((int)std::sqrt(pointsCount));
	createdNodes.push_back(root);

	while (!createdNodes.empty()) {
		for (Node* & node : createdNodes) {
			node->MovePointsToChildren(PointsPerNode, *nodeAllocator, createdNodesQueue);
		}
		createdNodes.clear();
		createdNodes.swap(createdNodesQueue);
	}
	createdNodes.shrink_to_fit();
	createdNodesQueue.shrink_to_fit();
	delete nodeAllocator;
	root->RemoveEmptyNodes();
	int nodeBlocksCount = 0;
	int pointBlocksCount = 0;
	root->CalculateBlockCount(nodeBlocksCount, pointBlocksCount);
	SearchContext* context = new SearchContext();
	context->NodeBlocks = (BlockNode*)_aligned_malloc((nodeBlocksCount + 1) * sizeof(BlockNode), 64);
	context->PointBlocks = (Block*)_aligned_malloc((pointBlocksCount + 1) * sizeof(Block), 64);
	std::vector<std::tuple<Node*, BlockNode*, int>> blocksToCreate;
	std::vector<std::tuple<Node*, BlockNode*, int>> blocksToCreateQueue;
	blocksToCreate.reserve(4096);
	blocksToCreateQueue.reserve(4096);
	blocksToCreate.push_back(std::tuple<Node*, BlockNode*, int>(root, NULL, -1));
	int currentNodeBlocksCount = 0;
	int currentPointBlocksCount = 0;
	while (!blocksToCreate.empty()) {
		for (auto & todoBlock : blocksToCreate) {
			Node * currentNode = std::get<0>(todoBlock);
			int newChildIndex = currentNode->CreateBlock(context->NodeBlocks, context->PointBlocks, currentNodeBlocksCount, currentPointBlocksCount, blocksToCreateQueue, 0);
			BlockNode* parentBlock = std::get<1>(todoBlock);
			if (parentBlock != NULL){
				int parentChildIndex = std::get<2>(todoBlock);
				parentBlock->ChildIndex[parentChildIndex] = newChildIndex;
			}
		}
		blocksToCreate.clear();
		blocksToCreateQueue.swap(blocksToCreate);
	}
	delete root;
	points.clear();
	points.shrink_to_fit();
	WarmupSearch(context, bounds);
	return context;
}

extern "C" __declspec(dllexport) SearchContext* destroy(SearchContext* sc) {
	_aligned_free(sc->NodeBlocks);
	_aligned_free(sc->PointBlocks);
	sc->NodeBlocks = NULL;
	sc->PointBlocks = NULL;
	delete sc;
	return NULL;
}
