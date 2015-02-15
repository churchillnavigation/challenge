#pragma once

#include "point_search.h"
#include <vector>
#include <algorithm>

// *** tmp ***
#include <fstream>

void rect_set( Rect& rect, const Point& point )
{
	rect.lx = point.x;
	rect.ly = point.y;
   	rect.hx = point.x;
    rect.hy = point.y;
}

void rect_expand( Rect& rect, const Point& point )
{
	if( rect.lx > point.x ) rect.lx = point.x;
	if( rect.ly > point.y ) rect.ly = point.y;
	if( rect.hx < point.x ) rect.hx = point.x;
    if( rect.hy < point.y ) rect.hy = point.y;
}

bool rect_intersect( const Rect& r0, const Rect& r1 )
{
	if( r0.hx < r1.lx || r0.lx > r1.hx )
		return false;

	if( r0.hy < r1.ly || r0.ly > r1.hy )
		return false;

	return true;
}

bool rect_intersect( const Rect& rect, const Point& point )
{
	if( rect.hx < point.x || rect.lx > point.x )
		return false;

	if( rect.hy < point.y || rect.ly > point.y )
		return false;

	return true;
}

bool rect_contains( const Rect& rect, const Rect& c )
{
	if( c.lx < rect.lx || c.hx > rect.hx )
		return false;

	if( c.ly < rect.ly || c.hy > rect.hy )
		return false;

	return true;
}

bool compare_rank(const Point& p0, const Point& p1) { return p0.rank < p1.rank; }
bool compare_x(const Point& p0, const Point& p1) { return p0.x < p1.x; }
bool compare_y(const Point& p0, const Point& p1) { return p0.y < p1.y; }

//for short list
bool list_insort(std::vector<Point>& list, Point& point)
{
	if( list.size() == list.capacity() )
	{
		if( list.back().rank <= point.rank )
			return false;

		list.pop_back();
	}

	list.insert( std::lower_bound(list.begin(), list.end(), point, compare_rank), point);
	
	return true;
}