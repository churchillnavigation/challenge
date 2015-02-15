#include <iostream>
#include <vector>
#include <functional>
#include <chrono>
#include "ppl.h"
#include "ppltasks.h"

#include "common.h"
#include "bitset.h"
#include "surfaces.h"

struct SearchContext
{
	SurfacePtr top;
	Coordinates coordinates;
	Ids ids;
	std::vector<Point> ordered_points;

	SearchContext(const Point* points_begin, const Point* points_end)
	{
		auto start_time = std::chrono::system_clock::now();

		auto objects = points_end - points_begin;
		if (objects > std::numeric_limits<int32_t>::max())
			return;

		auto points_count = (int32_t)objects;
		coordinates.xs.resize(points_count);
		coordinates.ys.resize(points_count);
		ids.resize(points_count);
		ordered_points.resize(points_count);

		auto ranks = Ranks(points_count, END_OF_DATA);
		auto points = std::vector<Point>(points_count);
		for (auto it = points_begin; it != points_end; ++it)
		{
			auto idx = it->rank;
			if (idx < 0 || idx >= points_count || ranks[idx] != END_OF_DATA)
				return;
			coordinates.xs[idx] = it->x;
			coordinates.ys[idx] = it->y;
			ids[idx] = it->id;
			ranks[idx] = idx;
			points[idx] = *it;
			ordered_points[idx] = *it;
		}
		Mappings mappings(points_count, coordinates, ids);

		concurrency::parallel_sort(points.begin(), points.end(), [](const Point& a, const Point& b) { return a.x < b.x; });
		for (auto i = 0; i < points_count; ++i)
		{
			mappings.x_sorted_rank_idxes[i] = points[i].rank;
			mappings.rank_idx_to_x_sorted_idxes[points[i].rank] = i;
		}

		concurrency::parallel_sort(points.begin(), points.end(), [](const Point& a, const Point& b) { return a.y < b.y; });
		for (auto i = 0; i < points_count; ++i)
		{
			mappings.y_sorted_rank_idxes[i] = points[i].rank;
			mappings.rank_idx_to_y_sorted_idxes[points[i].rank] = i;
		}

		top = surface_factory(mappings, ranks, 0, false, false);
	}

	int32_t search(const Rect& rect, const int32_t count, Point* out_points) const
	{
		auto found_count = 0;
		auto iterator = top->get_iterator(rect);

		auto rank_buffer = (int32_t*)alloca(sizeof(int32_t)*count);

		for (auto p = iterator->iterate(); p != END_OF_DATA; p = iterator->iterate())
		{
			*(rank_buffer + found_count) = p;
			if (++found_count == count)
				break;
		}

		for (auto i = 0; i < found_count; ++i)
			*(out_points + i) = ordered_points[rank_buffer[i]];

		return found_count;
	}
};

extern "C" __declspec(dllexport) SearchContext* __stdcall create(const Point* points_begin, const Point* points_end)
{
	return new SearchContext(points_begin, points_end);
}

extern "C" __declspec(dllexport) int32_t __stdcall search(SearchContext* sc, const Rect rect, const int32_t count, Point* out_points)
{
	return sc->search(rect, count, out_points);
}

extern "C" __declspec(dllexport) SearchContext* __stdcall destroy(SearchContext* sc)
{
	delete sc;
	return nullptr;
}

