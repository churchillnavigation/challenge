#pragma once

#include "KdTree.h"
#include "point_search.h"
#include "Switches.h"
#include "MemoryPool.h"

struct IteratorContext;

enum EIterState
{
	EStart					= 0,
	ECheckingPointsInNode	= 1,
	ECheckingChildren		= 2,
};

class _ALIGN_TO(16) SearchIterator
{
public:
	SearchIterator(KdTree<Axis_X>* inTree);
	SearchIterator(KdTree<Axis_Y>* inTree);

	int GetNext(IteratorContext& inContext);

private:
	inline SearchIterator* GetLowerChild() { return this + mChildrenOffset; }
	inline SearchIterator* GetUpperChild() { return this + mChildrenOffset + 1; }

	union
	{
		KdTree<Axis_X>*	mTree;				// 8
		KdTree<Axis_X>*	mXTree;
		KdTree<Axis_Y>*	mYTree;
	};

	int					mSelectedValue;		// 4

	union
	{
		short 			mChildrenOffset;	// 2
		short			mAxis;
	};

	uint8_t 			mIndex;				// 1
	uint8_t				mState;				// 1
};
