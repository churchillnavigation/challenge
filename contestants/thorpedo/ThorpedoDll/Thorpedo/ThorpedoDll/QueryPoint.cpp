//Brian Thorpe 2015
#include "stdafx.h"
#include "QueryPoint.h"

QueryPoint::QueryPoint(const Point* p, float x_min, float x_range, float y_min, float y_range) : m_Id(p->id), m_Rank(p->rank), m_X(p->x), m_Y(p->y), 
	m_NX((uint32_t)(((p->x - x_min) / (x_range)) * FMAX_TREE_SIZE)),
	m_NY((uint32_t)(((p->y - y_min) / (y_range)) * FMAX_TREE_SIZE))
{
}