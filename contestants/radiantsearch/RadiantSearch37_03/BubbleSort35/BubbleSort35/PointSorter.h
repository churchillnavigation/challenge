#pragma once

#include <algorithm>
#include <windows.h>
#include "sort.h"
#include "../../point_search.h"

#define CHECK(BL_AH) do {if (!(BL_AH)) DebugBreak(); } while(0)

#define USE_INT_POSITIONS
#define USE_COLD_DATA

struct SearchContext
{
public:
					SearchContext() {}
	virtual			~SearchContext() {}
	
	virtual void Create(const Point* points_begin, const Point* points_end) = 0;
	virtual void PostCreate(const Point* points_begin, const Point* points_end) = 0;
	virtual int32_t Search(const Rect & rect, const int32_t count, Point* out_points) = 0;
};

typedef int32_t RankType;
typedef signed __int64 int64;
typedef int32_t int32;
typedef unsigned int uint32;

#ifdef USE_INT_POSITIONS
typedef int PosType;
#else
typedef float PosType;
#endif

inline int MIN(int a, int b) { return (a < b) ? a : b; }
inline int MAX(int a, int b) { return (a > b) ? a : b; }

// A 3 byte structure to store an integer from -8 million to 8 million
struct Int24
{
	struct Int24Bitfield
	{
		int32 m_val24 : 24;
	};

	char m_bytes[3];
	
	Int24()
	{
	}

	inline Int24(int val)
	{
		Int24Bitfield *int24Ptr = reinterpret_cast<Int24Bitfield *>(this);
		int24Ptr->m_val24 = val;
	}

	inline void operator=(const int newVal)
	{
		Int24Bitfield *int24Ptr = reinterpret_cast<Int24Bitfield *>(this);
		int24Ptr->m_val24 = newVal;
	}

	inline operator int() const
	{
		const Int24Bitfield *int24Ptr = reinterpret_cast<const Int24Bitfield *>(this);
		return int24Ptr->m_val24;
	}
	
	inline bool operator<(const int & rhs) const { return int(*this) < rhs; }
	inline bool operator>(const int & rhs) const { return int(*this) > rhs; }
	inline int  operator-(const int & rhs) const { return int(*this) - rhs; }
	inline int  operator+(const int & rhs) const { return int(*this) + rhs; }
};

// A 3 byte structure to store an unsigned integer up to 16 million
struct UInt24
{
	struct UInt24Bitfield
	{
		uint32 m_val24 : 24;
	};

	char m_bytes[3];
	
	UInt24()
	{
	}

	inline UInt24(int val)
	{
		UInt24Bitfield *int24Ptr = reinterpret_cast<UInt24Bitfield *>(this);
		int24Ptr->m_val24 = val;
	}

	inline void operator=(const int newVal)
	{
		UInt24Bitfield *int24Ptr = reinterpret_cast<UInt24Bitfield *>(this);
		int24Ptr->m_val24 = newVal;
	}

	inline operator int() const
	{
		const UInt24Bitfield *int24Ptr = reinterpret_cast<const UInt24Bitfield *>(this);
		return int24Ptr->m_val24;
	}

/*	inline operator uint32() const
	{
		const UInt24Bitfield *int24Ptr = reinterpret_cast<const UInt24Bitfield *>(this);
		return int24Ptr->m_val24;
	}*/
	
	inline bool operator<(const int & rhs) const { return int(*this) < rhs; }
	inline bool operator>(const int & rhs) const { return int(*this) > rhs; }
	inline int  operator-(const int & rhs) const { return int(*this) - rhs; }
	inline int  operator+(const int & rhs) const { return int(*this) + rhs; }
};

#ifdef USE_INT_POSITIONS
typedef UInt24 PosSmallStorageType;
#else
typedef float PosSmallStorageType;
#endif

typedef UInt24 RankSmallStorageType;
	
struct RectType
{		
	PosType m_lx;
	PosType m_ly;
	PosType m_hx;
	PosType m_hy;

	unsigned int m_xSize;
	unsigned int m_ySize;

	struct Relationship
	{
		enum Enum
		{
			Contains,
			Overlaps,
			Distinct
		};
	};

	bool Overlaps(const RectType & other) const
	{
		if (m_hx < other.m_lx)
			return false;
		if (m_lx > other.m_hx)
			return false;
		if (m_hy < other.m_ly)
			return false;
		if (m_ly > other.m_hy)
			return false;
		return true;
	}

	bool Contains(const RectType & other) const
	{
		if (m_hx < other.m_hx)
			return false;
		if (m_lx > other.m_lx)
			return false;
		if (m_hy < other.m_hy)
			return false;
		if (m_ly > other.m_ly)
			return false;
		return true;
	}

	/*State::Enum GetRelationshipTo(const RectType & other) const
	{
		if (m_hx < other.m_lx)
			return State::Distinct;
		if (m_lx > other.m_hx)
			return State::Distinct;
		if (m_hy < other.m_ly)
			return State::Distinct;
		if (m_ly > other.m_hy)
			return State::Distinct;
	}*/
};

#ifdef USE_COLD_DATA
struct ColdData
{
	int8_t id;
	float x;
	float y;
};
#endif

struct RankedDataPoint
{
#ifndef USE_COLD_DATA
	int8_t id;
#endif
	RankSmallStorageType rank;
	PosSmallStorageType x;
	PosSmallStorageType y;

public:
	bool	operator<(const RankedDataPoint & rhs) const { return rank < rhs.rank; }
};

struct XSortedDataPoints
{
	RankSmallStorageType rank;
	PosSmallStorageType y;
};

struct YSortedDataPoints
{
	RankSmallStorageType rank;
	PosSmallStorageType x;
};

// These don't need the rank as they are stored in rank order
struct DataPoint
{
	PosSmallStorageType x;
	PosSmallStorageType y;
};

struct Coordinate
{
	RankSmallStorageType rank;
	float pos;

public:
	operator float() const { return pos; }
	bool	operator<(const Coordinate & rhs) const { return pos < rhs.pos; }
	bool	operator<(const float & rhs) const { return pos < rhs; }
};
	
#ifdef USE_INT_POSITIONS
static PosType ConvertCoordinateMin(const float *sortedCoords, int noofCoords, float searchVal)
{
	const float *found = std::lower_bound(sortedCoords, sortedCoords + noofCoords, searchVal);

	return PosType(found - sortedCoords);
}
	
static PosType ConvertCoordinateMax(const float *sortedCoords, int noofCoords, float searchVal)
{
	// Get the first point that is strictly greater than the given value, then subtract one to get the last one that should be included in this max
	const float *found = std::upper_bound(sortedCoords, sortedCoords + noofCoords, searchVal);
	return PosType(found - sortedCoords) - 1;
}
#endif

RectType ConvertRect(const Rect &rectIn, bool & outOfBounds, const float *xCoordsSorted, const float *yCoordsSorted, const int noofPoints)
{
	RectType newRect;
	outOfBounds = false;
#ifdef USE_INT_POSITIONS
	// Find matching integer positions for each float position
	newRect.m_lx = ConvertCoordinateMin(xCoordsSorted, noofPoints, rectIn.lx);
	if (newRect.m_lx >= noofPoints)
	{
		outOfBounds = true;
		return newRect;
	}
	newRect.m_ly = ConvertCoordinateMin(yCoordsSorted, noofPoints, rectIn.ly);
	if (newRect.m_ly >= noofPoints)
	{
		outOfBounds = true;
		return newRect;
	}
	newRect.m_hx = ConvertCoordinateMax(xCoordsSorted, noofPoints, rectIn.hx);
	if (newRect.m_hx < newRect.m_lx)
	{
		outOfBounds = true;
		return newRect;
	}
	newRect.m_hy = ConvertCoordinateMax(yCoordsSorted, noofPoints, rectIn.hy);
	if (newRect.m_hy < newRect.m_ly)
	{
		outOfBounds = true;
		return newRect;
	}
#else
	newRect.m_lx = rectIn.lx;
	newRect.m_ly = rectIn.ly;
	newRect.m_hx = rectIn.hx;
	newRect.m_hy = rectIn.hy;
#endif

	newRect.m_xSize = newRect.m_hx - newRect.m_lx;
	newRect.m_ySize = newRect.m_hy - newRect.m_ly;

	return newRect;
}
	
template <typename DataType>
bool IsInRect(const RectType & rect, const DataType & point)
{
#ifdef USE_INT_POSITIONS
	// If it's out of bounds, one or more of these unsigned ints will be a huge number
	unsigned int xRel = point.x - rect.m_lx;
	unsigned int yRel = point.y - rect.m_ly;	

//*
	unsigned int xRelEnd = rect.m_hx - point.x;
	unsigned int yRelEnd = rect.m_hy - point.y;	
	
	// Or them together and see if the answer is huge
	if ((xRel | xRelEnd | yRel | yRelEnd) & 0x80000000)
		return false;

/*/		
	// Use unsigned int trick to just do one range check
	// TODO: Can these be combined?
	if (xRel > rect.m_xSize)
		return false;
	if (yRel > rect.m_ySize)
		return false;
//		*/
#else
	if (point.x < rect.m_lx)
		return false;
	if (point.x > rect.m_hx)
		return false;
	if (point.y < rect.m_ly)
		return false;
	if (point.y > rect.m_hy)
		return false;
#endif

	return true;
}

// Returns the highest rank still on the list
RankType InsertRankIntoTopN(RankType *topNList, int N, int numFoundSoFar, const RankType rankToInsert)
{
	int listSize;
	RankType highestOnList;
	if (numFoundSoFar >= N)
	{
		listSize = N;
		highestOnList = topNList[N - 1];
		if (rankToInsert >= highestOnList)
		{
			// too high to go on the list, and the list is full
			return highestOnList;
		}
	}
	else if (numFoundSoFar)
	{
		listSize = numFoundSoFar;
		highestOnList = topNList[listSize - 1];
		if (rankToInsert >= highestOnList)
		{
			// too high to go on the list, but the list isn't full so add at the end
			topNList[listSize] = rankToInsert;
			return rankToInsert;
		}
	}
	else
	{
		topNList[0] = rankToInsert;
		return rankToInsert;
	}
	
	highestOnList = topNList[listSize - 1];

	// Find the place for this to go
	RankType * firstHigherRank = std::lower_bound(topNList, topNList + listSize, rankToInsert);
	int rankID = int(firstHigherRank - topNList);

	// Insert it here, and shuffle the rest down
	if (numFoundSoFar < N)
	{
		for (int higherID = listSize - 1; higherID >= rankID; --higherID)
		{
			topNList[higherID + 1] = topNList[higherID];
		}
	}
	else
	{
		for (int higherID = N - 2; higherID >= rankID; --higherID)
		{
			topNList[higherID + 1] = topNList[higherID];
		}
	}
		
	topNList[rankID] = rankToInsert;

	return highestOnList;
}

template <typename TRankStorageType>
void InsertRanksIntoTopN(RankType *topNList, int N, int numFoundSoFar, const TRankStorageType * ranksToInsert, int noofRanksToInsert)
{

	RankType highestSoFar = numFoundSoFar ? topNList[MIN(numFoundSoFar, N) - 1] : INT_MAX;
	const TRankStorageType * rankIter = ranksToInsert;
	for (int rankID = 0; rankID < noofRanksToInsert; ++rankID)
	{
		RankType rank = *rankIter++;
		if ((rank >= highestSoFar) && (numFoundSoFar >= N))
		{
			continue;
		}
		highestSoFar = InsertRankIntoTopN(topNList, N, numFoundSoFar, rank);
		numFoundSoFar++;
	}
}

// These were members of PointSorter, but that's not very efficient when using virtual inheritance to derive from it, so this is a HACK!
	const DataPoint *		m_dataPoints;
	const RankedDataPoint *	m_dataPointsWithRank;
#ifdef USE_COLD_DATA
	ColdData *				m_coldData;
#endif
	RankType *				m_resultBuffer;
	RankType *				m_untestedResults;
	float *					m_xCoordsSorted;
	float *					m_yCoordsSorted;

#ifdef USE_INT_POSITIONS
	XSortedDataPoints *		m_xSortedDataPoints;
	YSortedDataPoints *		m_ySortedDataPoints;
#endif

	int						m_noofPoints;

class PointSorter : public SearchContext
{
public:
	PointSorter() 
		: SearchContext()
	{
	}

	~PointSorter()
	{
		delete[] m_dataPoints;
#ifdef USE_COLD_DATA
		delete[] m_coldData;
#endif
		delete[] m_resultBuffer;
		delete[] m_untestedResults;
	}

	virtual void Create(const Point* points_begin, const Point* points_end) override
	{
		m_noofPoints = (int)(points_end - points_begin);
		m_dataPoints = new DataPoint[m_noofPoints];	
		m_dataPointsWithRank = new RankedDataPoint[m_noofPoints];
#ifdef USE_INT_POSITIONS
		m_xSortedDataPoints = new XSortedDataPoints[m_noofPoints];
		m_ySortedDataPoints = new YSortedDataPoints[m_noofPoints];
#endif

		const int MaxResults = 100;
		m_resultBuffer = new RankType[MaxResults];	// technically they only ever ask for 20 results
		for (int resID = 0; resID < MaxResults; ++resID)
		{
			// Write to all of this memory so the penalty for committing it isn't paid during testing
			m_resultBuffer[resID] = resID;
		}
		const int MaxUntestedResults = 10000;
		m_untestedResults = new RankType[MaxUntestedResults];
		for (int resID = 0; resID < MaxUntestedResults; ++resID)
		{
			// Write to all of this memory so the penalty for committing it isn't paid during testing
			m_untestedResults[resID] = resID;
		}
#ifdef USE_INT_POSITIONS
		Coordinate *xCoordsSorted = new Coordinate[m_noofPoints];
		Coordinate *yCoordsSorted = new Coordinate[m_noofPoints];
#else
		m_xCoordsSorted = nullptr;
		m_yCoordsSorted = nullptr;
#endif
		RankedDataPoint *pointsOutIter = const_cast<RankedDataPoint *>(m_dataPointsWithRank);
		const Point *pointsInIter = points_begin;
		for (int pid = 0; pid < m_noofPoints; ++pid, ++pointsOutIter, ++pointsInIter)
		{
			RankType rank = pointsInIter->rank;
			pointsOutIter->rank = rank;
#ifndef USE_COLD_DATA
			pointsOutIter->id = pointsInIter->id;
#endif
#ifndef USE_INT_POSITIONS
			pointsOutIter->x = pointsInIter->x;
			pointsOutIter->y = pointsInIter->y;
#endif
			
#ifdef USE_INT_POSITIONS
			xCoordsSorted[rank].rank = rank;
			xCoordsSorted[rank].pos = pointsInIter->x;
			
			yCoordsSorted[rank].rank = rank;
			yCoordsSorted[rank].pos = pointsInIter->y;
#endif
		}
		
		DataPoint * pointsNonConst = const_cast<DataPoint *>(m_dataPoints);
		RankedDataPoint *rankedPointsNonConst = const_cast<RankedDataPoint *>(m_dataPointsWithRank);
		
		// Sort points by rank
		std::sort(rankedPointsNonConst, rankedPointsNonConst + m_noofPoints);
		//QuickSort(pointsNonConst, m_noofPoints);

#ifdef USE_INT_POSITIONS
		// Sort points by X, and then assign a unique integer to each floating point x coordinate, 
		//   so the positions can be dealt with as ints from then on
		std::sort(xCoordsSorted, xCoordsSorted + m_noofPoints);
		std::sort(yCoordsSorted, yCoordsSorted + m_noofPoints);
		// Look up the data points by rank now they have been sorted, and give them an integer position
		for (int coordID = 0; coordID < m_noofPoints; ++coordID)
		{
			int xRank = xCoordsSorted[coordID].rank;
			int yRank = yCoordsSorted[coordID].rank;
			pointsNonConst[xRank].x = coordID;
			pointsNonConst[yRank].y = coordID;
			rankedPointsNonConst[xRank].x = coordID;
			rankedPointsNonConst[yRank].y = coordID;
		}
		// Fill in m_xSortedDataPoints
		for (int coordID = 0; coordID < m_noofPoints; ++coordID)
		{
			{
				int pointRank = xCoordsSorted[coordID].rank;
				m_xSortedDataPoints[coordID].y = pointsNonConst[pointRank].y;
				m_xSortedDataPoints[coordID].rank = pointRank;
			}
			{
				int pointRank = yCoordsSorted[coordID].rank;
				m_ySortedDataPoints[coordID].x = pointsNonConst[pointRank].x;
				m_ySortedDataPoints[coordID].rank = pointRank;
			}
		}

#else
		for (int coordID = 0; coordID < m_noofPoints; ++coordID)
		{
			pointsNonConst[coordID].x = rankedPointsNonConst[coordID].x;
			pointsNonConst[coordID].y = rankedPointsNonConst[coordID].y;
		}
#endif

		// Compact sorted coordinates down to just floats
		
		m_xCoordsSorted = new float[m_noofPoints];
		m_yCoordsSorted = new float[m_noofPoints];
		
		for (int coordID = 0; coordID < m_noofPoints; ++coordID)
		{
			m_xCoordsSorted[coordID] = xCoordsSorted[coordID].pos;
			m_yCoordsSorted[coordID] = yCoordsSorted[coordID].pos;
		}

		delete[] xCoordsSorted;
		delete[] yCoordsSorted;
	}

	virtual void PostCreate(const Point* points_begin, const Point* points_end) override
	{
		// Create cold data at last possible moment, to minimise the peak memory usage

		// TODO: Don't need cold data - include id in m_dataPointsWithRank and look up float positions from coords arrays 
#ifdef USE_COLD_DATA
		m_coldData = new ColdData[m_noofPoints];

		const Point *pointsInIter = points_begin;
		for (int pid = 0; pid < m_noofPoints; ++pid, ++pointsInIter)
		{
			RankType rank = pointsInIter->rank;
			m_coldData[rank].id = pointsInIter->id;
			m_coldData[rank].x = pointsInIter->x;
			m_coldData[rank].y = pointsInIter->y;
		}

		delete[] m_dataPointsWithRank;
#endif

	}

	static void ProduceOutputData(int noofResults, const RankType *resultRanks, Point* pointsOut)
	{
		Point *pointsOutIter = pointsOut;
		const RankType *ranksIter = resultRanks;
		for (int pid = 0; pid < noofResults; ++pid, ++pointsOutIter)
		{
			RankType rank = *ranksIter++;
			pointsOutIter->rank = rank;
#ifdef USE_COLD_DATA
			pointsOutIter->id = m_coldData[rank].id;
			pointsOutIter->x = m_coldData[rank].x;
			pointsOutIter->y = m_coldData[rank].y;
#else
			const RankedDataPoint &dataPoint = m_dataPointsWithRank[rank];
			pointsOutIter->id = dataPoint.id;
			pointsOutIter->x = m_xCoordsSorted[dataPoint.x];
			pointsOutIter->y = m_yCoordsSorted[dataPoint.y];
#endif
		}
	}

	static void InitResults(RankType *results, int count)
	{
		for (int i=0; i<count; i++)
		{
			results[i] = INT_MAX;
		}
	}

};