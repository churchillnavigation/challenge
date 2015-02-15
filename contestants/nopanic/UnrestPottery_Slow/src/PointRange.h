#include "point_search.h"
#include <vector>

class PointRange
{
public:
	PointRange(const Point *begin, const Point *end);
	
	Rect span() const;
	
	Rect small_span() const;
	
	int outside() const;
	
	int size() const;

	const Point& operator[](int i) const;

private:
	const Point *points;
	std::vector<Point> sorted_points;
	int size_;
	int outside_;
	Rect span_;
	Rect small_span_;
};
