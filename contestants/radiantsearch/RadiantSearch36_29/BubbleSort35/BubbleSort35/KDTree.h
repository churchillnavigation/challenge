#pragma once

#include "PointSorter.h"

//#define TRIM_BOUNDS

class KDTreeSearch : public virtual PointSorter
{
public:

	typedef RankSmallStorageType RankStorageType;
	
	const static bool	DoXDivisionFirst = true;
	const static int	MinimumSizeForSplit = 8;  // TODO: This number could be higher?
	const static int	NoofTrees = 3;  // TODO: This number could be higher?
	const static int	Tree1Fraction = 10000;	// a 10000th of the points go in the first tree (the lowest ranked points)
	const static int	Tree2Fraction = 100;	// a 100th of the points go in the first tree (the lowest ranked points)
	
	// TODO: KDTree node completely redundant? Values can be inferred as we traverse the tree, just as they were calculated when we built it
	struct TreeNode
	{
		// TODO: make the tree nodes smaller lower down, once the ints don't need to be 32bit?
		// TODO: Left child is always the next one in memory

		// The others are indices into the array of TreeNodes rather than 64 bit pointers

		UInt24					highChild;
		UInt24					divisionPos;		// The world space position that it is divided along the X axis
#ifdef TRIM_BOUNDS
		unsigned short			minAdjust;			// Trim the min inwards by this much compared to the min of its parent - in the split axis
		unsigned short			maxAdjust;			// Trim the max inwards by this much compared to the max of its parent - in the split axis
#endif
		RankSmallStorageType	lowestRankChild;	// The lowest rank from here down - if you've already found enough with lower ranks then stop looking

//		int	divisionAxis : 1;	// 0 for x axis division, else 1
		// TODO: Divide alternately between X and Y as we go down the tree, calling specific functions for each
//		int LIsRank : 1;		// Flag to say LChild is an actual data point (A leaf) rather than a tree node
//		int RIsRank : 1;		// Flag to say RChild is an actual data point (A leaf) rather than a tree node

		TreeNode()
		{
#ifdef TRIM_BOUNDS
			minAdjust = 0;
			maxAdjust = 0;
#endif
		}
	};

	struct TreeInfo
	{
		int noofPoints;
		int firstPoint;
		TreeNode *treeNodes;
	};

	int CalculateNumberOfTreeNodesRecurse(const int thisNodeIndex, int noofPoints)
	{
		int nextNodeIndex = thisNodeIndex + 1;

		if (noofPoints >= MinimumSizeForSplit)
		{
			int noofLowNodePoints = (noofPoints + 1) / 2;
			nextNodeIndex = CalculateNumberOfTreeNodesRecurse(nextNodeIndex, noofLowNodePoints);
			int noofHighNodePoints = noofPoints - noofLowNodePoints;
			nextNodeIndex = CalculateNumberOfTreeNodesRecurse(nextNodeIndex, noofHighNodePoints);
		}

		return nextNodeIndex;
	}

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
			int noofLowNodePoints = (noofPoints + 1) / 2;
			DataPoint *firstLowPoint = data + firstPoint;
			DataPoint *firstHighPoint = data + firstPoint + noofLowNodePoints;

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
			nextNodeIndex = MakeTreeRecurse(lowNode, lowChildIndex, data, firstPoint, noofLowNodePoints, !doXAxisDivision);
			
			// Right child goes after all low child's children
			TreeNode *highNode = thisNode + nextNodeIndex - thisNodeIndex;
			thisNode->highChild = nextNodeIndex;
			int noofHighNodePoints = noofPoints - noofLowNodePoints;
			nextNodeIndex = MakeTreeRecurse(highNode, thisNode->highChild, data, (int)(firstHighPoint - data), noofHighNodePoints, !doXAxisDivision);
		}
		else
		{
			// Sort by rank so if the last N are too high we can skip them all
			DataPoint *first = data + firstPoint;
			std::sort(first, first + noofPoints);
		}

		return nextNodeIndex;
	}

	void MakeTree()
	{
		// This changes the order of the array, and then we sort it back at the end
		DataPoint *dataNonConst = const_cast<DataPoint *>(m_dataPoints);
		m_treeSortedRanks = new RankStorageType[m_noofPoints];
		
		m_trees[0].noofPoints = m_noofPoints / Tree1Fraction;
		m_trees[1].noofPoints = m_noofPoints / Tree2Fraction;
		m_trees[2].noofPoints = -1;	// Use the rest

		// Store a small subset of the points with the highest rank in a tree, and then the next bunch in another tree, and so on.
		// For example the top 1000 points in a tree, followed by the next 100000, followed by the rest (9899000)
		int pointsRemaining = m_noofPoints;
		for (int treeIndex = 0; treeIndex < ARRAYSIZE(m_trees); ++treeIndex)
		{
			int noofPointsInThisTree = m_trees[treeIndex].noofPoints;
			if ((noofPointsInThisTree < 0) || (noofPointsInThisTree > pointsRemaining))
			{
				noofPointsInThisTree = pointsRemaining;
				m_trees[treeIndex].noofPoints = noofPointsInThisTree;
			}
			if (noofPointsInThisTree == 0)
			{
				m_trees[treeIndex].treeNodes = nullptr;
				continue;
			}
			int firstPointInThisTree = m_noofPoints - pointsRemaining;
			m_trees[treeIndex].firstPoint = firstPointInThisTree;

			int noofTreeNodesNeeded = CalculateNumberOfTreeNodesRecurse(0, noofPointsInThisTree);
			m_trees[treeIndex].treeNodes = new TreeNode[noofTreeNodesNeeded];

			MakeTreeRecurse(m_trees[treeIndex].treeNodes, 0, dataNonConst, firstPointInThisTree, noofPointsInThisTree, DoXDivisionFirst);

			// Now convert the tree-sorted data into a tree-sorted array of just the ranks, which can be used to look up into m_dataPoints
			for (int i = firstPointInThisTree; i < firstPointInThisTree + noofPointsInThisTree; ++i)
			{
				m_treeSortedRanks[i] = dataNonConst[i].rank;
			}

			// Now work out what the lowest ranked point below any node is
			CalculateLowestRankChildValues(m_trees[treeIndex].treeNodes, 0, firstPointInThisTree, noofPointsInThisTree, INT_MAX);

			pointsRemaining -= noofPointsInThisTree;
		}

		// Sort the data by rank again
		std::sort(dataNonConst, dataNonConst + m_noofPoints);

		// TODO: Kick off a thread that will keep the cache warm, by continually traversing the first 6-10 layers of the tree.
		// Or at least do so here to set up the cache
	}

	void InsertRankIntoTopN(RankType *topNList, int N, int numFoundSoFar, const RankType rankToInsert)
	{
		// Find location in list
		int listSize = (numFoundSoFar >= N) ? N : numFoundSoFar;

		// TODO: Search from bottom up, as generally ranks will not be high up the list or on it at all		
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

	void InsertRanksIntoTopN(RankType *topNList, int N, int numFoundSoFar, const RankStorageType * ranksToInsert, int noofRanksToInsert)
	{
		const RankStorageType * rankIter = ranksToInsert;
		for (int rankID = 0; rankID < noofRanksToInsert; ++rankID, ++rankIter)
		{
			// TODO: have this function return the new highest rank on list, and use it to avoid ones that are too low
			InsertRankIntoTopN(topNList, N, numFoundSoFar, *rankIter);
			numFoundSoFar++;
		}
	}

//#define SHELVE_UNTESTED_RESULTS

	// Do the recursive search, appending potentially suitable ranks to the output array
	// This will write out more than desiredCount, and then they will be sorted to get the top ranks
	struct SearchParams
	{
		const RectType * testBounds;
		const TreeNode * treeNodes;
		RankType * ranksOut;
		int32_t desiredCount;
#ifdef SHELVE_UNTESTED_RESULTS
		RankType * untestedResults;
		int32 noofUntestedResults;
#endif
	};
	int32_t SearchRecurse(SearchParams &params, const TreeNode *thisNode, const RectType thisNodeBounds, int firstPoint, int noofChildPoints, const bool doXAxisDivision, int32_t foundCount)
	{
		// TODO: Pre-fetch left and right children, and the points within this node?
		//_mm_prefetch((char const *)(params.treeNodes + thisNode->highChild), _MM_HINT_NTA);

		RankType * const bottomRankOnList = params.ranksOut + params.desiredCount - 1;

		// If this node has no children high enough to make it onto our top N, then don't bother
		if (foundCount >= params.desiredCount)
		{
			if (thisNode->lowestRankChild >= *bottomRankOnList)
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
			const RankStorageType * treeRankIter = &m_treeSortedRanks[firstPoint];
			for (int child = 0; child < noofChildPoints; ++child, ++treeRankIter)
			{
				RankType childRank = *treeRankIter;
				// TODO: move foundCount to searchParams, and have it set a flag when it's full for quicker testing
				if (foundCount >= params.desiredCount)
				{
					// skip if it wouldn't make it onto the list anyway
					if (childRank >= *bottomRankOnList)
					{
						// TODO: Break here as they are sorted by rank
						continue;
					}
				}
				
#ifdef SHELVE_UNTESTED_RESULTS
				params.untestedResults[params.noofUntestedResults++] = childRank;
#else
				if (IsInRect(*params.testBounds, m_dataPoints[childRank]))
				{
					InsertRankIntoTopN(params.ranksOut, params.desiredCount, foundCount, childRank);
					foundCount++;
				}
#endif
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
		searchParams.ranksOut = m_resultBuffer;
#ifdef SHELVE_UNTESTED_RESULTS
		searchParams.untestedResults = m_untestedResults;
		searchParams.noofUntestedResults = 0;
#endif
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
		
		int noofFound = 0;
		for (int treeIndex = 0; treeIndex < ARRAYSIZE(m_trees); ++treeIndex)
		{
			searchParams.treeNodes = m_trees[treeIndex].treeNodes;
			noofFound = SearchRecurse(searchParams, searchParams.treeNodes, treeNodeRect, m_trees[treeIndex].firstPoint, m_trees[treeIndex].noofPoints, DoXDivisionFirst, noofFound);
			
#ifdef SHELVE_UNTESTED_RESULTS
			if (searchParams.noofUntestedResults)
			{
				std::sort(searchParams.untestedResults, searchParams.untestedResults + searchParams.noofUntestedResults);
				noofFound = MIN(noofFound, count);
				RankType actualResults[20];
				for (int i=0; i<noofFound; i++)
				{
					actualResults[i] = searchParams.ranksOut[i];
				}
				
				// Do a merge sort with the results found so far, and the untested ones that may or may not be in bounds
				RankType *resultIter = searchParams.ranksOut;
				RankType *resultEndIter = searchParams.ranksOut + count;
				const RankType *list1Iter = actualResults;
				const RankType *list1EndIter = actualResults + noofFound;
				const RankType *list2Iter = searchParams.untestedResults;
				const RankType *list2EndIter = searchParams.untestedResults + searchParams.noofUntestedResults;
				while (1)
				{
					if (list1Iter == list1EndIter)
					{
						// First list run out, so just fill with second
						while (list2Iter < list2EndIter)
						{
							RankType rank = *list2Iter++;
							if (IsInRect(rect, m_dataPoints[rank]))
							{
								*resultIter++ = rank;
								noofFound++;
								if (resultIter >= resultEndIter)
								{
									break;
								}
							}
						}
						break;
					}
					if (list2Iter == list2EndIter)
					{
						// Second list run out, so just fill with first
						while (list1Iter < list1EndIter)
						{
							RankType rank = *list1Iter++;
							if (IsInRect(rect, m_dataPoints[rank]))
							{
								*resultIter++ = rank;
								noofFound++;
								if (resultIter >= resultEndIter)
								{
									break;
								}
							}
						}
						break;
					}

					// Both lists still have something, so see which if any should go in
					RankType rank2 = *list2Iter;
					RankType rank1 = *list1Iter;
					if (rank1 < rank2)
					{
						if (IsInRect(rect, m_dataPoints[rank1]))
						{
							*resultIter++ = rank1;
							noofFound++;
							if (resultIter >= resultEndIter)
							{
								break;
							}
						}
						list1Iter++;
					}
					else
					{
						if (IsInRect(rect, m_dataPoints[rank2]))
						{
							*resultIter++ = rank2;
							noofFound++;
							if (resultIter >= resultEndIter)
							{
								break;
							}
						}
						list2Iter++;
					}
				}
				// Processed now, so reset for next tree
				searchParams.noofUntestedResults = 0;
			}
#endif
		}

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
			const RankStorageType *childRankIter = &m_treeSortedRanks[firstPoint];
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

	TreeInfo m_trees[NoofTrees];
	RankStorageType * m_treeSortedRanks;	// The data point ranks sorted according to 
};