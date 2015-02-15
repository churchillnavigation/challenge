#include "stdafx.h"

#include <stdint.h>
#include <immintrin.h>
#include <vector>
#include <algorithm>
#include <limits>
#include <ppl.h>
#include <queue>

#include "Point.h"
#include "Rect.h"
#include "Definitions.h"
#include "InputData.h"
#include "Allocator.h"
#include "Block.h"
#include "Node.h"
#include "Helpers.h"


struct SearchContext {
	std::vector<Block> Blocks;
	BlockNode* RootNode;
};


inline void findNodesToCheck(const Rect & searchBounds, const int maximumRank, const BlockNode* node, std::vector<BlockNode*> & nodesToSearch, std::vector<BlockNode*> & nodesWithHigherRank) {
	for (int childNodeIndex = 0; childNodeIndex < node->ChildCount; childNodeIndex++) {
		if (searchBounds.Collides(node->ChildBounds[childNodeIndex])) {
			BlockNode* child = node->Children[childNodeIndex];
			if (child->MinimumRank <= maximumRank) {
				nodesToSearch.push_back(child);
				findNodesToCheck(searchBounds, maximumRank, child, nodesToSearch, nodesWithHigherRank);
			}
			else {
				nodesWithHigherRank.push_back(child);
			}
		}
	}
}

_declspec(align(32)) std::vector<BlockNode*> nodesToSearch;
_declspec(align(32)) std::vector<BlockNode*> nodesToSearchWithHigherRank;
_declspec(align(32)) std::vector<BlockNode*> nodesToSearchWithHigherRankQueue;


_declspec(align(32)) std::vector<Point> possibleFoundPoints;
_declspec(align(32)) std::vector<Point> possibleFoundPointsTooHigh;


const __m256 zero = _mm256_setzero_ps();

extern "C" __declspec(dllexport) int32_t search(SearchContext* sc, const DataRect rect, const int32_t count, DataPoint* out_points) {
	nodesToSearchWithHigherRank.push_back(sc->RootNode);
	int foundPointsCount = 0;
	const __m256 boundsLx = _mm256_broadcast_ss(&rect.lx);
	const __m256 boundsHx = _mm256_broadcast_ss(&rect.hx);
	const __m256 boundsLy = _mm256_broadcast_ss(&rect.ly);
	const __m256 boundsHy = _mm256_broadcast_ss(&rect.hy);

	const Rect searchBounds = rect.toRect();
	while (foundPointsCount < count && !nodesToSearchWithHigherRank.empty()) {
		int minimumNodeRank = std::numeric_limits<int>().max();
		int maximumRank = std::numeric_limits<int>().max();
		for (BlockNode* node : nodesToSearchWithHigherRank) {
			if (node->MinimumRank < minimumNodeRank) {
				minimumNodeRank = node->MinimumRank;
				maximumRank = node->MaximumRank;
			}
		}

		for (BlockNode* node : nodesToSearchWithHigherRank) {
			if (node->MinimumRank <= maximumRank) {
				nodesToSearch.push_back(node);
				findNodesToCheck(searchBounds, maximumRank, node, nodesToSearch, nodesToSearchWithHigherRankQueue);
			}
			else {
				nodesToSearchWithHigherRankQueue.push_back(node);
			}
		}

		nodesToSearchWithHigherRank.clear();
		nodesToSearchWithHigherRank.swap(nodesToSearchWithHigherRankQueue);

		int pointSourcesCount = (int)nodesToSearch.size();

		std::sort(possibleFoundPointsTooHigh.begin(), possibleFoundPointsTooHigh.end(), std::greater<Point>());
		while (!possibleFoundPointsTooHigh.empty() && possibleFoundPointsTooHigh.back().rank <= maximumRank) {
			possibleFoundPoints.push_back(possibleFoundPointsTooHigh.back());
			possibleFoundPointsTooHigh.pop_back();
			pointSourcesCount++;
		}

		for (BlockNode* node : nodesToSearch) {
			const int pointsSize = node->PointsCount;
			int pointBlockIndex = 0;
			BlockPointCoordinates* pointCoordinates = (BlockPointCoordinates*)(node + 1);
			BlockPointAttributes* pointAttributes = (BlockPointAttributes*)(node + 2);
			for (int pointStartIndex = 0; pointStartIndex < pointsSize; pointStartIndex += SimdSize) {
				const __m256 pX = _mm256_loadu_ps(&pointCoordinates->X[0]);
				const __m256 pY = _mm256_loadu_ps(&pointCoordinates->Y[0]);
				const __m256 minX = _mm256_sub_ps(pX, boundsLx);
				const __m256 maxX = _mm256_sub_ps(pX, boundsHx);
				const __m256 multiX = _mm256_mul_ps(minX, maxX);
				const __m256 minY = _mm256_sub_ps(pY, boundsLy);
				const __m256 maxY = _mm256_sub_ps(pY, boundsHy);
				const __m256 multiY = _mm256_mul_ps(minY, maxY);
				const __m256 maxMultiValues = _mm256_max_ps(multiX, multiY);
				const __m256 greaterOrEqualToZero = _mm256_cmp_ps(zero, maxMultiValues, _CMP_NLT_UQ);
				const int resultMask = _mm256_movemask_ps(greaterOrEqualToZero);
				if (resultMask != 0) {
					for (int pointIndex = 0; pointIndex < SimdSize; pointIndex++) {
						if ((resultMask & (1 << pointIndex)) != 0) {
							Point point(pointAttributes->Id[pointIndex],
								pointAttributes->Rank[pointIndex],
								pointCoordinates->X[pointIndex],
								pointCoordinates->Y[pointIndex]);
							if (point.rank > maximumRank) {
								possibleFoundPointsTooHigh.push_back(point);
							}
							else {
								possibleFoundPoints.push_back(point);
							}
						}
					}
				}
				pointCoordinates += 2;
				pointAttributes += 2;
			}
		}

		nodesToSearch.clear();

		std::sort(possibleFoundPoints.begin(), possibleFoundPoints.end(), std::greater<Point>());
		while (!possibleFoundPoints.empty()) {
			out_points[foundPointsCount] = DataPoint(possibleFoundPoints.back());
			possibleFoundPoints.pop_back();
			foundPointsCount++;
			if (foundPointsCount >= count) {
				break;
			}
		}
	}


	std::sort(possibleFoundPointsTooHigh.begin(), possibleFoundPointsTooHigh.end(), std::greater<Point>());
	while (!possibleFoundPointsTooHigh.empty() && (possibleFoundPoints.size() + foundPointsCount) < count) {
		possibleFoundPoints.push_back(possibleFoundPointsTooHigh.back());
		possibleFoundPointsTooHigh.pop_back();
	}

	std::sort(possibleFoundPoints.begin(), possibleFoundPoints.end(), std::greater<Point>());
	while (foundPointsCount < count && !possibleFoundPoints.empty()) {
		out_points[foundPointsCount] = DataPoint(possibleFoundPoints.back());
		possibleFoundPoints.pop_back();
		foundPointsCount++;
		if (foundPointsCount >= count) {
			break;
		}
	}

	possibleFoundPoints.clear();
	possibleFoundPointsTooHigh.clear();
	nodesToSearchWithHigherRank.clear();
	nodesToSearch.clear();

	return std::min((int32_t)foundPointsCount, count);
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

	int currentLevel = 0;

	while (!createdNodes.empty()) {
		const int pointsLimit = 32;
		for (Node* & node : createdNodes) {
			node->MovePointsToChildren(pointsLimit, *nodeAllocator, createdNodesQueue);
		}
		createdNodes.clear();
		createdNodes.swap(createdNodesQueue);
		currentLevel++;
	}
	createdNodes.shrink_to_fit();
	createdNodesQueue.shrink_to_fit();
	delete nodeAllocator;
	root->RemoveEmptyNodes();

	const int blocksCount = root->CalculateBlockCount();
	SearchContext* context = new SearchContext();
	context->Blocks.reserve(blocksCount);
	std::vector<std::pair<const Node*, BlockNode**>> blocksToCreate;
	std::vector<std::pair<const Node*, BlockNode**>> blocksToCreateQueue;
	blocksToCreate.reserve(4096);
	blocksToCreateQueue.reserve(4096);
	blocksToCreate.push_back(std::pair<const Node*, BlockNode**>(root, &context->RootNode));
	currentLevel = 0;
	while (!blocksToCreate.empty()) {
		for (auto todoBlock : blocksToCreate) {
			*todoBlock.second = todoBlock.first->CreateBlock(context->Blocks, blocksToCreateQueue);
		}
		blocksToCreate.clear();
		blocksToCreateQueue.swap(blocksToCreate);
		currentLevel++;
	}
	delete root;
	points.clear();
	points.shrink_to_fit();
	possibleFoundPoints.reserve(4096);
	possibleFoundPointsTooHigh.reserve(4096);
	nodesToSearch.reserve(4096);
	nodesToSearchWithHigherRank.reserve(4096);
	nodesToSearchWithHigherRankQueue.reserve(4096);
	return context;
}

extern "C" __declspec(dllexport) SearchContext* destroy(SearchContext* sc) {
	sc->RootNode = NULL;
	delete sc;
	return NULL;
}
