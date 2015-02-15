//Brian Thorpe 2015
#include "stdafx.h"
#include "QueryRect.h"
QueryRect::QueryRect(float lx, float ly, float hx, float hy)
{
	m_LX = lx;
	m_LY = ly;
	m_HX = hx;
	m_HY = hy;
}

QueryRect::QueryRect(const Rect r) 
{
	m_LX = r.lx;
	m_LY = r.ly;
	m_HX = r.hx;
	m_HY = r.hy;
}

QueryRect QueryRect::NormalizedRectFromRect(Rect r, float x_min, float x_range, float y_min, float y_range, float scaling)
{	
	float lx = max(((r.lx - x_min) / (x_range)) * scaling, 0.0f);
	float ly = max(((r.ly - y_min) / (y_range)) * scaling, 0.0f);
	float hx = min(((r.hx - x_min) / (x_range)) * scaling, scaling);
	float hy = min(((r.hy - y_min) / (y_range)) * scaling, scaling);
	return QueryRect(lx,ly,hx,hy);
}

bool QueryRect::RectContainsPoint(float lx, float ly, float hx, float hy, float px, float py)
{
	if (px >= lx && px <= hx && py >= ly && py <= hy)
		return true;
	return false;
}

bool QueryRect::ContainsPoint(float x, float y) const
{
	if (x >= m_LX && x <= m_HX && y >= m_LY && y <= m_HY)
		return true;
	return false;
}

bool QueryRect::ContainsRect(float lx, float ly, float hx, float hy) const
{
	if(lx >= m_LX && hx <= m_HX && ly >= m_LY && hy <= m_HY)
	{
		return true;
	}
	return false;
}

bool QueryRect::IntersectsRect(float lx, float ly, float hx, float hy) const
{
	if ( lx > m_HX || ly > m_HY|| m_LX > hx || m_LY > hy)
	{
		return false;
	}
	return true;
}