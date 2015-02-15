#include "point_search.h"
#include <iterator>
#include <vector>
#include <algorithm>
#include <tuple>
#include <memory>

typedef std::vector<int8_t>  Ids;
typedef std::vector<int32_t> Ranks;
typedef std::vector<float>   Floats;
typedef std::vector<Point>   Points;

typedef std::unique_ptr<Ids>    IdsPtr;
typedef std::unique_ptr<Ranks>  RanksPtr;
typedef std::unique_ptr<Floats> FloatsPtr;
typedef std::unique_ptr<Points> PointsPtr;

typedef std::tuple<IdsPtr, RanksPtr, FloatsPtr, FloatsPtr> FlattenedPoints;

struct SearchContext
{
	SearchContext(const Point* points_begin, const Point* points_end);
	int32_t search(const Rect& rect, const int32_t count, Point* out_points);

	bool good;

	std::tuple<float, float, FlattenedPoints>  a;
	std::tuple<float, float, FlattenedPoints> b1;
	std::tuple<float, float, FlattenedPoints> b2;
	std::tuple<float, float, FlattenedPoints> c1;
	std::tuple<float, float, FlattenedPoints> c2;
	std::tuple<float, float, FlattenedPoints> c3;
	std::tuple<float, float, FlattenedPoints> c4;
	std::tuple<float, float, FlattenedPoints> d1;
	std::tuple<float, float, FlattenedPoints> d2;
	std::tuple<float, float, FlattenedPoints> d3;
	std::tuple<float, float, FlattenedPoints> d4;
	std::tuple<float, float, FlattenedPoints> d5;
	std::tuple<float, float, FlattenedPoints> d6;
	std::tuple<float, float, FlattenedPoints> d7;
	std::tuple<float, float, FlattenedPoints> d8;
};

std::tuple<float, float, FlattenedPoints> create_flattened(const Point* points_begin, const Point* points_end)
{
	auto rankedAll = PointsPtr(new Points(points_begin, points_end));
	sort(begin(*rankedAll), end(*rankedAll), [](const Point& a, const Point& b){ return a.rank < b.rank; });

	auto size = rankedAll->size();

	auto ids   = IdsPtr   (new Ids   (size));
	auto ranks = RanksPtr (new Ranks (size));
	auto xs    = FloatsPtr(new Floats(size));
	auto ys    = FloatsPtr(new Floats(size));

	for (auto i = 0; i < size; ++i)
	{
		(*ids)  [i] = (*rankedAll)[i].id;
		(*ranks)[i] = (*rankedAll)[i].rank;
		(*xs)   [i] = (*rankedAll)[i].x;
		(*ys)   [i] = (*rankedAll)[i].y;
	}

	return std::tuple<float, float, FlattenedPoints>(points_begin->x, (points_end-1)->x, FlattenedPoints(IdsPtr(ids.release()), RanksPtr(ranks.release()), FloatsPtr(xs.release()), FloatsPtr(ys.release())));
}

SearchContext::SearchContext(const Point* points_begin, const Point* points_end)
{
	if (points_begin == points_end)
	{
		good = false;
		return;
	}
	good = true;

	auto x_sorted = PointsPtr(new Points(points_begin, points_end));
	sort(begin(*x_sorted), end(*x_sorted), [](const Point& a, const Point& b){ return a.x < b.x; });

	auto point_count = x_sorted->size();

	auto one_eighth_count   = point_count / 8 * 1;
	auto two_eights_count   = point_count / 8 * 2;
	auto three_eighth_count = point_count / 8 * 3;
	auto four_eights_count  = point_count / 8 * 4;
	auto five_eighth_count  = point_count / 8 * 5;
	auto six_eights_count   = point_count / 8 * 6;
	auto seven_eighth_count = point_count / 8 * 7;

	auto zero_eighth  = &(*(x_sorted.get()))[0];
	auto one_eighth   = zero_eighth + one_eighth_count;
	auto two_eights   = zero_eighth + two_eights_count;
	auto three_eighth = zero_eighth + three_eighth_count;
	auto four_eights  = zero_eighth + four_eights_count;
	auto five_eighth  = zero_eighth + five_eighth_count;
	auto six_eights   = zero_eighth + six_eights_count;
	auto seven_eighth = zero_eighth + seven_eighth_count;
	auto eight_eighth = zero_eighth + point_count;

	a = create_flattened(zero_eighth, eight_eighth);

	b1 = create_flattened(zero_eighth, four_eights);
	b2 = create_flattened(four_eights, eight_eighth);

	c1 = create_flattened(zero_eighth, two_eights);
	c2 = create_flattened(two_eights,  four_eights);
	c3 = create_flattened(four_eights, six_eights);
	c4 = create_flattened(six_eights,  eight_eighth);

	d1 = create_flattened(zero_eighth,  one_eighth);
	d2 = create_flattened(one_eighth,   two_eights);
	d3 = create_flattened(two_eights,   three_eighth);
	d4 = create_flattened(three_eighth, four_eights);
	d5 = create_flattened(four_eights,  five_eighth);
	d6 = create_flattened(five_eighth,  six_eights);
	d7 = create_flattened(six_eights,   seven_eighth);
	d8 = create_flattened(seven_eighth, eight_eighth);
}

void create_point(Point* pt, int8_t* ids, int32_t* ranks, float* xs, float* ys, Points::size_type idx)
{
	pt->id   = ids[idx];
	pt->rank = ranks[idx];
	pt->x    = xs[idx];
	pt->y    = ys[idx];
}

int32_t searchy(const Rect& r, const int32_t count, Point* out_points, const FlattenedPoints& points)
{
	using namespace std;

	auto& ids_   = get<0>(points);
	auto& ranks_ = get<1>(points);
	auto& xs_    = get<2>(points);
	auto& ys_    = get<3>(points);

	auto pointsCounts = (*ids_).size();
	if (pointsCounts == 0)
		return 0;

	int8_t*  ids   = &(*(ids_  .get()))[0];
	int32_t* ranks = &(*(ranks_.get()))[0];
	float*   xs    = &(*(xs_   .get()))[0];
	float*   ys    = &(*(ys_   .get()))[0];

	int32_t result_count = 0;

	register auto rect = r;
	for (Points::size_type i = 0; i < pointsCounts; ++i)
	{
		if (rect.ly <= ys[i] && rect.hy >= ys[i] && rect.lx <= xs[i] && rect.hx >= xs[i])
		{
			create_point(out_points++, ids, ranks, xs, ys, i);
			if (++result_count == count)
				break;
		}
	}
	return result_count;
}

int32_t SearchContext::search(const Rect& rect, const int32_t count, Point* out_points)
{
	using namespace std;

	if (!good)
		return 0;

	if (rect.hx < get<1>(b1))
	{
		if (rect.hx <= get<1>(c1))
		{
			if (rect.hx <= get<1>(d1))
				return searchy(rect, count, out_points, get<2>(d1));
			if (rect.lx > get<1>(d1))
				return searchy(rect, count, out_points, get<2>(d2));
			return searchy(rect, count, out_points, get<2>(c1));
		}
		if (rect.lx > get<1>(c1))
		{
			if (rect.hx <= get<1>(d3))
				return searchy(rect, count, out_points, get<2>(d3));
			if (rect.lx > get<1>(d3))
				return searchy(rect, count, out_points, get<2>(d4));
			return searchy(rect, count, out_points, get<2>(c2));
		}
		return searchy(rect, count, out_points, get<2>(b1));
	}
	if (rect.lx > get<1>(b1))
	{
		if (rect.hx <= get<1>(c3))
		{
			if (rect.hx <= get<1>(d5))
				return searchy(rect, count, out_points, get<2>(d5));
			if (rect.lx > get<1>(d5))
				return searchy(rect, count, out_points, get<2>(d6));
			return searchy(rect, count, out_points, get<2>(c3));
		}
		if (rect.lx > get<1>(c3))
		{
			if (rect.hx <= get<1>(d7))
				return searchy(rect, count, out_points, get<2>(d7));
			if (rect.lx > get<1>(d7))
				return searchy(rect, count, out_points, get<2>(d8));
			return searchy(rect, count, out_points, get<2>(c4));
		}
		return searchy(rect, count, out_points, get<2>(b2));
	}

	return searchy(rect, count, out_points, get<2>(a));
}

extern "C" __declspec(dllexport)
SearchContext* __stdcall
create(const Point* points_begin, const Point* points_end)
{
	return new SearchContext(points_begin, points_end);
}

extern "C" __declspec(dllexport)
int32_t __stdcall
search(SearchContext* sc, const Rect rect, const int32_t count, Point* out_points)
{
	return sc->search(rect, count, out_points);
}

extern "C" __declspec(dllexport)
SearchContext* __stdcall
destroy(SearchContext* sc)
{
	delete sc;
	return nullptr;
}