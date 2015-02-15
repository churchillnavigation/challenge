#include "PointRange.h"
#include <algorithm>
#include <limits>

using namespace std;

inline bool inside(Rect r, Point p)
{
	return (r.lx <= p.x) & (p.x <= r.hx)
	     & (r.ly <= p.y) & (p.y <= r.hy);
}

PointRange::PointRange(const Point *begin, const Point *end):
	outside_(0)
{
	points = begin;
	size_  = end - begin;

	span_.lx = span_.ly = numeric_limits<float>::infinity();
	span_.hx = span_.hy = -numeric_limits<float>::infinity();

	bool sorted = true;
	for (size_t i = 0; i < size_; i++)
	{
		if (i > 0)
		{
			if (points[i].rank < points[i-1].rank)
				sorted = false;
		}
		
		span_.lx = min(span_.lx, points[i].x);
		span_.ly = min(span_.ly, points[i].y);

		span_.hx = max(span_.hx, points[i].x);
		span_.hy = max(span_.hy, points[i].y);
	}
	
	Rect small_span_initial;
	float mx = (span_.lx + span_.hx) * 0.5f;
	float my = (span_.ly + span_.hy) * 0.5f;
	float dx = (span_.hx - span_.lx) * 0.5f;
	float dy = (span_.hy - span_.ly) * 0.5f;
	
	const float SMALL_SPAN = 0.9f;
	small_span_initial.lx = mx - dx * SMALL_SPAN;
	small_span_initial.hx = mx + dx * SMALL_SPAN;
	small_span_initial.ly = my - dy * SMALL_SPAN;
	small_span_initial.hy = my + dy * SMALL_SPAN;

	small_span_.lx = small_span_.ly = numeric_limits<float>::infinity();
	small_span_.hx = small_span_.hy = -numeric_limits<float>::infinity();
	
	for (size_t i = 0; i < size_; i++)
	{
		if (inside(small_span_initial, points[i]))
		{
			small_span_.lx = min(small_span_.lx, points[i].x);
			small_span_.ly = min(small_span_.ly, points[i].y);

			small_span_.hx = max(small_span_.hx, points[i].x);
			small_span_.hy = max(small_span_.hy, points[i].y);
		}
		else
			outside_ ++;
	}
	
	
	if (!sorted)
	{
		sorted_points.insert(sorted_points.begin(), begin, end);
		sort(sorted_points.begin(), sorted_points.end(), [](const Point &a, const Point &b){
			return a.rank < b.rank;
		});
	}
}

Rect PointRange::span() const
{
	return span_;
}

Rect PointRange::small_span() const
{
	return small_span_;
}

size_t PointRange::size() const
{
	return size_;
}

int PointRange::outside() const
{
	return outside_;
}

const Point& PointRange::operator[](size_t i) const
{
	if (sorted_points.empty())
		return points[i];
	else
		return sorted_points[i];
}
