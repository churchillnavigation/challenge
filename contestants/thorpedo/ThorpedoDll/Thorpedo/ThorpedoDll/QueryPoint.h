//Brian Thorpe 2015
#ifndef QUERY_POINT_H
#define QUERY_POINT_H
#include "CommonStructs.h"

class QueryPoint {
public:
	int8_t m_Id;
	int32_t m_Rank;
	float m_X;
	float m_Y;
	uint32_t m_NX;
	uint32_t m_NY;
	QueryPoint() : m_Id(0), m_Rank(0), m_X(0), m_Y(0), m_NX(0), m_NY(0) {}
	QueryPoint(const Point* p, float x_min, float x_range, float y_min, float y_range);
};

#endif