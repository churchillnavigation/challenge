#ifndef __POINTRANGE_H
#define __POINTRANGE_H

#include "point_search.h"
#include <cstddef>
#include <vector>

class PointRange
{
public:
	PointRange(const Point *begin, const Point *end);
	
	Rect span() const;
	
	Rect small_span() const;
	
	int outside() const;
	
	size_t size() const;

	const Point& operator[](size_t i) const;

private:
	const Point *points;
	std::vector<Point> sorted_points;
	size_t size_;
	int outside_;
	Rect span_;
	Rect small_span_;
};

#endif
