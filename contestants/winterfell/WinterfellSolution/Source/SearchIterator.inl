

template< int TAxis >
int	SearchIterator<TAxis>::GetNext(const Rect& inRect, MemoryPool& inAllocator)
{
	// If it's not -1, it's cached from a rejected previous search, 
	// or it has finished (INT32_MAX). Return it now.
	if (mSelectedValue != -1)
	{
		return mSelectedValue;
	}

	switch (mState)
	{
		case EStart:
		{
			if (gRectOverlapsRect(inRect, mTree->mBRect))
			{
				mState = ECheckingPointsInNode;
			}
			else
			{
				// Skip this node's points.
				// mState will get set as the program flow falls through.
				mIndex = mTree->mPointCount;
			}

			// Fall through
		}

		case ECheckingPointsInNode:
		{
			// iterate over tree until a point is found. Return it.
			while (mIndex < mTree->mPointCount)
			{
				if (gIsPointInRect(mTree->mCoords[mIndex], inRect))
				{
					mSelectedValue = mTree->mPointIndices[mIndex];
					goto done;
				}

				++mIndex;
			}

			// If unable to create children iterators, abort.
			if (mTree->mLowerChild == nullptr)
			{
				mSelectedValue = INT32_MAX;
				goto done;
			}

			// Create child iterators
			bool checkLower = (gGetLower<TAxis>(inRect) <= mTree->mMedian);
			bool checkUpper = (gGetUpper<TAxis>(inRect) >= mTree->mMedian);

			if (checkLower && checkUpper)
			{
				mLowerChild = inAllocator.Alloc<SearchIterChild>(mTree->mLowerChild);
				mUpperChild = inAllocator.Alloc<SearchIterChild>(mTree->mUpperChild);
				mState = ECheckingBothChildren;
				goto check_both_children;
			}
			else
			{
				auto child = checkLower ? mTree->mLowerChild : mTree->mUpperChild;
				mLowerChild = inAllocator.Alloc<SearchIterChild>(child);
				mState = ECheckingOneChild;
				goto check_one_child;
			}

			// TODO: Assert that it should not get here.
			break;
		}

		case ECheckingBothChildren:
		{
		check_both_children:
			int lowerSelected = mLowerChild->GetNext(inRect, inAllocator);
			int upperSelected = mUpperChild->GetNext(inRect, inAllocator);
			mSelectedValue = (lowerSelected < upperSelected)
				? (mSelectedChild = ELower, lowerSelected)
				: (mSelectedChild = EUpper, upperSelected);
			break;
		}

		case ECheckingOneChild:
		{
		check_one_child:
			mSelectedValue = mLowerChild->GetNext(inRect, inAllocator);
			break;
		}
	}

	done:
	return mSelectedValue;
}

template< int TAxis >
void SearchIterator<TAxis>::Advance()
{
	if (mSelectedValue == INT32_MAX)
	{
		// Iterator is finished - abort.
		return;
	}

	// TODO: Assert that mSelectedValue != -1 in debug to catch advancing more than once before a search.
	
	switch (mState)
	{
		case ECheckingPointsInNode:
			++mIndex;
			break;

		case ECheckingBothChildren:
		{
			SearchIterChild* selectedChild = (mSelectedChild == ELower) ? mLowerChild : mUpperChild;
			selectedChild->Advance();
			mSelectedChild = ENone;
			break;
		}

		case ECheckingOneChild:
			mLowerChild->Advance();
			break;

		default:
			// TODO: assert that it should not get here.
			break;
	}

	mSelectedValue = -1;
}
