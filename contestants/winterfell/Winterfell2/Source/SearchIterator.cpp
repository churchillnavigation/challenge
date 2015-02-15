#include "SearchIterator.h"

#include "IteratorContext.h"


SearchIterator::SearchIterator(KdTree<Axis_X>* inTree)
	: mXTree(inTree)
	, mAxis(Axis_X)
	, mSelectedValue(-1)
	, mIndex(0)
	, mState(EStart)
{
}

SearchIterator::SearchIterator(KdTree<Axis_Y>* inTree)
	: mYTree(inTree)
	, mAxis(Axis_Y)
	, mSelectedValue(-1)
	, mIndex(0)
	, mState(EStart)
{
}

int	SearchIterator::GetNext(IteratorContext& inContext)
{
	PUSH_ITER_DEPTH;

	// If it's not -1, it's cached from a rejected previous search, 
	// or it has finished (INT32_MAX). Return it now.
	if (mSelectedValue != inContext.mLastRankSelected)
	{
		return mSelectedValue;
	}

	switch (mState)
	{
		case EStart:
		{
			if (gRectOverlapsRect(inContext.mRect, mTree->mBRect))
			{
				mState = ECheckingPointsInNode;
				CULL_POINTS_CHECK_MISS;
			}
			else
			{
				// Skip this node's points.
				// mState will get set as the program flow falls through.
				mIndex = mTree->mPointCount;
				CULL_POINTS_CHECK_HIT;
			}

			// fall through
		}

		case ECheckingPointsInNode:
		{
			// iterate over tree until a point is found. Return it.
			while (mIndex < mTree->mPointCount)
			{
				if (gIsPointInRect(mTree->mCoords[mIndex], inContext.mRect))
				{
					mSelectedValue = mTree->mPointIndices[mIndex];
					INC_POINTINRECT_HIT;
					++mIndex;
					return mSelectedValue;
				}
				else
				{
					INC_POINTINRECT_MISS;
					++mIndex;
				}
			}

			// If unable to create children iterators, abort.
			if (mTree->mLowerChild == nullptr)
			{
				mSelectedValue = INT32_MAX;
				return mSelectedValue;
			}

			// Create child iterators
			bool checkLower = false, checkUpper = false;
			switch (mAxis)
			{
				case Axis_X:
					checkLower = mXTree->DoesRectOverlapLowerChild(inContext.mRect);
					checkUpper = mXTree->DoesRectOverlapUpperChild(inContext.mRect);
					if (checkLower & checkUpper)
					{
						mChildrenOffset = (short) (inContext.AllocIterator(mXTree->mLowerChild) - this);
						inContext.AllocIterator(mXTree->mUpperChild); // allocate upper too.
					}
					break;

				case Axis_Y:
					checkLower = mYTree->DoesRectOverlapLowerChild(inContext.mRect);
					checkUpper = mYTree->DoesRectOverlapUpperChild(inContext.mRect);
					if (checkLower & checkUpper)
					{
						mChildrenOffset = (short) (inContext.AllocIterator(mYTree->mLowerChild) - this);
						inContext.AllocIterator(mYTree->mUpperChild); // allocate upper too.
					}
					break;
			}

			if (checkLower & checkUpper)
			{
				GetLowerChild()->mSelectedValue = inContext.mLastRankSelected;
				GetUpperChild()->mSelectedValue = inContext.mLastRankSelected;
				mState = ECheckingChildren;
			}
			else
			{
				// Reset this iterator to next child.
				// This collapses some pointer indirection.
				auto child = checkLower ? mTree->mLowerChild : mTree->mUpperChild;

				mTree = reinterpret_cast< KdTree<Axis_X>* >(child);
				mSelectedValue = inContext.mLastRankSelected;
				mIndex = 0;
				mState = EStart;
				mAxis = mAxis^1;
				return GetNext(inContext);
			}

			// fall through
		}

		case ECheckingChildren:
		{
			int lowerSelected = GetLowerChild()->GetNext(inContext);
			int upperSelected = GetUpperChild()->GetNext(inContext);
			mSelectedValue = std::min(lowerSelected, upperSelected);

			// fall through
		}
	}

	return mSelectedValue;
}
