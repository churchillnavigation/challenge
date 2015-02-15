#pragma once

#include "KDTree.h"
#include "XAxisSearch.h"
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

class HybridSearch : public KDTreeSearch, public LinearSearch, public AxisSearch
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
		Point xAxisResults[100];

		int noofLinearResults = LinearSearch::DoSearch(rect, count, linearResults);
		int noofKDTreeResults = KDTreeSearch::DoSearch(rect, count, kdTreeResults);
		int noofXAxisResults = AxisSearch::DoSearchX(rect, count, xAxisResults);
		CHECK(noofLinearResults == noofKDTreeResults);
		CHECK(noofLinearResults == noofXAxisResults);
		for (int res = 0; res < noofLinearResults; res++)
		{
			CHECK(linearResults[res] == kdTreeResults[res]);
			CHECK(linearResults[res] == xAxisResults[res]);
		}
#endif
		
//#define COMPARE_METHODS
//#define PROFILE_KD_TREE

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
		
		int64 minAxis;
		bool xIsLower;
		if (xSize < ySize)
		{
			minAxis = xSize;
			xIsLower = true;
		}
		else
		{
			minAxis = ySize;
			xIsLower = false;
		}

	#ifdef COMPARE_METHODS
		static int loopOffset = 0;
		loopOffset++;
		double linearTime;
		double kdTime;
		double xAxisTime;
		int ret;
		for (int i=0; i<3; i++)
		{
			switch ((i + loopOffset) % 3)
			{
			case 0:
				StartCounter();
				ret = KDTreeSearch::DoSearch(rect, count, pointsOut);
				kdTime = GetCounter();
				break;
			case 1:
				StartCounter();
				ret = LinearSearch::DoSearch(rect, count, pointsOut);
				linearTime = GetCounter();
				break;
			case 2:
				if (minAxis <= MaxAxisSearchSize)
				{
					if (xIsLower)
					{
						StartCounter();
						ret = AxisSearch::DoSearchX(rect, count, pointsOut);
						xAxisTime = GetCounter();
					}
					else
					{
						StartCounter();
						ret = AxisSearch::DoSearchY(rect, count, pointsOut);
						xAxisTime = GetCounter();
					}
				}
				else
				{
					xAxisTime = 1.0;
				}
				break;
			}
		}

		std::cout << xSize << ", " << ySize << ", " << areaCoverageMillionths << ", " << linearTime << ", " << kdTime << ", " << xAxisTime << "\n";

		return ret;		
	#else
		#ifdef PROFILE_KD_TREE
			if ((areaCoverageMillionths >= 0) && (areaCoverageMillionths <= 150))
			{
				for (int i=0; i<10000; i++)
				{
					KDTreeSearch::DoSearch(rect, count, pointsOut);
				}
			}
		#endif

		// TODO: For very small search areas use a grid search
		// sort all points by grid index and store a copy of the data like that - potentially z-swizzled ordering?
		// Each grid bucket just an index of first point and a short for the number in that bucket.
		// 

		// TODO: temp removal

		if (minAxis <= 3500)
		{
			if (xIsLower)
			{
				return AxisSearch::DoSearchX(rect, count, pointsOut);
			}
			else
			{
				return AxisSearch::DoSearchY(rect, count, pointsOut);
			}

			// TODO: Prep the rank list by finding some points with axis search even if we go on to use another algorithm?

			// TODO: 
		}
		else if (areaCoverageMillionths > 2500) // TODO: improve KDTree and increase this number
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