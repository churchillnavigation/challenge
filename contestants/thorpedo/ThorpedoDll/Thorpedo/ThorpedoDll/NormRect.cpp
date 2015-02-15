//Brian Thorpe 2015
#include "stdafx.h"
#include "NormRect.h"
NormRect::NormRect(uint32_t lx, uint32_t ly, uint32_t hx, uint32_t hy)
{
	m_LX = lx;
	m_LY = ly;
	m_HX = hx;
	m_HY = hy;
}

NormRect NormRect::NormalizedRectFromRect(Rect r, float x_min, float x_range, float y_min, float y_range)
{
	uint32_t lx = (uint32_t)(max((r.lx - x_min) / (x_range),0.0f) * FMAX_TREE_SIZE);
	uint32_t ly = (uint32_t)(max((r.ly - y_min) / (y_range),0.0f) * FMAX_TREE_SIZE);
	uint32_t hx = (uint32_t)(min((r.hx - x_min) / (x_range),1.0f) * FMAX_TREE_SIZE);
	uint32_t hy = (uint32_t)(min((r.hy - y_min) / (y_range),1.0f) * FMAX_TREE_SIZE);
	return NormRect(lx,ly,hx,hy);
}

bool NormRect::RectContainsPoint(uint32_t lx, uint32_t ly, uint32_t hx, uint32_t hy, uint32_t px, uint32_t py)
{
	if (px >= lx && px <= hx && py >= ly && py <= hy)
		return true;
	return false;
}

bool NormRect::ContainsPoint(uint32_t x, uint32_t y) const
{
	if (x >= m_LX && x <= m_HX && y >= m_LY && y <= m_HY)
		return true;
	return false;
}

bool NormRect::ContainsRect(uint32_t lx, uint32_t ly, uint32_t hx, uint32_t hy) const
{
	if(lx >= m_LX && hx <= m_HX && ly >= m_LY && hy <= m_HY)
	{
		return true;
	}
	return false;
}

bool NormRect::IntersectsRect(uint32_t lx, uint32_t ly, uint32_t hx, uint32_t hy) const
{
	if ( lx > m_HX || ly > m_HY|| m_LX > hx || m_LY > hy)
	{
		return false;
	}
	return true;
}