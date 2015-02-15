extern "C" {
#include "point_search.h"
}

#include <algorithm>
#include <vector>

extern "C" {
	__declspec(dllexport) SearchContext* __stdcall create(const Point* points_begin, const Point* points_end);
	__declspec(dllexport) SearchContext* __stdcall destroy(SearchContext* sc);
	__declspec(dllexport) int32_t __stdcall search(SearchContext* sc, const Rect rect, const int32_t count, Point* out_points);
}

static bool predicate_rank(const Point& a, const Point& b) {
	return (a.rank < b.rank);
}

struct SearchContext {
	std::vector<Point> points_ranked;

	SearchContext(const Point* begin, const Point* end) {
		if (begin == nullptr || end == nullptr) {
			return;
		}

		points_ranked.reserve(end - begin);

		for (; begin != end; ++begin) {
			points_ranked.push_back(*begin);
		}
		std::sort(points_ranked.begin(), points_ranked.end(), predicate_rank);
	}

	inline int32_t search_ranked(const Rect rect, const int32_t count, Point* output) {
		if (output == nullptr) {
			return 0;
		}

		if (count == 0) {
			return 0;
		}

		int32_t points_found = 0;
		// p should be a reference, if you want speed
		for (const auto& p : points_ranked) {
			// single & are intentional, allow parallelization of inexpensive comparisons
			if ((p.x >= rect.lx) & (p.y >= rect.ly) & (p.x <= rect.hx) & (p.y <= rect.hy)) {
				*output = p;
				output += 1;
				points_found += 1;
				if (points_found == count) {
					break;
				}
			}
		}

		return points_found;
	}

};

SearchContext* __stdcall create(const Point* points_begin, const Point* points_end) {
	return new SearchContext(points_begin, points_end);
}

SearchContext* __stdcall destroy(SearchContext* sc) {
	delete sc;
	return nullptr;
}

int32_t __stdcall search(SearchContext* sc, const Rect rect, const int32_t count, Point* out_points) {
	return sc->search_ranked(rect, count, out_points);
}
