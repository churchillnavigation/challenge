#pragma once

#include "KdTree.h"
#include "point_search.h"
#include "Switches.h"
#include "MemoryPool.h"

enum ESelectedChild
{
	ENone = 0,
	ELower = 1,
	EUpper = 2,
};

enum EIterState
{
	EStart,
	ECheckingPointsInNode,
	ECheckingBothChildren,
	ECheckingOneChild,
};

template< int TAxis >
class _ALIGN_TO(32) SearchIterator
{
public:
	SearchIterator(KdTree<TAxis>* inTree)
		: mTree(inTree)
		, mLowerChild(nullptr)
		, mUpperChild(nullptr)
		, mSelectedValue(-1)
		, mSelectedChild(ENone)
		, mIndex(0)
		, mState(EStart)
	{
	}

	int		GetNext(const Rect& inRect, MemoryPool& inAllocator);
	void	Advance();

private:
	typedef SearchIterator< TAxis ^ 1 > SearchIterChild;

	KdTree<TAxis>*		mTree;			// 8
	SearchIterChild* 	mLowerChild;	// 8
	SearchIterChild* 	mUpperChild;	// 8
	int					mSelectedValue;	// 4
	uint8_t 			mSelectedChild;	// 1
	uint8_t 			mIndex;			// 1
	uint8_t				mState;			// 1
};

#include "SearchIterator.inl"
