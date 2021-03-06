#pragma once

#include <algorithm>
#include <windows.h>
#include "sort.h"
#include "../../point_search.h"

#define CHECK(BL_AH) do {if (!(BL_AH)) DebugBreak(); } while(0)

#define USE_INT_POSITIONS

struct SearchContext
{
public:
					SearchContext() {}
	virtual			~SearchContext() {}
	
	virtual void Create(const Point* points_begin, const Point* points_end) = 0;
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
};

struct ColdData
{
	// TODO: Can this be packed to 1 byte so the floats are misaligned?
	int8_t id;
	float x;
	float y;
};

struct DataPoint
{
	// TODO: Change RankType and PosType to be a 3 byte struct with operators to cast to/from int
	// TODO: Can this be packed to 1 byte so the ints are misaligned?

//	int8_t id;
	RankSmallStorageType rank;
	PosSmallStorageType x;
	PosSmallStorageType y;

public:
	bool	operator<(const DataPoint & rhs) const { return rank < rhs.rank; }
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
static PosType ConvertCoordinateMin(const Coordinate *sortedCoords, int noofCoords, float searchVal)
{
	const Coordinate *found = std::lower_bound(sortedCoords, sortedCoords + noofCoords, searchVal);

	return PosType(found - sortedCoords);
}
	
static PosType ConvertCoordinateMax(const Coordinate *sortedCoords, int noofCoords, float searchVal)
{
	// Get the first point that is strictly greater than the given value, then subtract one to get the last one that should be included in this max
	const Coordinate *found = std::upper_bound(sortedCoords, sortedCoords + noofCoords, searchVal);
	return PosType(found - sortedCoords) - 1;
}
#endif

RectType ConvertRect(const Rect &rectIn, bool & outOfBounds, const Coordinate *xCoordsSorted, const Coordinate *yCoordsSorted, const int noofPoints)
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
	
bool IsInRect(const RectType & rect, const DataPoint & point)
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
		delete[] m_coldData;
		delete[] m_resultBuffer;
		delete[] m_untestedResults;
	}

	virtual void Create(const Point* points_begin, const Point* points_end) override
	{
		m_noofPoints = (int)(points_end - points_begin);
		m_dataPoints = new DataPoint[m_noofPoints];	
		m_coldData = new ColdData[m_noofPoints];
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
		m_xCoordsSorted = new Coordinate[m_noofPoints];
		m_yCoordsSorted = new Coordinate[m_noofPoints];
#else
		m_xCoordsSorted = nullptr;
		m_yCoordsSorted = nullptr;
#endif
		DataPoint *pointsOutIter = const_cast<DataPoint *>(m_dataPoints);
		const Point *pointsInIter = points_begin;
		for (int pid = 0; pid < m_noofPoints; ++pid, ++pointsOutIter, ++pointsInIter)
		{
			RankType rank = pointsInIter->rank;
			pointsOutIter->rank = rank;
#ifndef USE_INT_POSITIONS
			pointsOutIter->x = pointsInIter->x;
			pointsOutIter->y = pointsInIter->y;
#endif
			
			m_coldData[rank].id = pointsInIter->id;
			m_coldData[rank].x = pointsInIter->x;
			m_coldData[rank].y = pointsInIter->y;
			
#ifdef USE_INT_POSITIONS
			m_xCoordsSorted[rank].rank = rank;
			m_xCoordsSorted[rank].pos = pointsInIter->x;
			
			m_yCoordsSorted[rank].rank = rank;
			m_yCoordsSorted[rank].pos = pointsInIter->y;
#endif
		}
		
		DataPoint * pointsNonConst = const_cast<DataPoint *>(m_dataPoints);
		
		// Sort points by rank
		std::sort(pointsNonConst, pointsNonConst + m_noofPoints);
		//QuickSort(pointsNonConst, m_noofPoints);

#ifdef USE_INT_POSITIONS
		// Sort points by X, and then assign a unique integer to each floating point x coordinate, 
		//   so the positions can be dealt with as ints from then on
		std::sort(m_xCoordsSorted, m_xCoordsSorted + m_noofPoints);
		std::sort(m_yCoordsSorted, m_yCoordsSorted + m_noofPoints);
		// Look up the data points by rank now they have been sorted, and give them an integer position
		for (int coordID = 0; coordID < m_noofPoints; ++coordID)
		{
			pointsNonConst[m_xCoordsSorted[coordID].rank].x = coordID;
			pointsNonConst[m_yCoordsSorted[coordID].rank].y = coordID;
		}
#endif
	}

	void ProduceOutputData(int noofResults, const RankType *resultRanks, Point* pointsOut)
	{
		Point *pointsOutIter = pointsOut;
		const RankType *ranksIter = resultRanks;
		for (int pid = 0; pid < noofResults; ++pid, ++pointsOutIter)
		{
			RankType rank = *ranksIter++;
			pointsOutIter->rank = rank;
			pointsOutIter->id = m_coldData[rank].id;
			pointsOutIter->x = m_coldData[rank].x;
			pointsOutIter->y = m_coldData[rank].y;
		}
	}

protected:
	const DataPoint *	m_dataPoints;
	ColdData *				m_coldData;
	RankType *				m_resultBuffer;
	RankType *				m_untestedResults;
	Coordinate *			m_xCoordsSorted;
	Coordinate *			m_yCoordsSorted;
	int						m_noofPoints;
};