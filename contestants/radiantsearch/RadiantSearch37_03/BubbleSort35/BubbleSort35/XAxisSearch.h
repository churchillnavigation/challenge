#pragma once

#include "PointSorter.h"

class AxisSearch : public virtual PointSorter
{
public:
	static const int MaxAxisSearchSize = 5000;

	int32_t DoSearchX(const RectType & rect, const int32_t count, Point* pointsOut)
	{
		RankType ranks[20];
#ifdef _DEBUG
		CHECK(count <= ARRAYSIZE(ranks));
#endif
		
		InitResults(ranks, count);

		int noofFound = 0;
		const XSortedDataPoints *pointsInIter = m_xSortedDataPoints + rect.m_lx;
		const XSortedDataPoints *pointsInEnd = m_xSortedDataPoints + rect.m_hx + 1;
		
		for (int x = rect.m_lx; pointsInIter < pointsInEnd; ++pointsInIter, ++x)
		{
			int y = pointsInIter->y;
			
			// Use unsigned int trick to just do one range check
			unsigned int yRel = y - rect.m_ly;
			if (yRel > rect.m_ySize)
				continue;

			// Inside the rectangle so add it to the list
			int rank = pointsInIter->rank;
			InsertRankIntoTopN(ranks, count, noofFound, rank);
			noofFound++;
		}

		if (noofFound > count)
		{
			noofFound = count;
		}
		
		ProduceOutputData(noofFound, ranks, pointsOut);

		return noofFound;
	}

	int32_t DoSearchY(const RectType & rect, const int32_t count, Point* pointsOut)
	{
		RankType ranks[20];
#ifdef _DEBUG
		CHECK(count <= ARRAYSIZE(ranks));
#endif
		
		InitResults(ranks, count);

		int noofFound = 0;
		const YSortedDataPoints *pointsInIter = m_ySortedDataPoints + rect.m_ly;
		const YSortedDataPoints *pointsInEnd = m_ySortedDataPoints + rect.m_hy + 1;
		
		for (int y = rect.m_ly; pointsInIter < pointsInEnd; ++pointsInIter, ++y)
		{
			int x = pointsInIter->x;
			
			// Use unsigned int trick to just do one range check
			unsigned int xRel = x - rect.m_lx;
			if (xRel > rect.m_xSize)
				continue;

			// Inside the rectangle so add it to the list
			int rank = pointsInIter->rank;
			InsertRankIntoTopN(ranks, count, noofFound, rank);
			noofFound++;
		}

		if (noofFound > count)
		{
			noofFound = count;
		}
		
		ProduceOutputData(noofFound, ranks, pointsOut);

		return noofFound;
	}


	virtual int32_t Search(const Rect & rectIn, const int32_t count, Point* pointsOut) override
	{
		bool outOfBounds = false;
		RectType rect = ConvertRect(rectIn, outOfBounds, m_xCoordsSorted, m_yCoordsSorted, m_noofPoints);
		if (outOfBounds)
		{
			return 0;
		}

		return DoSearchX(rect, count, pointsOut);
	}
};
