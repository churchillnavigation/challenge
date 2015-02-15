#ifndef SURFACES_H
#define SURFACES_H

#include <memory>

#include "common.h"
#include "iterators.h"
#include "bit_sorting.h"

struct Surface;
typedef std::shared_ptr<Surface> SurfacePtr;
SurfacePtr surface_factory(const Mappings& mappings, Ranks& points, int depth, bool x_tainted, bool y_tainted);

struct Surface
{
	const int depth;
	Rect area_rect;

	float cache_area_spanned;
	float cache_quarter_area_spanned;

	Surface(const Mappings& mappings, Ranks& points, int depth)
		: depth(depth)
	{
		Rect area;
		area.lx = std::numeric_limits<float>::max();
		area.hx = -std::numeric_limits<float>::max();
		area.ly = std::numeric_limits<float>::max();
		area.hy = -std::numeric_limits<float>::max();
		for (auto const r : points)
		{
			auto x = mappings.coordinates.xs[r];
			auto y = mappings.coordinates.ys[r];
			area.lx = std::min(area.lx, x);
			area.hx = std::max(area.hx, x);
			area.ly = std::min(area.ly, y);
			area.hy = std::max(area.hy, y);
		}
		set_area(area);
	}

	void set_area(Rect area)
	{
		area_rect = area;

		auto area_value = (area.hx - area.lx)*(area.hy - area.ly);
		cache_area_spanned = area_value * UseBecauseAreaSpanned;
		cache_quarter_area_spanned = area_value * (UseBecauseAreaSpanned / 4.f);
	}

	virtual PointIterator* get_iterator(const Rect& rect) = 0;
};


struct SimpleSurface
	: Surface
{
	Ranks points;
	Coordinates coordinates;

	std::shared_ptr<RanksIterator> all_iterator;
	SimpleIteratorSSE2 simple_iterator;
	//SimpleIteratorAVX2 simple_iterator;

	SimpleSurface(const Mappings& mappings, Ranks& points, std::shared_ptr<RanksIterator> all_iterator, int depth)
		: Surface(mappings, points, depth)
		, all_iterator(all_iterator)
		, simple_iterator(this->coordinates, this->points)
	{
		auto count = points.size();
		coordinates.xs.resize(count);
		coordinates.ys.resize(count);
		for (auto i = 0; i < count; ++i)
		{
			coordinates.xs[i] = mappings.coordinates.xs[points[i]];
			coordinates.ys[i] = mappings.coordinates.ys[points[i]];
		}

		points.shrink_to_fit();
		this->points.swap(points);
	}

	PointIterator* get_iterator(const Rect& rect)
	{
		if (rect.lx > area_rect.hx || rect.hx < area_rect.lx || rect.ly > area_rect.hy || rect.hy < area_rect.ly)
			return &(null_iterator);

		if (rect.lx <= area_rect.lx && rect.hx >= area_rect.hx && rect.ly <= area_rect.ly && rect.hy >= area_rect.hy)
		{
			all_iterator->reset();
			return all_iterator.get();
		}

		simple_iterator.reset(area_rect, rect);
		return &(simple_iterator);
	}
};

struct CompositeSurface
	: Surface
{
	SurfacePtr lower;
	SurfacePtr upper;

	std::shared_ptr<RanksIterator> all_iterator;
	std::shared_ptr<SimpleRankIterator> simple_iterator;
	CompositeIterator composite_iterator;

	CompositeSurface(const Mappings& mappings, Ranks& points, std::shared_ptr<RanksIterator> all_iterator, SurfacePtr lower, SurfacePtr upper, int depth)
		: Surface(mappings, points, depth)
		, all_iterator(all_iterator)
		, lower(lower)
		, upper(upper)
	{
		simple_iterator = std::shared_ptr<SimpleRankIterator>(new SimpleRankIterator(mappings.coordinates, all_iterator));
	}

	virtual PointIterator* get_iterator_directional(const Rect& rect) = 0;

	PointIterator* get_iterator(const Rect& rect)
	{
		if (rect.lx > area_rect.hx || rect.hx < area_rect.lx || rect.ly > area_rect.hy || rect.hy < area_rect.ly)
			return &(null_iterator);

		if (rect.lx <= area_rect.lx && rect.hx >= area_rect.hx && rect.ly <= area_rect.ly && rect.hy >= area_rect.hy)
		{
			all_iterator->reset();
			return all_iterator.get();
		}

		auto directional_iterator = get_iterator_directional(rect);
		if (directional_iterator)
			return directional_iterator;

		auto lower_common_area = (std::min(rect.hx, lower->area_rect.hx) - std::max(rect.lx, lower->area_rect.lx)) * ((std::min(rect.hy, lower->area_rect.hy) - std::max(rect.ly, lower->area_rect.ly)));
		if (lower_common_area > lower->cache_quarter_area_spanned)
		{
			auto upper_common_area = (std::min(rect.hx, upper->area_rect.hx) - std::max(rect.lx, upper->area_rect.lx)) * ((std::min(rect.hy, upper->area_rect.hy) - std::max(rect.ly, upper->area_rect.ly)));
			if (upper_common_area > upper->cache_quarter_area_spanned)
			{
				auto common_area = (std::min(rect.hx, area_rect.hx) - std::max(rect.lx, area_rect.lx)) * ((std::min(rect.hy, area_rect.hy) - std::max(rect.ly, area_rect.ly)));
				if (common_area > cache_area_spanned)
				{
					simple_iterator->reset(area_rect, rect);
					return simple_iterator.get();
				}
			}
		}

		auto lower_iterator = lower->get_iterator(rect);
		auto upper_iterator = upper->get_iterator(rect);
		if (lower_iterator == &(null_iterator))
			return upper_iterator;
		if (upper_iterator == &(null_iterator))
			return lower_iterator;
		composite_iterator.reset(lower_iterator, upper_iterator);
		return &(composite_iterator);
	}
};

struct CompositeSurfaceX
	: CompositeSurface
{
	CompositeSurfaceX(const Mappings& mappings, Ranks& points, std::shared_ptr<RanksIterator> all_iterator, SurfacePtr lower, SurfacePtr upper, int depth)
		: CompositeSurface(mappings, points, all_iterator, lower, upper, depth)
	{}

	PointIterator* get_iterator_directional(const Rect& rect)
	{
		if (rect.hx <= lower->area_rect.hx)
			return lower->get_iterator(rect);
		if (rect.lx >= upper->area_rect.lx)
			return upper->get_iterator(rect);
		return nullptr;
	}
};

struct CompositeSurfaceY
	: CompositeSurface
{
	CompositeSurfaceY(const Mappings& mappings, Ranks& points, std::shared_ptr<RanksIterator> all_iterator, SurfacePtr lower, SurfacePtr upper, int depth)
		: CompositeSurface(mappings, points, all_iterator, lower, upper, depth)
	{}

	PointIterator* get_iterator_directional(const Rect& rect)
	{
		if (rect.hy <= lower->area_rect.hy)
			return lower->get_iterator(rect);
		if (rect.ly >= upper->area_rect.ly)
			return upper->get_iterator(rect);
		return nullptr;
	}
};

void sort_by_x(const Mappings& mappings, Ranks& points)
{
	if (points.size() > MinimumSizeToUseMappingSort)
		sort_by_coordinate(mappings.points_count, points, mappings.rank_idx_to_x_sorted_idxes, mappings.x_sorted_rank_idxes);
	else
		sort(points.begin(), points.end(), [&mappings](int32_t a, const int32_t b){ return mappings.coordinates.xs[a] < mappings.coordinates.xs[b]; });
}

void sort_by_y(const Mappings& mappings, Ranks& points)
{
	if (points.size() > MinimumSizeToUseMappingSort)
		sort_by_coordinate(mappings.points_count, points, mappings.rank_idx_to_y_sorted_idxes, mappings.y_sorted_rank_idxes);
	else
		sort(points.begin(), points.end(), [&mappings](int32_t a, const int32_t b){ return mappings.coordinates.ys[a] < mappings.coordinates.ys[b]; });
}

int get_mid_point(const Ranks& points, const Floats& values)
{
	for (auto i = 0; i < (int)points.size() - 1; ++i)
	{
		auto mid_point = (int)points.size() / 2 + ((i % 2 ? 1 : -1) * (i + 1) / 2);
		if (values[points[mid_point]] != values[points[mid_point - 1]])
			return mid_point;
	}
	return -1;
}

std::tuple<float, int> get_gap(const Ranks& points, const Floats& values)
{
	auto max_gap = -std::numeric_limits<float>::max();
	auto max_gap_idx = -1;

	auto min_value = std::numeric_limits<float>::max();
	auto max_value = -std::numeric_limits<float>::max();
	if (abs(values[points[0]]) < 1e10)
	{
		min_value = values[points[0]];
		max_value = values[points[0]];
	}

	for (auto i = 1; i < points.size(); ++i)
	{
		auto n2 = values[points[i]];
		if (abs(n2) >= 1e10)
			continue;

		min_value = std::min(min_value, n2);
		max_value = std::max(max_value, n2);

		auto n1 = values[points[i - 1]];
		if (abs(n1) >= 1e10)
			continue;

		auto gap = n2 - n1;
		if (gap > max_gap)
		{
			max_gap = gap;
			max_gap_idx = i;
		}
	}

	auto gap_fraction = (max_gap / (max_value - min_value));

	return std::tuple<float, int>(gap_fraction, max_gap_idx);
}

void copy_split_points(const Mappings& mappings, const Ranks& points, int midPoint, Ranks& lower, Ranks& upper)
{
	lower.resize(midPoint);
	copy(points.begin(), points.begin() + midPoint, lower.begin());

	upper.resize(points.size() - midPoint);
	copy(points.begin() + midPoint, points.end(), upper.begin());
}

std::tuple<SurfacePtr, SurfacePtr> create_sub_surfaces(const Mappings& mappings, Ranks& points, int mid_point, int depth, bool x_tainted, bool y_tainted)
{
	Ranks lower_points;
	Ranks upper_points;
	copy_split_points(mappings, points, mid_point, lower_points, upper_points);

	auto make_lower = [&](){return surface_factory(mappings, lower_points, depth + 1, x_tainted, y_tainted); };
	auto make_upper = [&](){return surface_factory(mappings, upper_points, depth + 1, x_tainted, y_tainted); };

	SurfacePtr lower;
	SurfacePtr upper;

	if (depth < 2)
	{
		auto task_lower = concurrency::create_task(make_lower);
		auto task_upper = concurrency::create_task(make_upper);

		lower = task_lower.get();
		upper = task_upper.get();
	}
	else
	{
		lower = make_lower();
		upper = make_upper();
	}

	return std::tuple<SurfacePtr, SurfacePtr>(lower, upper);
}

SurfacePtr surface_factory(const Mappings& mappings, Ranks& points, int depth, bool x_tainted, bool y_tainted)
{
	if (points.size() < PointCountThreshold)
	{
		auto all_iterator = ranks_iterator_factory(mappings.points_count, points, depth);
		return SurfacePtr(new SimpleSurface(mappings, points, all_iterator, depth));
	}

	if (!x_tainted && (y_tainted || depth % 2 == 0))
	{
		sort_by_x(mappings, points);

		auto xgap = get_gap(points, mappings.coordinates.xs);

		int mid_point;
		if (std::get<0>(xgap) > MaxGapFraction)
			mid_point = std::get<1>(xgap);
		else
			mid_point = get_mid_point(points, mappings.coordinates.xs);

		if (mid_point == -1)
			return surface_factory(mappings, points, depth, true, y_tainted);

		auto sub_surfaces = create_sub_surfaces(mappings, points, mid_point, depth, x_tainted, y_tainted);

		auto all_iterator = ranks_iterator_factory(mappings.points_count, points, depth);
		return SurfacePtr(new CompositeSurfaceX(mappings, points, all_iterator, std::get<0>(sub_surfaces), std::get<1>(sub_surfaces), depth));
	}
	else if (!y_tainted)
	{
		sort_by_y(mappings, points);

		auto ygap = get_gap(points, mappings.coordinates.ys);

		int mid_point;
		if (std::get<0>(ygap) > MaxGapFraction)
			mid_point = std::get<1>(ygap);
		else
			mid_point = get_mid_point(points, mappings.coordinates.ys);

		if (mid_point == -1)
			return surface_factory(mappings, points, depth, x_tainted, true);

		auto sub_surfaces = create_sub_surfaces(mappings, points, mid_point, depth, x_tainted, y_tainted);

		auto all_iterator = ranks_iterator_factory(mappings.points_count, points, depth);
		return SurfacePtr(new CompositeSurfaceY(mappings, points, all_iterator, std::get<0>(sub_surfaces), std::get<1>(sub_surfaces), depth));
	}
	else
	{
		auto all_iterator = ranks_iterator_factory(mappings.points_count, points, depth);
		return SurfacePtr(new SimpleSurface(mappings, points, all_iterator, depth));
	}
}


#endif