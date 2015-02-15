#pragma once

#include "PointSorter.h"

class LinearSearch : public virtual PointSorter
{
public:
	int32_t DoSearch(const RectType & rect, const int32_t count, Point* pointsOut)
	{
		RankType * lowestRankPoints = m_resultBuffer;
		RankType * resultIter = lowestRankPoints;
		int noofFound = 0;
		const DataPoint *pointsInIter = m_dataPoints;
		for (int pid = 0; pid < m_noofPoints; ++pid, ++pointsInIter)
		{
			if (IsInRect(rect, *pointsInIter))
			{
				*resultIter++ = pointsInIter->rank;
				++noofFound;
				if (noofFound >= count)
					break;
			}
		}

		ProduceOutputData(noofFound, lowestRankPoints, pointsOut);

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

		return DoSearch(rect, count, pointsOut);
	}
};
