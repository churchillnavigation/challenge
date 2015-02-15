#include "../../point_search.h"
#include <iterator>
#include <vector>
#include <stack>
#include <algorithm>
#include <tuple>
#include <memory>
#include <unordered_set>
#include <chrono>
#include "ppl.h"
#include "ppltasks.h"

#include "bitset.h"

using namespace std;

using concurrency::parallel_sort;
using concurrency::task;
using concurrency::create_task;

struct Coordinate {
	Coordinate() {}
	Coordinate(float x, float y) { this->x = x; this->y = y; }
	float x;
	float y;
};

typedef std::vector<Coordinate> Coordinates;
typedef std::vector<int8_t>		Ids;
typedef std::vector<int32_t>	Ranks;

struct Mappings
{
	Mappings()
		: point_count(0)
	{}

	int32_t point_count;
	Coordinates rank_idx_to_coordinates;
	Ids rank_idx_to_ids;
};

struct SortHelp
{
	SortHelp(int points_count)
		: points_count(points_count) {}

	int points_count;
	vector<int32_t> x_sorted_rank_idxes;
	vector<int32_t> y_sorted_rank_idxes;
	vector<int32_t> rank_idx_to_x_sorted_idxes;
	vector<int32_t> rank_idx_to_y_sorted_idxes;
};

typedef std::vector<Point>   Points;
typedef std::unique_ptr<Points> PointsPtr;

enum RanksBy {
	ByRank,
	ByX,
	ByY,
};

struct OrderedPoints {
private:
	static shared_ptr<Ranks> SortByRank(int point_count, shared_ptr<Ranks> rankCollection)
	{
		BitSet ws(point_count);
		for (auto it : *rankCollection)
			ws.set(it);

		auto output = shared_ptr<Ranks>(new Ranks(rankCollection->size()));
		int idx = 0;
		for (auto it = ws.get_values_iterator(); !it.at_end(); ++it)
			(*output)[idx++] = *it;

		return output;
	}

	static shared_ptr<Ranks> SortByCoordinate(int point_count, shared_ptr<Ranks> rankCollection, const vector<int32_t>& rank_idx_to_coord_sorted_idxes, const vector<int32_t>& coord_sorted_rank_idxes)
	{
		BitSet ws(point_count);
		for (auto it : *rankCollection)
			ws.set(rank_idx_to_coord_sorted_idxes[it]);

		auto output = shared_ptr<Ranks>(new Ranks(rankCollection->size()));
		int idx = 0;
		for (auto it = ws.get_values_iterator(); !it.at_end(); ++it)
			(*output)[idx++] = coord_sorted_rank_idxes[*it];

		return output;
	}

public:
	static shared_ptr<Ranks> Sort(SortHelp& sortHelp, shared_ptr<Ranks> points, RanksBy current, RanksBy required)
	{
		if (current == required)
			return points;
		
		switch (required)
		{
		case RanksBy::ByRank:
			return SortByRank(sortHelp.points_count, points);

		case RanksBy::ByX:
			return SortByCoordinate(sortHelp.points_count, points, sortHelp.rank_idx_to_x_sorted_idxes, sortHelp.x_sorted_rank_idxes);

		case RanksBy::ByY:
			return SortByCoordinate(sortHelp.points_count, points, sortHelp.rank_idx_to_y_sorted_idxes, sortHelp.y_sorted_rank_idxes);
		}

		throw "Illegal";
	}
};


struct Surface;
typedef shared_ptr<Surface> SurfacePtr;

typedef std::list<std::function<list<SurfacePtr>()>> SurfaceFunctions;

struct Surface
{
	SurfacePtr _left;
	SurfacePtr _right;
	SurfacePtr _top;
	SurfacePtr _bottom;

	Rect _area;

	SurfaceFunctions creators;

	Surface(SortHelp& sortHelp, Coordinates& coordinates, shared_ptr<Ranks> points, RanksBy ordering, bool isVertical, int level);

	void create_pt(const Ids& ids, const Coordinates& coordinates, int32_t r, Point* pt)
	{
		pt->id	 = ids[r];
		pt->rank = r;
		pt->x	 = coordinates[r].x;
		pt->y	 = coordinates[r].y;
	}

	int32_t search(const Ids& ids, const Coordinates& coordinates, const Rect& rect, const int32_t count, Point* out_points)
	{
		if (_left   && rect.hx < _left  ->_area.hx) return _left  ->search(ids, coordinates, rect, count, out_points);
		if (_right  && rect.lx > _right ->_area.lx) return _right ->search(ids, coordinates, rect, count, out_points);
		if (_bottom && rect.hy < _bottom->_area.hy) return _bottom->search(ids, coordinates, rect, count, out_points);
		if (_top    && rect.ly > _top   ->_area.ly) return _top   ->search(ids, coordinates, rect, count, out_points);

		return get_points(ids, coordinates, rect, count, out_points);
	}

	virtual int32_t get_points(const Ids& ids, const Coordinates& coordinates, const Rect& rect, const int32_t count, Point* out_points) = 0;
};

struct Surface0
	: Surface
{
	int _points_count;

	Surface0(SortHelp& sortHelp, Coordinates& coordinates, shared_ptr<Ranks> points, RanksBy ordering, bool isVertical, int level)
		: Surface(sortHelp, coordinates, points, ordering, isVertical, level) 
	{
		_points_count = (int)points->size();
	}

	int32_t get_points(const Ids& ids, const Coordinates& coordinates, const Rect& rect, const int32_t count, Point* out_points)
	{
		using namespace std;
		int32_t result_count = 0;
		for (int r = 0; r < _points_count; ++r) {
			auto const & pt = coordinates[r];
			if (rect.ly <= pt.y && rect.hy >= pt.y && rect.lx <= pt.x && rect.hx >= pt.x)
			{
				create_pt(ids, coordinates, r, out_points++);
				if (++result_count == count)
					break;
			}
		}
		return result_count;
	}
};

struct Surface1
	: Surface
{
	int _points_count;
	int _first;
	unique_ptr<BitSet> _data;

	Surface1(SortHelp& sortHelp, Coordinates& coordinates, shared_ptr<Ranks> points, RanksBy ordering, bool isVertical, int level)
		: Surface(sortHelp, coordinates, points, ordering, isVertical, level)
	{
		auto by_rank = OrderedPoints::Sort(sortHelp, points, ordering, RanksBy::ByRank);

		_points_count = (int)by_rank->size();
		_first = by_rank->front();

		auto last = by_rank->front();
		Ranks::iterator i = by_rank->begin();
		int gaps = 0;
		for (++i; i != by_rank->end(); ++i)
		{
			gaps += *i - last - 1;
			last = *i;
		}

		_data = unique_ptr<BitSet>(new BitSet(_points_count + gaps));

		last = by_rank->front();
		i = by_rank->begin();
		auto idx = 0;
		for (++i; i != by_rank->end(); ++i)
		{
			while (++last < *i)
				++idx;
			_data->set(idx++);
		}
	}

	int32_t get_points(const Ids& ids, const Coordinates& coordinates, const Rect& rect, const int32_t count, Point* out_points)
	{
		using namespace std;
		int32_t result_count = 0;
		auto it = _data->begin();
		auto rank = _first;
		int i = 0;
		while (true)
		{
			auto const & pt = coordinates[rank];
			if (rect.ly <= pt.y && rect.hy >= pt.y && rect.lx <= pt.x && rect.hx >= pt.x)
			{
				create_pt(ids, coordinates, rank, out_points++);
				if (++result_count == count)
					break;
			}

			if (++i == _points_count)
				break;

			for (++rank; *it == false; ++it)
				++rank;
			++it;
		}
		return result_count;
	}
};


struct SurfaceUChar
	: Surface
{
	int _points_count;
	int _first;
	vector<unsigned char> _data;

	SurfaceUChar(SortHelp& sortHelp, Coordinates& coordinates, shared_ptr<Ranks> points, RanksBy ordering, bool isVertical, int level)
		: Surface(sortHelp, coordinates, points, ordering, isVertical, level)
	{
		auto buffer = stack<unsigned char>();
		auto by_rank = OrderedPoints::Sort(sortHelp, points, ordering, RanksBy::ByRank);

		_points_count = (int)by_rank->size();
		_first = by_rank->front();

		auto last = by_rank->front();
		Ranks::iterator i = by_rank->begin();
		int escapes = 0;
		for (++i; i != by_rank->end(); ++i)
		{
			auto n = (*i - last - 1);
			while (n > 254)
			{
				escapes += 2;
				n /= 255;
			}
			last = *i;
		}

		auto data = vector<unsigned char>();
		data.reserve(_points_count + escapes);

		auto capacity = data.capacity();

		last = by_rank->front();
		i = by_rank->begin();
		for (++i; i != by_rank->end(); ++i)
		{
			auto n = *i - last - 1;
			while (n > 254)
			{
				buffer.push(n % 255);
				n /= 255;
			}
			if (buffer.empty())
				data.push_back(n);
			else
			{
				do
				{
					data.push_back(255);
					data.push_back(n);
					n = buffer.top();
					buffer.pop();
				} while (!buffer.empty());
				data.push_back(n);
			}

			last = *i;
		}

		data.shrink_to_fit();
		_data.swap(data);
	}

	int32_t get_points(const Ids& ids, const Coordinates& coordinates, const Rect& rect, const int32_t count, Point* out_points)
	{
		using namespace std;
		int32_t result_count = 0;
		vector<unsigned char>::iterator it = _data.begin();
		auto rank = _first;
		int i = 0;
		while (true)
		{
			auto const & pt = coordinates[rank];
			if (rect.ly <= pt.y && rect.hy >= pt.y && rect.lx <= pt.x && rect.hx >= pt.x)
			{
				create_pt(ids, coordinates, rank, out_points++);
				if (++result_count == count)
					break;
			}

			if (++i == _points_count)
				break;

			auto increment = 0;
			auto check = *it++;
			while (check == 255)
			{
				increment *= 255;
				increment += *it++;
				check = *it++;
			}
			increment *= 255;
			increment += check;

			rank += (increment+1);
		}
		return result_count;
	}
};

struct SurfaceBitSet
	: Surface
{
	BitSet ranks;

	SurfaceBitSet(SortHelp& sortHelp, Coordinates& coordinates, shared_ptr<Ranks> points, RanksBy ordering, bool isVertical, int level)
		: Surface(sortHelp, coordinates, points, ordering, isVertical, level)
		, ranks(sortHelp.points_count)
	{
		for (auto it : *points)
			ranks.set(it);
	}

	int32_t get_points(const Ids& ids, const Coordinates& coordinates, const Rect& rect, const int32_t count, Point* out_points)
	{
		using namespace std;
		int32_t result_count = 0;
		for (auto it = ranks.get_values_iterator(); !it.at_end(); ++it)
		{
			auto const & pt = coordinates[*it];
			if (rect.ly <= pt.y && rect.hy >= pt.y && rect.lx <= pt.x && rect.hx >= pt.x)
			{
				create_pt(ids, coordinates, *it, out_points++);
				if (++result_count == count)
					break;
			}
		}
		return result_count;
	}
};


struct SurfaceX
	: Surface
{
	shared_ptr<Ranks> ranks;

	SurfaceX(SortHelp& sortHelp, Coordinates& coordinates, shared_ptr<Ranks> points, RanksBy ordering, bool isVertical, int level)
		: Surface(sortHelp, coordinates, points, ordering, isVertical, level)
	{
		ranks = OrderedPoints::Sort(sortHelp, points, ordering, RanksBy::ByRank);
	}

	int32_t get_points(const Ids& ids, const Coordinates& coordinates, const Rect& rect, const int32_t count, Point* out_points)
	{
		using namespace std;
		int32_t result_count = 0;
		for (auto rank : *ranks)
		{
			auto const & pt = coordinates[rank];
			if (rect.ly <= pt.y && rect.hy >= pt.y && rect.lx <= pt.x && rect.hx >= pt.x)
			{
				create_pt(ids, coordinates, rank, out_points++);
				if (++result_count == count)
					break;
			}
		}
		return result_count;
	}
};

SurfacePtr surface_factory(SortHelp& sortHelp, Coordinates& coordinates, shared_ptr<Ranks> points, RanksBy ordering, bool isVertical, int level)
{
	if (level == 0)
		return SurfacePtr(new Surface0(sortHelp, coordinates, points, ordering, isVertical, level));
	else if (level == 1)
		return SurfacePtr(new Surface1(sortHelp, coordinates, points, ordering, isVertical, level));
	else if (level < 4)
		return SurfacePtr(new SurfaceBitSet(sortHelp, coordinates, points, ordering, isVertical, level));
	else if (level < 10)
		return SurfacePtr(new SurfaceUChar(sortHelp, coordinates, points, ordering, isVertical, level));
	else
		return SurfacePtr(new SurfaceX(sortHelp, coordinates, points, ordering, isVertical, level));
}

Surface::Surface(SortHelp& sortHelp, Coordinates& coordinates, shared_ptr<Ranks> points, RanksBy ordering, bool isVertical, int level)
{
	auto pointCount = points->size();
	if (pointCount == 0)
		return;

	auto by_x = OrderedPoints::Sort(sortHelp, points, ordering, RanksBy::ByX);
	auto by_y = OrderedPoints::Sort(sortHelp, points, ordering, RanksBy::ByY);

	auto midPoint = pointCount / 2;

	_area = Rect();
	_area.lx = coordinates[by_x->front()].x;
	_area.hx = coordinates[by_x->back()].x;
	_area.ly = coordinates[by_y->front()].y;
	_area.hy = coordinates[by_y->back()].y;
	if (midPoint < 10) // TODO: Make a constant or remove or something
		return;

	auto jobs = shared_ptr<SurfaceFunctions>(new SurfaceFunctions());
	if (!isVertical)
	{
		auto addLeft = [&, by_x, midPoint, level]()->list<SurfacePtr>{
			auto count = midPoint;
			auto points = shared_ptr<Ranks>(new Ranks(count));
			for (auto i = 0; i < count; ++i)
				(*points)[i] = (*by_x)[i];

			_left = surface_factory(sortHelp, coordinates, points, RanksBy::ByX, false, level + 1);
			list<SurfacePtr> surface;
			surface.push_back(_left);
			return surface;
		};
		jobs->push_back(addLeft);

		auto addRight = [&, by_x, pointCount, midPoint, level]()->list<SurfacePtr>{
			auto count = pointCount - midPoint;
			auto points = shared_ptr<Ranks>(new Ranks(count));
			for (auto i = 0; i < count; ++i)
				(*points)[i] = (*by_x)[midPoint+i];

			_right = surface_factory(sortHelp, coordinates, points, RanksBy::ByX, false, level + 1);
			list<SurfacePtr> surface;
			surface.push_back(_right);
			return surface;
		};
		jobs->push_back(addRight);
	}

	auto addBottom = [&, by_y, midPoint, level]()->list<SurfacePtr>{
		auto count = midPoint;
		auto points = shared_ptr<Ranks>(new Ranks(count));
		for (auto i = 0; i < count; ++i)
			(*points)[i] = (*by_y)[i];

		_bottom = surface_factory(sortHelp, coordinates, points, RanksBy::ByY, true, level + 1);
		list<SurfacePtr> surface;
		surface.push_back(_bottom);
		return surface;
	};
	jobs->push_back(addBottom);

	auto addTop = [&, by_y, pointCount, midPoint, level]()->list<SurfacePtr>{
		auto count = pointCount - midPoint;
		auto points = shared_ptr<Ranks>(new Ranks(count));
		for (auto i = 0; i < count; ++i)
			(*points)[i] = (*by_y)[midPoint + i];

		_top = surface_factory(sortHelp, coordinates, points, RanksBy::ByY, true, level + 1);
		list<SurfacePtr> surface;
		surface.push_back(_top);
		return surface;
	};
	jobs->push_back(addTop);

	if (level == 0)
	{
		copy(jobs->begin(), jobs->end(), back_inserter(creators));
	}
	else
	{
		auto create = [jobs](){
			list<SurfacePtr> surfaces;
			for (auto& job : *jobs)
			{
				auto result = job();
				copy(result.begin(), result.end(), back_inserter(surfaces));
			}
			return surfaces;
		};
		creators.push_back(create);
	}
}

struct SearchContext
{
	Mappings _mappings;
	SurfacePtr _topLevel;

	SearchContext(const Point* points_begin, const Point* points_end)
	{
		auto start_time = chrono::system_clock::now();

		auto objects  = points_end - points_begin;
		if (objects > std::numeric_limits<int32_t>::max())
			return;
		
		auto _points_count = (int32_t)objects;

		// we have been told that "rank" is unique. I am also assuming that "rank" is an integer index.
		// i.e. starts at 0, always incrementing by 1.
		// NB. this assumption could be relaxed with anther level of indirection (where we just get back
		// to the 'real' rank with an array access)
		auto checkUnique = BitSet(_points_count);
		auto points = Points(_points_count);
		auto coordinates = Coordinates(_points_count);
		auto ids = Ids(_points_count);
		auto ranks = shared_ptr<Ranks>(new Ranks(_points_count));
		for (auto it = points_begin; it != points_end; ++it)
		{
			auto idx = it->rank;
			if (idx < 0 || idx >= _points_count || checkUnique[idx])
				return;
			checkUnique.set(idx);
			points[idx] = *it;
			coordinates[idx] = Coordinate(it->x, it->y);
			ids[idx] = it->id;
			(*ranks)[idx] = it->rank;
		}

		_mappings.point_count = _points_count;
		_mappings.rank_idx_to_coordinates.swap(coordinates);
		_mappings.rank_idx_to_ids.swap(ids);

		parallel_sort(points.begin(), points.end(), [](const Point& a, const Point& b) { return a.x < b.x; });
		auto x_sorted_rank_idxes = vector<int32_t>(_points_count);
		auto rank_idx_to_x_sorted_idxes = vector<int32_t>(_points_count);
		for (auto i = 0; i < _points_count; ++i)
		{
			x_sorted_rank_idxes[i] = points[i].rank;
			rank_idx_to_x_sorted_idxes[points[i].rank] = i;
		}

		parallel_sort(points.begin(), points.end(), [](const Point& a, const Point& b) { return a.y < b.y; });
		auto y_sorted_rank_idxes = vector<int32_t>(_points_count);
		auto rank_idx_to_y_sorted_idxes = vector<int32_t>(_points_count);
		for (auto i = 0; i < _points_count; ++i)
		{
			y_sorted_rank_idxes[i] = points[i].rank;
			rank_idx_to_y_sorted_idxes[points[i].rank] = i;
		}

		auto sort_help = SortHelp(_points_count);
		sort_help.x_sorted_rank_idxes.swap(x_sorted_rank_idxes);
		sort_help.y_sorted_rank_idxes.swap(y_sorted_rank_idxes);
		sort_help.rank_idx_to_x_sorted_idxes.swap(rank_idx_to_x_sorted_idxes);
		sort_help.rank_idx_to_y_sorted_idxes.swap(rank_idx_to_y_sorted_idxes);


		_topLevel = surface_factory(sort_help, _mappings.rank_idx_to_coordinates, ranks, RanksBy::ByRank, false, 0);
		vector<int> tasks_ids;
		vector<task<list<SurfacePtr>>> tasks;
		auto surface_id = 0;
		auto waiting_surface_id = 0;
		auto& topLevelCreators = _topLevel->creators;
		while (!topLevelCreators.empty())
		{
			tasks.push_back(create_task(topLevelCreators.front()));
			tasks_ids.push_back(surface_id++);
			topLevelCreators.pop_front();
		}

		auto max_time_allowed = 29500;
		auto max_index_before_memory_exceeded = 1100; //TODO: should probably get the surfaces to report their size

		auto creators = SurfaceFunctions();
		auto keep_adding = true;
		while (!tasks.empty() && waiting_surface_id < max_index_before_memory_exceeded)
		{
			if (keep_adding && creators.size() > ((1.1 * max_index_before_memory_exceeded) - waiting_surface_id))
				keep_adding = false;

			auto duration = chrono::system_clock::now() - start_time;
			if (chrono::duration_cast<chrono::milliseconds>(duration).count() > max_time_allowed)
				break;

			concurrency::when_any(tasks.begin(), tasks.end()).wait();
			for (auto i = 0; i < tasks.size(); ++i)
			{
				if (tasks_ids[i] == waiting_surface_id)
				{
					auto& t = tasks[i];
					if (t.wait())
					{
						waiting_surface_id++;

						auto ls = t.get();
						for (auto& s : ls)
						{
							if (keep_adding)
								copy(s->creators.begin(), s->creators.end(), back_inserter(creators));
							s->creators.clear();
						}

						tasks[i] = create_task(creators.front());
						tasks_ids[i] = surface_id++;
						creators.pop_front();
						
						while (tasks.size() < 6 && !creators.empty())
						{
							tasks.push_back(create_task(creators.front()));
							tasks_ids.push_back(surface_id++);
							creators.pop_front();
						}
					}
				}
			}
		}
		concurrency::when_all(tasks.begin(), tasks.end()).wait();
	}

	int32_t search(const Rect& rect, const int32_t count, Point* out_points)
	{
		return _topLevel->search(_mappings.rank_idx_to_ids, _mappings.rank_idx_to_coordinates, rect, count, out_points);
	}
};

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
