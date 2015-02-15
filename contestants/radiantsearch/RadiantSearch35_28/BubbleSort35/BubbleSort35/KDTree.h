#pragma once

#include "PointSorter.h"

class KDTreeSearch : public virtual PointSorter
{
public:
	
	const static bool DoXDivisionFirst = true;
	const static int  MinimumSizeForSplit = 6;  // TODO: This number could be higher?
	
	// TODO: KDTree node completely redundant? Values can be inferred as we traverse the tree, just as they were calculated when we built it
	struct TreeNode
	{
		// TODO: make the tree nodes smaller lower down, once the ints don't need to be 32bit?
		// TODO: Left child is always the next one in memory

		// The others are indices into the array of TreeNodes rather than 64 bit pointers

		int32		highChild;
		int32		divisionPos;		// The world space position that it is divided along the X axis
		RankType	lowestRankChild;	// The lowest rank from here down - if you've already found enough with lower ranks then stop looking
//		int	divisionAxis : 1;	// 0 for x axis division, else 1
		// TODO: Divide alternately between X and Y as we go down the tree, calling specific functions for each
//		int LIsRank : 1;		// Flag to say LChild is an actual data point (A leaf) rather than a tree node
//		int RIsRank : 1;		// Flag to say RChild is an actual data point (A leaf) rather than a tree node
	};

	int MakeTreeRecurse(TreeNode *thisNode, const int thisNodeIndex, DataPoint * const data, int firstPoint, int noofPoints, const bool doXAxisDivision)
	{
		int nextNodeIndex = thisNodeIndex + 1;
		TreeNode *nextNode = thisNode + 1;

		// Fill in this node
		thisNode->highChild = -1;
		thisNode->divisionPos = 0;
		thisNode->lowestRankChild = INT_MAX;	// Updated after tree is made

		if (noofPoints >= MinimumSizeForSplit)
		{
			int noofLeftNodePoints = (noofPoints + 1) / 2;
			DataPoint *firstLowPoint = data + firstPoint;
			DataPoint *firstHighPoint = data + firstPoint + noofLeftNodePoints;

			// Sort this node's points according to x or y position
			if (doXAxisDivision)
			{
				std::nth_element(firstLowPoint, firstHighPoint, firstLowPoint + noofPoints, [](const DataPoint & a, const DataPoint & b) -> bool
											{ 
												return a.x < b.x; 
											}); 
			}
			else
			{
				std::nth_element(firstLowPoint, firstHighPoint, firstLowPoint + noofPoints, [](const DataPoint & a, const DataPoint & b) -> bool
											{ 
												return a.y < b.y; 
											}); 
			}

			// Left child is next node
			TreeNode *lowNode = nextNode;
			int lowChildIndex = thisNodeIndex + 1;
			thisNode->divisionPos = doXAxisDivision ? firstHighPoint->x : firstHighPoint->y;
			nextNodeIndex = MakeTreeRecurse(lowNode, lowChildIndex, data, firstPoint, noofLeftNodePoints, !doXAxisDivision);
			
			// Right child goes after all low child's children
			TreeNode *highNode = thisNode + nextNodeIndex - thisNodeIndex;
			thisNode->highChild = nextNodeIndex;
			int noofRightNodePoints = noofPoints - noofLeftNodePoints;
			nextNodeIndex = MakeTreeRecurse(highNode, thisNode->highChild, data, (int)(firstHighPoint - data), noofRightNodePoints, !doXAxisDivision);
		}

		return nextNodeIndex;
	}

	void MakeTree()
	{
		// TODO: work out how many needed
		m_treeNodes = new TreeNode[(m_noofPoints * 2) / (MinimumSizeForSplit / 2)];	// Each leaf node has at least MinimumSizeForSplit / 2 children, and double the number to account for each pair of leaf nodes having a parent and so on 
		m_treeSortedRanks = new RankType[m_noofPoints];

		// This changes the order of the array, and then we sort it back at the end
		DataPoint *dataNonConst = const_cast<DataPoint *>(m_dataPoints);
		MakeTreeRecurse(m_treeNodes, 0, dataNonConst, 0, m_noofPoints, DoXDivisionFirst);

		// Now convert the tree-sorted data into a tree-sorted array of just the ranks, which can be used to look up into m_dataPoints
		for (int i=0; i<m_noofPoints; ++i)
		{
			m_treeSortedRanks[i] = dataNonConst[i].rank;
		}
		
		// Sort the data by rank again
		std::sort(dataNonConst, dataNonConst + m_noofPoints);

		// Now work out what the lowest ranked point below any node is
		CalculateLowestRankChildValues(m_treeNodes, 0, 0, m_noofPoints, INT_MAX);

		// TODO: Kick off a thread that will keep the cache warm, by continually traversing the first 6-10 layers of the tree.
		// Or at least do so here to set up the cache
	}

	void InsertRankIntoTopN(RankType *topNList, int N, int numFoundSoFar, const RankType rankToInsert)
	{
		// Find location in list
		int listSize = (numFoundSoFar >= N) ? N : numFoundSoFar;
		RankType * rankIter = topNList;
		for (int rankID = 0; rankID < listSize; ++rankID, ++rankIter)
		{
			if (rankToInsert < *rankIter)
			{
				// Insert it here, and shuffle the rest down
				if (numFoundSoFar < N)
				{
					for (int higherID = listSize - 1; higherID >= rankID; --higherID)
					{
						topNList[higherID + 1] = topNList[higherID];
					}
				}
				else
				{
					for (int higherID = N - 2; higherID >= rankID; --higherID)
					{
						topNList[higherID + 1] = topNList[higherID];
					}
				}
				*rankIter = rankToInsert;
				return;
			}
		}
		
		if (numFoundSoFar < N)
		{
			*rankIter = rankToInsert;
		}
	}

	void InsertRanksIntoTopN(RankType *topNList, int N, int numFoundSoFar, const RankType * ranksToInsert, int noofRanksToInsert)
	{
		const RankType * rankIter = ranksToInsert;
		for (int rankID = 0; rankID < noofRanksToInsert; ++rankID, ++rankIter)
		{
			InsertRankIntoTopN(topNList, N, numFoundSoFar, *rankIter);
			numFoundSoFar++;
		}
	}


	// Do the recursive search, appending potentially suitable ranks to the output array
	// This will write out more than desiredCount, and then they will be sorted to get the top ranks
	struct SearchParams
	{
		const RectType * testBounds;
		const TreeNode * treeNodes;
		RankType * ranksOut;
		int noofPoints;
		int32_t desiredCount;
	};
	int32_t SearchRecurse(const SearchParams &params, const TreeNode *thisNode, const RectType thisNodeBounds, int firstPoint, int noofChildPoints, const bool doXAxisDivision, int32_t foundCount)
	{
		// If this node has no children high enough to make it onto our top N, then don't bother
		if (foundCount >= params.desiredCount)
		{
			RankType bottomRankOnList = params.ranksOut[params.desiredCount - 1];
			if (thisNode->lowestRankChild >= bottomRankOnList)
			{
				return foundCount;
			}
		}

		// If this node is completely within the test region, just add all its points
		if (params.testBounds->Contains(thisNodeBounds))
		{
			InsertRanksIntoTopN(params.ranksOut, params.desiredCount, foundCount, &m_treeSortedRanks[firstPoint], noofChildPoints);
			return foundCount + noofChildPoints;
		}
		
		if (noofChildPoints < MinimumSizeForSplit)
		{
			// Just test all child data points rather than going to the child tree nodes
			const RankType * treeRankIter = &m_treeSortedRanks[firstPoint];
			for (int child = 0; child < noofChildPoints; ++child, ++treeRankIter)
			{
				if (IsInRect(*params.testBounds, m_dataPoints[*treeRankIter]))
				{
					InsertRankIntoTopN(params.ranksOut, params.desiredCount, foundCount, *treeRankIter);
					foundCount++;
				}
			}
			return foundCount;
		}
			
		int noofLowNodePoints = (noofChildPoints + 1) / 2;
		int noofHighNodePoints = noofChildPoints - noofLowNodePoints;

		// See whether the low and high children are completely within/outside the test bounds, or partially overlapping
		// We can assume that params.testBounds overlaps thisNodeBounds otherwise we wouldn't have got here
		if (doXAxisDivision)
		{
#ifdef USE_INT_POSITIONS
			// TODO: Formalise the difference in the greater/equal tests when using ints
			// TODO: Can we subtract 1 from the bounds at the split point when going down the tree?
			if (thisNode->divisionPos > params.testBounds->m_lx)
#else
			if (thisNode->divisionPos >= params.testBounds->m_lx)
#endif
			{
				// lower child is not completely outside the bounds
				RectType lowBounds = thisNodeBounds;
				lowBounds.m_hx = thisNode->divisionPos;
				foundCount = SearchRecurse(params, thisNode + 1, lowBounds, firstPoint, noofLowNodePoints, !doXAxisDivision, foundCount);
			}
			if (thisNode->divisionPos <= params.testBounds->m_hx)
			{
				// higher child is not completely outside the bounds
				RectType highBounds = thisNodeBounds;
				highBounds.m_lx = thisNode->divisionPos;
				foundCount = SearchRecurse(params, params.treeNodes + thisNode->highChild, highBounds, firstPoint + noofLowNodePoints, noofHighNodePoints, !doXAxisDivision, foundCount);
			}
		}
		else
		{
			// y axis division
#ifdef USE_INT_POSITIONS
			// TODO: Formalise the difference in the greater/equal tests when using ints
			// TODO: Can we subtract 1 from the bounds at the split point when going down the tree?
			if (thisNode->divisionPos > params.testBounds->m_ly)
#else
			if (thisNode->divisionPos >= params.testBounds->m_ly)
#endif
			{
				// lower child is not completely outside the bounds
				RectType lowBounds = thisNodeBounds;
				lowBounds.m_hy = thisNode->divisionPos;
				foundCount = SearchRecurse(params, thisNode + 1, lowBounds, firstPoint, noofLowNodePoints, !doXAxisDivision, foundCount);
			}
			if (thisNode->divisionPos <= params.testBounds->m_hy)
			{
				// higher child is not completely outside the bounds
				RectType highBounds = thisNodeBounds;
				highBounds.m_ly = thisNode->divisionPos;
				foundCount = SearchRecurse(params, params.treeNodes + thisNode->highChild, highBounds, firstPoint + noofLowNodePoints, noofHighNodePoints, !doXAxisDivision, foundCount);
			}
		}

		return foundCount;
	}

	int32_t DoSearch(const RectType & rect, const int32_t count, Point* pointsOut)
	{
		SearchParams searchParams;
		searchParams.testBounds = &rect;
		searchParams.treeNodes = m_treeNodes;
		searchParams.ranksOut = m_resultBuffer;
		searchParams.noofPoints = m_noofPoints;
		searchParams.desiredCount = count;
		// First node covers whole space
		RectType treeNodeRect;
#ifdef USE_INT_POSITIONS
		treeNodeRect.m_lx = 0;
		treeNodeRect.m_hx = m_noofPoints;
		treeNodeRect.m_ly = 0;
		treeNodeRect.m_hy = m_noofPoints;
#else
		treeNodeRect.m_lx = -FLT_MAX;
		treeNodeRect.m_hx = FLT_MAX;
		treeNodeRect.m_ly = -FLT_MAX;
		treeNodeRect.m_hy = FLT_MAX;
#endif

		int noofFound = SearchRecurse(searchParams, m_treeNodes, treeNodeRect, 0, m_noofPoints, DoXDivisionFirst, 0);

		// Sort the found points by rank
		// TODO: Use a faster sort knowing that we only want the top N - no need to sort the rest
//		std::sort(m_resultBuffer, m_resultBuffer + noofFound);

		int noofResults = (noofFound < count) ? noofFound : count;
		ProduceOutputData(noofResults, m_resultBuffer, pointsOut);

		return noofResults;
	}

	virtual int32_t Search(const Rect & rectIn, const int32_t count, Point* pointsOut) override
	{
		bool outOfBounds = false;
		RectType rect = ConvertRect(rectIn, outOfBounds, m_xCoordsSorted, m_yCoordsSorted, m_noofPoints);
		if (outOfBounds)
		{
			return 0;
		}

		return DoSearch(rect, count, pointsOut);
	}

	virtual void Create(const Point* points_begin, const Point* points_end) override
	{
		PointSorter::Create(points_begin, points_end);

		MakeTree();
	}
	
	int32_t CalculateLowestRankChildValues(TreeNode *thisNode, const int thisNodeIndex, int firstPoint, int noofChildPoints, RankType lowestFoundSoFar)
	{
		if (noofChildPoints < MinimumSizeForSplit)
		{
			const RankType *childRankIter = &m_treeSortedRanks[firstPoint];
			for (int child = 0; child < noofChildPoints; ++child, ++childRankIter)
			{
				RankType childRank = *childRankIter;
				if (childRank < lowestFoundSoFar) 
					lowestFoundSoFar = childRank;
			}

			thisNode->lowestRankChild = lowestFoundSoFar;

			return lowestFoundSoFar;
		}
			
		int noofLowNodePoints = (noofChildPoints + 1) / 2;
		int noofHighNodePoints = noofChildPoints - noofLowNodePoints;

		int lowChildIndex = thisNodeIndex + 1;
		lowestFoundSoFar = CalculateLowestRankChildValues(thisNode + 1, lowChildIndex, firstPoint, noofLowNodePoints, lowestFoundSoFar);
		lowestFoundSoFar = CalculateLowestRankChildValues(thisNode + thisNode->highChild - thisNodeIndex, thisNode->highChild, firstPoint + noofLowNodePoints, noofHighNodePoints, lowestFoundSoFar);
		
		thisNode->lowestRankChild = lowestFoundSoFar;

		return lowestFoundSoFar;
	}

	TreeNode * m_treeNodes;
	RankType * m_treeSortedRanks;	// The data point ranks sorted according to 
};