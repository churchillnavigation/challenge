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
		const DataPoint *pointsInEnd = m_dataPoints + m_noofPoints;
		const DataPoint *pointsInIter = m_dataPoints;
		/*	const DataPoint *pointsInEndMinus4 = pointsInEnd - 4;
		
		// Unroll loop a bit so compiler can interleave things
		for (; pointsInIter < pointsInEndMinus4; ++pointsInIter)
		{
			bool firstIsIn = IsInRect(rect, pointsInIter[0]);
			bool secondIsIn = IsInRect(rect, pointsInIter[1]);
			if (firstIsIn)
			{
				*resultIter++ = pointsInIter->rank;
				++noofFound;
				if (noofFound >= count)
					break;
			}
			pointsInIter++;
			if (secondIsIn)
			{
				*resultIter++ = pointsInIter->rank;
				++noofFound;
				if (noofFound >= count)
					break;
			}
		}
		
		if (noofFound < count)*/
		{
			// Now do the last odd number if there are any
			for (; pointsInIter < pointsInEnd; ++pointsInIter)
			{
				if (IsInRect(rect, *pointsInIter))
				{
					*resultIter++ = (int)(pointsInIter - m_dataPoints);
					++noofFound;
					if (noofFound >= count)
						break;
				}
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
