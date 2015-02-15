#pragma once

#include "KDTree.h"
#include "LinearSearch.h"
#include <iostream>


double PCFreq = 0.0;
__int64 CounterStart = 0;

void StartCounter()
{
    LARGE_INTEGER li;
    if(!QueryPerformanceFrequency(&li))
	{
		std::cout << "QueryPerformanceFrequency failed!\n";
		PCFreq = 0.0f;
		return;
	}

    PCFreq = double(li.QuadPart)/1000.0;

    QueryPerformanceCounter(&li);
    CounterStart = li.QuadPart;
}
double GetCounter()
{
    LARGE_INTEGER li;
    QueryPerformanceCounter(&li);
    return double(li.QuadPart-CounterStart)/PCFreq;
}

class HybridSearch : public KDTreeSearch, public LinearSearch
{
public:
	virtual void Create(const Point* points_begin, const Point* points_end) override
	{
		KDTreeSearch::Create(points_begin, points_end);
	}

	virtual int32_t Search(const Rect & rectIn, const int32_t count, Point* pointsOut) override
	{
		bool outOfBounds = false;
		RectType rect = ConvertRect(rectIn, outOfBounds, m_xCoordsSorted, m_yCoordsSorted, m_noofPoints);
		if (outOfBounds || (!m_noofPoints))
		{
			return 0;
		}

#ifdef _DEBUG
		// debug check that linear and KDTree search agree
		Point linearResults[100];
		Point kdTreeResults[100];

		int noofLinearResults = LinearSearch::DoSearch(rect, count, linearResults);
		int noofKDTreeResults = KDTreeSearch::DoSearch(rect, count, kdTreeResults);
		CHECK(noofLinearResults == noofKDTreeResults);
		for (int res = 0; res < noofLinearResults; res++)
		{
			CHECK(linearResults[res] == kdTreeResults[res]);
		}
#endif

//#define COMPARE_METHODS

		// TODO: Is it faster using int positions? If they can be compacted to 34bit then yes
		// Choose algorithm based on area of whole world covered (in integer space where points are evenly distributed)
#ifdef USE_INT_POSITIONS
		int64 noofPointsNonZero = 1 + m_noofPoints;
		int64 xSize = rect.m_hx - rect.m_lx;
		int64 ySize = rect.m_hy - rect.m_ly;
		int64 areaCoverageMillionths;
		if (xSize > 500)
		{
			// funny ordering to avoid wrapping
			areaCoverageMillionths = (((xSize * 1000000) / noofPointsNonZero) * ySize) / noofPointsNonZero;
		}
		else
		{
			areaCoverageMillionths = (xSize * ySize * 1000000) / (noofPointsNonZero * noofPointsNonZero);
		}

	#ifdef COMPARE_METHODS
		StartCounter();
		int ret = LinearSearch::DoSearch(rect, count, pointsOut);
		double linearTime = GetCounter();
		StartCounter();
		ret = KDTreeSearch::DoSearch(rect, count, pointsOut);
		double kdTime = GetCounter();

		std::cout << areaCoverageMillionths << ", " << linearTime << ", " << kdTime << "\n";

		return ret;
	#else
		// TODO: temp removal
		if (areaCoverageMillionths > 1000)
		{
			// Just do a linear search, because to find 20 points we're probably going to have to do less than 4000 bounds tests
			// Whereas for a coverage of 5 thousandths we're going to find 50000 points in that region and have to sort their ranks
			// TODO: If we have a more efficient algorithm for gathering the points, which only gathers N from each sub-tree, then this can be increased.
			return LinearSearch::DoSearch(rect, count, pointsOut);
		}
		else
		{
			// For a smaller area, do a spatial search
			return KDTreeSearch::DoSearch(rect, count, pointsOut);
		}
	#endif

#else
		return KDTreeSearch::DoSearch(rect, count, pointsOut);
#endif
	}
};