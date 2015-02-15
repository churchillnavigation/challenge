#pragma once

#include <queue>
#include <vector>

struct Point
{
public:
	int id;
	int rank;
	float x;
	float y;

	Point(){ };

	Point(const int id, const int rank, const float x, const float y) {
		this->id = id;
		this->rank = rank;
		this->x = x;
		this->y = y;
	}

	inline bool operator < (const Point & point) const
	{
		return (rank < point.rank);
	}

	inline bool operator >(const Point & point) const
	{
		return (rank > point.rank);
	}
};

class PointRankCompare
{
public:
	inline bool operator() (const Point* & pointA, const Point* & pointB) const {
		return pointA->rank > pointB->rank;
	}

	inline bool operator() (const Point & pointA, const Point & pointB) const {
		return pointA.rank > pointB.rank;
	}
};

const PointRankCompare PointRankCompareInstance = PointRankCompare();


#define PointQueue std::priority_queue<Point, std::vector<Point>, std::greater<Point>>
#define PointQueueInverted std::priority_queue<Point, std::vector<Point>, std::less<Point>>


