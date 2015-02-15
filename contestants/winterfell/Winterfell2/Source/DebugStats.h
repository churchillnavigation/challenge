#pragma once

//#define _ENABLE_STATS_LOGGING

#ifdef _ENABLE_STATS_LOGGING

#include <vector>
#include <algorithm>

struct StatsSession
{
	int mKDTreeCount;
	int mKDTreeCurrentDepth;
	int mKDTreeMaxDepth;
	int mPointInRectHit;
	int mPointInRectMiss;
	int mCullPointsCheckHit;
	int mCullPointsCheckMiss;
	int mCullClusterHit;
	int mCullClusterMiss;
	int mRectFullyInsideRectHit;
	int mRectFullyInsideRectMiss;
	int mClusterRectFullyInsideRectHit;
	int mClusterRectFullyInsideRectMiss;
	int mIteratorCount;
	int mIteratorMaxDepth;
	int mIteratorCurrentDepth;
	int mIteratorAccessCount;

	StatsSession()
	{
		memset(this, 0, sizeof(StatsSession));
	}
};

struct StatsAllQueries
{
	std::vector<StatsSession>	mSessions;
	StatsSession				mCurrent;

	StatsAllQueries();
	void CommitCurrent();
	void ExportLog();
};

extern StatsSession& gGetCurrentStatsSession();

struct PushKDTreeDepth
{
	PushKDTreeDepth()
	{
		StatsSession& sesh = gGetCurrentStatsSession();
		sesh.mKDTreeCurrentDepth++;
		sesh.mKDTreeMaxDepth = std::max(sesh.mKDTreeMaxDepth, sesh.mKDTreeCurrentDepth);
	}

	~PushKDTreeDepth()
	{
		gGetCurrentStatsSession().mKDTreeCurrentDepth--;
	}
};

struct PushIterDepth
{
	PushIterDepth() 
	{ 
		StatsSession& sesh = gGetCurrentStatsSession();
		sesh.mIteratorCurrentDepth++; 
		sesh.mIteratorMaxDepth = std::max(sesh.mIteratorMaxDepth, sesh.mIteratorCurrentDepth);
		sesh.mIteratorAccessCount++;
	}

	~PushIterDepth() 
	{ 
		gGetCurrentStatsSession().mIteratorCurrentDepth--; 
	}
};

#define INC_KDTREE_COUNT				gGetCurrentStatsSession().mKDTreeCount++
#define PUSH_KDTREE_DEPTH				PushKDTreeDepth pushedKDTreeDepth
#define INC_POINTINRECT_HIT				gGetCurrentStatsSession().mPointInRectHit++
#define INC_POINTINRECT_MISS			gGetCurrentStatsSession().mPointInRectMiss++
#define CULL_POINTS_CHECK_HIT			gGetCurrentStatsSession().mCullPointsCheckHit++
#define CULL_POINTS_CHECK_MISS			gGetCurrentStatsSession().mCullPointsCheckMiss++
#define CULL_CLUSTER_HIT				gGetCurrentStatsSession().mCullClusterHit++
#define CULL_CLUSTER_MISS				gGetCurrentStatsSession().mCullClusterMiss++
#define RECT_INSIDE_RECT_HIT			gGetCurrentStatsSession().mRectFullyInsideRectHit++
#define RECT_INSIDE_RECT_MISS			gGetCurrentStatsSession().mRectFullyInsideRectMiss++
#define CLUSTER_RECT_INSIDE_RECT_HIT	gGetCurrentStatsSession().mClusterRectFullyInsideRectHit++
#define CLUSTER_RECT_INSIDE_RECT_MISS	gGetCurrentStatsSession().mClusterRectFullyInsideRectMiss++
#define INC_ITER_COUNT					gGetCurrentStatsSession().mIteratorCount++
#define PUSH_ITER_DEPTH					PushIterDepth pushedIterDepth

#else

#define INC_KDTREE_COUNT	
#define PUSH_KDTREE_DEPTH
#define INC_POINTINRECT_HIT		
#define INC_POINTINRECT_MISS	
#define CULL_POINTS_CHECK_HIT
#define CULL_POINTS_CHECK_MISS
#define CULL_CLUSTER_HIT
#define CULL_CLUSTER_MISS
#define RECT_INSIDE_RECT_HIT
#define RECT_INSIDE_RECT_MISS
#define CLUSTER_RECT_INSIDE_RECT_HIT
#define CLUSTER_RECT_INSIDE_RECT_MISS
#define INC_ITER_COUNT			
#define PUSH_ITER_DEPTH			

#endif
