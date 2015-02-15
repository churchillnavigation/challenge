#include "SearchContext.h"

#include "KdTree.h"
#include "MemoryPool.h"
#include "SearchIterator.h"

SearchContext::SearchContext(const Point* points_begin, const Point* points_end)
	: mTree(nullptr)
	, mKDTreeMemPool(nullptr)
	, mIteratorMemPool(nullptr)
{
	int64_t pointCount = points_end - points_begin;

	{
		// Add and sort all points on rank
		mPoints.reserve((int) pointCount);
		for (const Point* iter = points_begin; iter != points_end; ++iter)
		{
			mPoints.push_back(*iter);
		}

		std::sort(mPoints.begin(), mPoints.end(),
			[] (const Point& inLHS, const Point& inRHS)
		{
			return inLHS.rank < inRHS.rank;
		});
	}

	{
		// Create KDTree Mem Pool
		int minimumNodeCount	= (int) (pointCount / kBinSize) + 1;
		int byteCount			= sizeof(KdTree<Axis_X>) * minimumNodeCount;
		byteCount				+= (64 - (byteCount % 64));

		mKDTreeMemPool			= new MemoryPool(byteCount);
	}

	{
		// Create iterator mem pool
		int bytes = sizeof( SearchIterator<Axis_X> ) * 5000;
		mIteratorMemPool = new MemoryPool(bytes);
	}

	{
		// Create KDTree
		mTree = mKDTreeMemPool->Alloc< KdTree<Axis_X> >();

		for (size_t i = 0; i < mPoints.size(); ++i)
		{
			mTree->Add(*mKDTreeMemPool, mPoints[i], (int) i);
		}
	}
}

SearchContext::~SearchContext() 
{
	delete mKDTreeMemPool;
	delete mIteratorMemPool;
}

int SearchContext::Search(const Rect& rect, const int32_t count, Point* out_points)
{
	mIteratorMemPool->Reset();

	auto iter = mIteratorMemPool->Alloc< SearchIterator<Axis_X> >(mTree);

	int* buffer = (int*) alloca(count * sizeof(int));

	int nextValue = 0;
	int foundCount = 0;
	while ((foundCount < count) && (nextValue != INT32_MAX))
	{
		nextValue = iter->GetNext(rect, *mIteratorMemPool);
		if (nextValue != INT32_MAX)
		{
			buffer[foundCount++] = nextValue;
			iter->Advance();
		}
	}

	for (int i = 0; i < foundCount; ++i)
	{
		out_points[i] = mPoints[buffer[i]];
	}

	return foundCount;
}
