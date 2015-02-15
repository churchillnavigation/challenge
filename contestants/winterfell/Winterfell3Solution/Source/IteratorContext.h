#pragma once

#include "DebugStats.h"

struct Rect;
class MemoryPool;

struct IteratorContext
{
	SearchIterator*	mIteratorMemPoolCursor;

	const Rect&		mRect;
	int				mLastRankSelected;

	IteratorContext(const Rect& inRect, SearchIterator* inMemPool)
		: mIteratorMemPoolCursor(inMemPool)
		, mRect(inRect)
		, mLastRankSelected(-1)
	{
	}

	template< int TAxis >
	SearchIterator*	AllocIterator(KdTree<TAxis>* inTree)
	{
		SearchIterator* out = mIteratorMemPoolCursor;
		++mIteratorMemPoolCursor;
		INC_ITER_COUNT;
		return new (out) SearchIterator(inTree);
	}
};
