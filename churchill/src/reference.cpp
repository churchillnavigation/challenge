#include "point_search.h"
#include <algorithm>
#include <vector>

struct SearchContext {
	std::vector<Point> m_points; // store all the points in a vector
};

extern "C" SearchContext* __stdcall create(const Point* points_begin, const Point* points_end) {
	SearchContext* o = new SearchContext; // create context
	o->m_points.assign(points_begin, points_end); // make copy of the points
	std::sort(o->m_points.begin(), o->m_points.end(), [](const Point& a, const Point& b) { return a.rank < b.rank; }); // then sort them by rank
	return o; // return the context pointer
}

extern "C" int32_t __stdcall search(SearchContext* o, const Rect rect, const int32_t count, Point* out_points) {
	int out_count = 0; // the number of points found
	if (o == nullptr) { return out_count; }
	const auto end = o->m_points.end();
	for(auto i = o->m_points.begin(); i != end; ++i) { // loop through the points
		if( (i->x < rect.lx) || (i->x > rect.hx) || (i->y < rect.ly) || (i->y > rect.hy) ) { continue; } // if point is outside the rectangle, then skip
		out_points[out_count++] = *i; // otherwise add to results
		if( out_count == count ) { break; } // if we found enough, then exit
	}
	return out_count;
}

extern "C" SearchContext* __stdcall destroy(SearchContext* o) {
	delete o; // release context
	return nullptr;
}