#pragma once
#include "Point.h"

struct Rect
{
	float lx;
	float hx;
	float ly;
	float hy;


	inline bool Contains(const Point & point) const
	{
		return point.x >= lx && point.x <= hx && point.y >= ly && point.y <= hy;
	}

	inline bool Collides(const Rect & rect) const
	{
		return lx <= rect.hx && hx >= rect.lx && ly <= rect.hy && hy >= rect.ly;
	}

};
