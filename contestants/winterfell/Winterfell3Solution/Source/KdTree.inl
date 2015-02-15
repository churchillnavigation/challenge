
#include <vector>
#include "point_search.h"
#include "MemoryPool.h"
#include "DebugStats.h"

template< int TAxis >
KdTree<TAxis>::KdTree()
	: mPointCount(0)
	, mMedian(0.0f)
	, mLowerChild(nullptr)
	, mUpperChild(nullptr)
{
	gSetInvMaxExtents(mBRect);
}

template< int TAxis >
void KdTree<TAxis>::Fill(MemoryPool& inMemPool, std::vector<CoordPairAndRank>& inPoints, int inDepth)
{
	PUSH_KDTREE_DEPTH;

	if (inDepth >= kBalancingDepth)
	{
		std::sort(inPoints.begin(), inPoints.end(), [] (const CoordPairAndRank& inLHS, const CoordPairAndRank& inRHS)
		{
			return inLHS.mRank < inRHS.mRank;
		});

		for (auto& coord : inPoints)
		{
			Point p;
			p.x = coord.mCoord.x;
			p.y = coord.mCoord.y;
			p.rank = coord.mRank;
			Add(inMemPool, p);
		}

		return;
	}

	std::sort(inPoints.begin(), inPoints.end(), [] (const CoordPairAndRank& inLHS, const CoordPairAndRank& inRHS)
	{
		return inLHS.mRank > inRHS.mRank;
	});

	while ((mPointCount < kBinSize) & (inPoints.size() > 0))
	{
		auto& point = inPoints.back();
		mCoords[mPointCount]		= point.mCoord;
		mPointIndices[mPointCount]	= point.mRank;

		++mPointCount;
		inPoints.pop_back();
	}

	if (inPoints.size() > 0)
	{
		mLowerChild = inMemPool.Alloc<KdTreeChild>();
		mUpperChild = inMemPool.Alloc<KdTreeChild>();
		INC_KDTREE_COUNT;
		INC_KDTREE_COUNT;

		std::sort(inPoints.begin(), inPoints.end(), [] (const CoordPairAndRank& inLHS, const CoordPairAndRank& inRHS)
		{
			return inLHS.mCoord.Get<TAxis>() < inRHS.mCoord.Get<TAxis>();
		});

		int remaining = (int) inPoints.size();
		int offset = (remaining - (1 - (remaining % 2))) / 2;

		auto lowerMidIter = inPoints.begin() + offset;
		mMedian = (*lowerMidIter).mCoord.Get<TAxis>();

		{
			std::vector<CoordPairAndRank> lowerPoints;
			lowerPoints.assign(	inPoints.begin(), lowerMidIter + 1 );
			mLowerChild->Fill(inMemPool, lowerPoints, inDepth+1);
		}

		{
			std::vector<CoordPairAndRank> upperPoints;
			upperPoints.assign( lowerMidIter + 1, inPoints.end() );
			mUpperChild->Fill(inMemPool, upperPoints, inDepth+1);
		}
	}
}


template< int TAxis >
void KdTree<TAxis>::Add(MemoryPool& inMemPool, const Point& inPoint)
{
	PUSH_KDTREE_DEPTH;

	if (mPointCount < kBinSize)
	{
		mCoords[mPointCount]		= CoordPair(inPoint);
		mPointIndices[mPointCount]	= inPoint.rank;
		++mPointCount;
	}
	else
	{
		if (nullptr == mLowerChild) { CreateChildren(inMemPool); }

		auto child = (gGetCoord<TAxis>(inPoint) <= mMedian) ? mLowerChild : mUpperChild;
		child->Add(inMemPool, inPoint);
	}
}

template< int TAxis >
void KdTree<TAxis>::CreateChildren(MemoryPool& inMemPool)
{
	static const size_t kMidIndex = kBinSize / 2;

	std::vector<float> coords(kBinSize);
	for (int i = 0; i < mPointCount; ++i)
	{
		coords[i] = mCoords[i].Get<TAxis>();
	}
	std::sort(coords.begin(), coords.end(), [] (float inLHS, float inRHS) { return inLHS < inRHS; } );

	mMedian = (coords[kMidIndex - 1] + coords[kMidIndex]) * 0.5f;

	mLowerChild = inMemPool.Alloc<KdTreeChild>();
	mUpperChild = inMemPool.Alloc<KdTreeChild>();
	INC_KDTREE_COUNT;
	INC_KDTREE_COUNT;
}

template< int TAxis >
void KdTree<TAxis>::Finalise()
{
	gSetInvMaxExtents(mBRect);
	for (int i = 0; i < mPointCount; ++i)
	{
		gExpandToContain(mBRect, mCoords[i]);
	}

	if (nullptr != mLowerChild)
	{
		mLowerChild->Finalise();
		mUpperChild->Finalise();
	}
}

template< int TAxis >
bool KdTree<TAxis>::DoesRectOverlapLowerChild(const Rect& inRect)
{
	return gGetLower<TAxis>(inRect) <= mMedian;
}

template< int TAxis >
bool KdTree<TAxis>::DoesRectOverlapUpperChild(const Rect& inRect)
{
	return gGetUpper<TAxis>(inRect) >= mMedian;
}

