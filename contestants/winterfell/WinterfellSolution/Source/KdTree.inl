
#include <vector>
#include "point_search.h"
#include "MemoryPool.h"

template< int TAxis >
void KdTree<TAxis>::Add(MemoryPool& inMemPool, const Point& inPoint, int inIndex)
{
	if (mPointCount == 0)
	{
		mBRect.lx = inPoint.x;
		mBRect.hx = inPoint.x;
		mBRect.ly = inPoint.y;
		mBRect.hy = inPoint.y;
	}

	if (mPointCount < kBinSize)
	{
		mCoords[mPointCount]		= CoordPair(inPoint);
		mPointIndices[mPointCount]	= inIndex;

		mBRect.lx = std::min( inPoint.x, mBRect.lx );
		mBRect.hx = std::max( inPoint.x, mBRect.hx );
		mBRect.ly = std::min( inPoint.y, mBRect.ly );
		mBRect.hy = std::max( inPoint.y, mBRect.hy );

		++mPointCount;
	}
	else
	{
		if (nullptr == mLowerChild) { CreateChildren(inMemPool); }

		auto child = (gGetCoord<TAxis>(inPoint) <= mMedian) ? mLowerChild : mUpperChild;
		child->Add(inMemPool, inPoint, inIndex);
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
}
