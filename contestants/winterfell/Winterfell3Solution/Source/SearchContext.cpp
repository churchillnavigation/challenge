#include "SearchContext.h"

#include <stdio.h>
#include "DataPrinter.h"
#include "KdTree.h"
#include "MemoryPool.h"
#include "SearchIterator.h"
#include "IteratorContext.h"

#ifdef _ENABLE_STATS_LOGGING

//#define _PRINT_POINTS_AND_QUERIES
//#include <functional>

static SearchContext* spxCurrentContext = nullptr;
StatsSession& gGetCurrentStatsSession()
{
	return spxCurrentContext->mStats.mCurrent;
}
#endif // _ENABLE_STATS_LOGGING

SearchContext::SearchContext(const Point* points_begin, const Point* points_end)
	: mTree(nullptr)
	, mKDTreeMemPool(nullptr)
	, mIteratorMemPool(nullptr)
{
#ifdef _ENABLE_STATS_LOGGING
	spxCurrentContext = this;
#endif // _ENABLE_STATS_LOGGING

	int64_t pointCount = points_end - points_begin;

	{
		// Add and sort all points on rank
		mPoints.assign(points_begin, points_end);

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
		mKDTreeMemPool			= new MemoryPool(byteCount);
	}

	{
		// Create iterator mem pool
		// DANGEROUS: Can overrun memory here,
		// but it's marginally faster than using a checked mem pool...
		static const int kIteratorMemPoolMaxCount = 2000;
		mIteratorMemPool = (SearchIterator*) _aligned_malloc(sizeof(SearchIterator)*kIteratorMemPoolMaxCount, __alignof(SearchIterator));
	}

	{
		// Create KDTree
		mTree = mKDTreeMemPool->Alloc< KdTree<Axis_X> >();
		INC_KDTREE_COUNT;

#ifdef _BALANCE_KDTREE
		std::vector<CoordPairAndRank> coords;
		coords.reserve(mPoints.size());

		for (auto& p : mPoints)
		{
			coords.emplace_back( p );
		}

		mTree->Fill(*mKDTreeMemPool, coords, 0);
#else
		for (auto& p : mPoints)
		{
			mTree->Add(*mKDTreeMemPool, p);
		}
#endif // _BALANCE_KDTREE

		mTree->Finalise();
	}

#ifdef _PRINT_POINTS_AND_QUERIES
	if (pointCount > 1) // Avoid robustness test.
	{
		DataPrinter::DeleteFiles();
		DataPrinter::PrintPoints(&(*mPoints.begin()), &(*mPoints.end()));

		auto copyPoints = mPoints;
		std::sort(copyPoints.begin(), copyPoints.end(),
			[] (const Point& inLHS, const Point& inRHS)
		{
			return inLHS.x < inRHS.x;
		});

		DataPrinter::PrintPoints(&(*copyPoints.begin()), &(*copyPoints.end()), "InputSortedOnX.txt");

		std::sort(copyPoints.begin(), copyPoints.end(),
			[] (const Point& inLHS, const Point& inRHS)
		{
			return inLHS.y < inRHS.y;
		});

		DataPrinter::PrintPoints(&(*copyPoints.begin()), &(*copyPoints.end()), "InputSortedOnY.txt");
	}
#endif // _PRINT_POINTS_AND_QUERIES
}

SearchContext::~SearchContext() 
{
	delete mKDTreeMemPool;
	_aligned_free(mIteratorMemPool);

#ifdef _ENABLE_STATS_LOGGING
	mStats.ExportLog();
	spxCurrentContext = nullptr;
#endif // _ENABLE_STATS_LOGGING

#ifdef _PRINT_POINTS_AND_QUERIES
	if (mQueryRects.size() > 1) // Avoid robustness test.
	{
		for (auto& rect : mQueryRects)
		{
			DataPrinter::AppendRect(rect);
		}

		auto writeRects = [&] (const char* inFilename, const std::function<bool(const Rect&, const Rect&)>& inPred)
		{
			std::sort(mQueryRects.begin(), mQueryRects.end(), inPred);

			for (auto& rect : mQueryRects)
			{
				DataPrinter::AppendRect(rect, inFilename);
			}
		};

		writeRects("RectsSortedOnLX.txt", 
			[](const Rect& inLHS, const Rect& inRHS)
		{
			return inLHS.lx < inRHS.lx;
		});

		writeRects("RectsSortedOnHX.txt",
			[] (const Rect& inLHS, const Rect& inRHS)
		{
			return inLHS.hx < inRHS.hx;
		});

		writeRects("RectsSortedOnLY.txt",
			[] (const Rect& inLHS, const Rect& inRHS)
		{
			return inLHS.ly < inRHS.ly;
		});

		writeRects("RectsSortedOnHY.txt",
			[] (const Rect& inLHS, const Rect& inRHS)
		{
			return inLHS.hy < inRHS.hy;
		});

		writeRects("RectsSortedOnXRange.txt",
			[] (const Rect& inLHS, const Rect& inRHS)
		{
			return (inLHS.hx - inLHS.lx) < (inRHS.hx - inRHS.lx);
		});

		writeRects("RectsSortedOnYRange.txt",
			[] (const Rect& inLHS, const Rect& inRHS)
		{
			return (inLHS.hy - inLHS.ly) < (inRHS.hy - inRHS.ly);
		});
	}
#endif // _PRINT_POINTS_AND_QUERIES
}

int SearchContext::Search(const Rect& rect, const int32_t count, Point* out_points)
{
#ifdef _PRINT_POINTS_AND_QUERIES
	mQueryRects.push_back(rect);
#endif // _PRINT_POINTS_AND_QUERIES

	int* buffer = (int*) alloca(count * sizeof(int));

	IteratorContext ctx(rect, mIteratorMemPool);
	auto iter = ctx.AllocIterator(mTree);

	int nextValue = 0;
	int foundCount = 0;
	while ((foundCount < count) & (nextValue != INT32_MAX))
	{
		nextValue = iter->GetNext(ctx);
		buffer[foundCount++] = nextValue;
		ctx.mLastRankSelected = nextValue;
	}

	if (nextValue == INT32_MAX)
	{
		--foundCount;
	}

	for (int i = 0; i < foundCount; ++i)
	{
		out_points[i] = mPoints[buffer[i]];
	}

	//LOG("Last point index = %i", buffer[foundCount-1]);

#ifdef _ENABLE_STATS_LOGGING
	mStats.CommitCurrent();
#endif // _ENABLE_STATS_LOGGING

	return foundCount;
}
