#pragma once

#include <vector>
#include <algorithm>

#include "point_search.h"
#include "Switches.h"
#include "Maths.h"

/*
	40 bytes overhead.
	12 bytes per point.
	16*12 = 196 bytes = 3*64 byte data cache lines.
	So bin size should be a multiple of 16, with 4 points 
	taken off for node overhead to fit neatly into data cache.
*/
static const size_t kBinSize = (16*2) - 4;

class MemoryPool;


template< int TAxis >
class _ALIGN_TO(32) KdTree
{
public:
	KdTree()
		: mPointCount(0)
		, mMedian(0.0f)
		, mLowerChild(nullptr)
		, mUpperChild(nullptr)
	{
	}

	void				Add(MemoryPool& inMemPool, const Point& inPoint, int inIndex);

private:
	void				CreateChildren(MemoryPool& inMemPool);

	template <int U>
	friend class SearchIterator;

	typedef KdTree< TAxis ^ 1 > KdTreeChild;
	KdTreeChild*		mLowerChild;				// 8
	KdTreeChild*		mUpperChild;				// 8

	Rect				mBRect;						// 16

	int					mPointCount;				// 4
	float				mMedian;					// 4

	CoordPair			mCoords[kBinSize];			// 8 * kBinSize
	int					mPointIndices[kBinSize];	// 4 * kBinSize
};

#include "KdTree.inl"
