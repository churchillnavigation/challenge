//#include "../../point_search.h"
//#include <iterator>
//#include <vector>
//#include <stack>
//#include <algorithm>
//#include <tuple>
//#include <memory>
//#include <unordered_set>
//#include "ppl.h"
//
//using namespace std;
//using concurrency::parallel_sort;
//
//struct Coordinate {
//	Coordinate() {}
//	Coordinate(float x, float y) { this->x = x; this->y = y; }
//	float x;
//	float y;
//};
//
//struct BitSet {
//	struct iterator
//	{
//		const vector<uint64_t>& _data;
//		int _index;
//		uint64_t _bit;
//
//		iterator(const vector<uint64_t>& data)
//			: _data(data)
//		{
//			_index = 0;
//			_bit = 1;
//		}
//
//		iterator(const vector<uint64_t>& data, int index, uint64_t bit)
//			: _data(data)
//			, _index(index)
//			, _bit(bit)
//		{}
//
//		bool operator*() const
//		{
//			return (_data[_index] & _bit) == _bit;
//		}
//
//		bool operator==(const iterator& rhs) const
//		{
//			return _index == rhs._index
//				&& _bit == rhs._bit;
//		}
//
//		bool operator!=(const iterator& rhs) const
//		{
//			return !(*this == rhs);
//		}
//
//		iterator& operator++()
//		{
//			_bit <<= 1;
//			if (!_bit)
//			{
//				_bit = 1;
//				++_index;
//			}
//			return *this;
//		}
//	};
//
//	struct values_iterator {
//		const vector<uint64_t>& _data;
//		int _index;
//		uint64_t _bit;
//		int _bit_value;
//
//		const int _end_index;
//		const uint64_t _end_bit;
//
//		values_iterator(const vector<uint64_t>& data, int end_index, uint64_t end_bit)
//			: _data(data)
//			, _end_index(end_index)
//			, _end_bit(end_bit)
//		{
//			_index = 0;
//			_bit = 0;
//			_bit_value = 0;
//			++(*this);
//		}
//
//		int operator*() const
//		{
//			return _index * 64 + _bit_value;
//		}
//
//		bool at_end()
//		{
//			return _index == _end_index && _bit == _end_bit;
//		}
//
//		values_iterator& operator++()
//		{
//			while (!(_index == _end_index && _bit == _end_bit))
//			{
//				if (_index < _end_index && _data[_index] == 0)
//				{
//					++_index;
//					_bit = 0;
//					_bit_value = 0;
//					continue;
//				}
//
//				if (_bit == 0)
//				{
//					_bit = 1;
//					_bit_value = 0;
//				}
//				else
//				{
//					_bit <<= 1;
//					_bit_value++;
//				}
//
//				while (_bit_value < 64)
//				{
//					if ((_index == _end_index && _bit == _end_bit) || _data[_index] & _bit)
//						goto found;
//					_bit <<= 1;
//					_bit_value++;
//				}
//
//				++_index;
//				_bit = 0;
//				_bit_value = 0;
//			}
//		found:
//			return *this;
//		}
//	};
//
//	uint64_t _count;
//	vector<uint64_t> _data;
//	const iterator _end;
//
//	BitSet(uint32_t count)
//		: _count(count)
//		, _data(count / 64 + (count % 64 == 0 ? 0 : 1))
//		, _end(_data, count / 64, uint64_t(1) << ((count % uint64_t(64)) % uint64_t(64)))
//	{}
//
//	void set(uint32_t bit)
//	{
//		uint32_t idx = bit / 64;
//		uint64_t offset = uint64_t(1) << (bit % uint64_t(64));
//		_data[idx] |= offset;
//	}
//
//	iterator begin()
//	{
//		return iterator(_data);
//	}
//
//	const iterator& end()
//	{
//		return _end;
//	}
//
//	values_iterator get_values_iterator()
//	{
//		return values_iterator(_data, _end._index, _end._bit);
//	}
//
//	void clear()
//	{
//		fill(_data.begin(), _data.end(), uint64_t(0));
//	}
//};
//
//typedef std::vector<Coordinate> Coordinates;
//typedef std::vector<int8_t>		Ids;
//typedef std::vector<int32_t>	Ranks;
//
//struct SortHelp {
//	SortHelp(int count)
//		: _workspace(count) {}
//
//	Ranks _by_X;
//	Ranks _by_Y;
//	BitSet _workspace;
//};
//
//typedef std::vector<Point>   Points;
//typedef std::unique_ptr<Points> PointsPtr;
//
//enum SortedRanks {
//	ByRank,
//	ByX,
//	ByY,
//};
//
//struct OrderedPoints {
//	Ranks _by_rank;
//	Ranks _by_x;
//	Ranks _by_y;
//
//	static void SortByRank(BitSet& ws, const Ranks& rankCollection, Ranks& by_rank)
//	{
//		ws.clear();
//		for (auto it : rankCollection)
//			ws.set(it);
//
//		by_rank.resize(rankCollection.size());
//		int idx = 0;
//		for (auto it = ws.get_values_iterator(); !it.at_end(); ++it)
//			by_rank[idx++] = *it;
//	}
//
//	OrderedPoints(SortHelp& sortHelp, Coordinates& coordinates, Ranks& points, SortedRanks sortedRanks)
//	{
//		switch (sortedRanks)
//		{
//		case SortedRanks::ByRank:
//			_by_rank.swap(points);
//			_by_x = _by_rank;
//			_by_y = _by_rank;
//			parallel_sort(_by_x.begin(), _by_x.end(), [&coordinates](int32_t a, int32_t b) { return coordinates[a].x < coordinates[b].x; });
//			parallel_sort(_by_y.begin(), _by_y.end(), [&coordinates](int32_t a, int32_t b) { return coordinates[a].y < coordinates[b].y; });
//			break;
//
//		case SortedRanks::ByX:
//		{
//			_by_x.swap(points);
//
//			SortByRank(sortHelp._workspace, _by_x, _by_rank);
//
//			_by_y = _by_x;
//			parallel_sort(_by_y.begin(), _by_y.end(), [&coordinates](int32_t a, int32_t b) { return coordinates[a].y < coordinates[b].y; });
//			break;
//		}
//
//		case SortedRanks::ByY:
//			_by_y.swap(points);
//
//			SortByRank(sortHelp._workspace, _by_y, _by_rank);
//
//			_by_x = _by_y;
//			parallel_sort(_by_x.begin(), _by_x.end(), [&coordinates](int32_t a, int32_t b) { return coordinates[a].x < coordinates[b].x; });
//			break;
//		}
//	}
//};
//
//typedef std::list<std::function<void()>> VoidFunctions;
//
//typedef std::shared_ptr<OrderedPoints> OrderedPointsSPtr;
//
//struct Surface;
//typedef unique_ptr<Surface> SurfacePtr;
//
//struct Surface
//{
//	SurfacePtr _left;
//	SurfacePtr _right;
//	SurfacePtr _top;
//	SurfacePtr _bottom;
//
//	Rect _area;
//
//	Surface(SortHelp& sortHelp, Coordinates& coordinates, OrderedPointsSPtr orderedPoints, bool isVertical, VoidFunctions& creators, int level);
//
//	void create_pt(const Ids& ids, const Coordinates& coordinates, int32_t r, Point* pt)
//	{
//		pt->id	 = ids[r];
//		pt->rank = r;
//		pt->x	 = coordinates[r].x;
//		pt->y	 = coordinates[r].y;
//	}
//
//	int32_t search(const Ids& ids, const Coordinates& coordinates, const Rect& rect, const int32_t count, Point* out_points)
//	{
//		if (_left   && rect.hx < _left  ->_area.hx) return _left  ->search(ids, coordinates, rect, count, out_points);
//		if (_right  && rect.lx > _right ->_area.lx) return _right ->search(ids, coordinates, rect, count, out_points);
//		if (_bottom && rect.hy < _bottom->_area.hy) return _bottom->search(ids, coordinates, rect, count, out_points);
//		if (_top    && rect.ly > _top   ->_area.ly) return _top   ->search(ids, coordinates, rect, count, out_points);
//
//		return get_points(ids, coordinates, rect, count, out_points);
//	}
//
//	virtual int32_t get_points(const Ids& ids, const Coordinates& coordinates, const Rect& rect, const int32_t count, Point* out_points) = 0;
//};
//
//struct Surface0
//	: Surface
//{
//	int _points_count;
//
//	Surface0(SortHelp& sortHelp, Coordinates& coordinates, OrderedPointsSPtr orderedPoints, bool isVertical, VoidFunctions& creators, int level)
//		: Surface(sortHelp, coordinates, orderedPoints, isVertical, creators, level) 
//	{
//		_points_count = (int)orderedPoints->_by_rank.size();
//		
//		orderedPoints->_by_rank.clear();
//		orderedPoints->_by_rank.shrink_to_fit();
//	}
//
//	int32_t get_points(const Ids& ids, const Coordinates& coordinates, const Rect& rect, const int32_t count, Point* out_points)
//	{
//		using namespace std;
//		int32_t result_count = 0;
//		for (int r = 0; r < _points_count; ++r) {
//			auto const & pt = coordinates[r];
//			if (rect.ly <= pt.y && rect.hy >= pt.y && rect.lx <= pt.x && rect.hx >= pt.x)
//			{
//				create_pt(ids, coordinates, r, out_points++);
//				if (++result_count == count)
//					break;
//			}
//		}
//		return result_count;
//	}
//};
//
//struct Surface1
//	: Surface
//{
//	int _points_count;
//	int _first;
//	unique_ptr<BitSet> _data;
//
//	Surface1(SortHelp& sortHelp, Coordinates& coordinates, OrderedPointsSPtr orderedPoints, bool isVertical, VoidFunctions& creators, int level)
//		: Surface(sortHelp, coordinates, orderedPoints, isVertical, creators, level)
//	{
//		auto& by_rank = orderedPoints->_by_rank;
//
//		_points_count = (int)by_rank.size();
//		_first = by_rank.front();
//
//		auto last = by_rank.front();
//		Ranks::iterator i = by_rank.begin();
//		int gaps = 0;
//		for (++i; i != by_rank.end(); ++i)
//		{
//			gaps += *i - last - 1;
//			last = *i;
//		}
//
//		_data = unique_ptr<BitSet>(new BitSet(_points_count + gaps));
//
//		last = by_rank.front();
//		i = by_rank.begin();
//		auto idx = 0;
//		for (++i; i != by_rank.end(); ++i)
//		{
//			while (++last < *i)
//				++idx;
//			_data->set(idx++);
//		}
//	}
//
//	int32_t get_points(const Ids& ids, const Coordinates& coordinates, const Rect& rect, const int32_t count, Point* out_points)
//	{
//		using namespace std;
//		int32_t result_count = 0;
//		auto it = _data->begin();
//		auto rank = _first;
//		int i = 0;
//		while (true)
//		{
//			auto const & pt = coordinates[rank];
//			if (rect.ly <= pt.y && rect.hy >= pt.y && rect.lx <= pt.x && rect.hx >= pt.x)
//			{
//				create_pt(ids, coordinates, rank, out_points++);
//				if (++result_count == count)
//					break;
//			}
//
//			if (++i == _points_count)
//				break;
//
//			for (++rank; *it == false; ++it)
//				++rank;
//			++it;
//		}
//		return result_count;
//	}
//};
//
//struct Surface2
//	: Surface
//{
//	int _points_count;
//	int _first;
//	vector<bool> _data;
//
//	Surface2(SortHelp& sortHelp, Coordinates& coordinates, OrderedPointsSPtr orderedPoints, bool isVertical, VoidFunctions& creators, int level)
//		: Surface(sortHelp, coordinates, orderedPoints, isVertical, creators, level)
//	{
//		auto& by_rank = orderedPoints->_by_rank;
//
//		_points_count = (int)by_rank.size();
//		_first = by_rank.front();
//
//		auto last = by_rank.front();
//		Ranks::iterator i = by_rank.begin();
//		int gaps = 0;
//		for (++i; i != by_rank.end(); ++i)
//		{
//			gaps += (*i - last + 1)/3;
//			last = *i;
//		}
//
//		auto data = vector<bool>();
//		data.reserve(2*(_points_count + gaps));
//
//		last = by_rank.front();
//		i = by_rank.begin();
//		for (++i; i != by_rank.end(); ++i)
//		{
//			auto gap = *i - last - 1;
//			while (gap > 0)
//			{
//				switch (gap)
//				{
//				case 1:
//					data.push_back(false);
//					data.push_back(true);
//					gap = 0;
//					break;
//				case 2:
//					data.push_back(true);
//					data.push_back(false);
//					gap = 0;
//					break;
//				default:
//					data.push_back(true);
//					data.push_back(true);
//					gap -= 3;
//				}
//			}
//			data.push_back(false);
//			data.push_back(false);
//
//			last = *i;
//		}
//
//		_data.swap(data);
//	}
//
//	int32_t get_points(const Ids& ids, const Coordinates& coordinates, const Rect& rect, const int32_t count, Point* out_points)
//	{
//		using namespace std;
//		int32_t result_count = 0;
//		vector<bool>::iterator it = _data.begin();
//		auto rank = _first;
//		int i = 0;
//		while (true)
//		{
//			auto const & pt = coordinates[rank];
//			if (rect.ly <= pt.y && rect.hy >= pt.y && rect.lx <= pt.x && rect.hx >= pt.x)
//			{
//				create_pt(ids, coordinates, rank, out_points++);
//				if (++result_count == count)
//					break;
//			}
//
//			if (++i == _points_count)
//				break;
//
//			++rank;
//			while (true)
//			{
//				auto increment = *it++ ? 2 : 0;
//				increment += *it++ ? 1 : 0;
//				if (increment == 0)
//					break;
//				rank += increment;
//			}
//		}
//		return result_count;
//	}
//};
//
//
//struct SurfaceUChar
//	: Surface
//{
//	int _points_count;
//	int _first;
//	vector<unsigned char> _data;
//
//	SurfaceUChar(SortHelp& sortHelp, Coordinates& coordinates, OrderedPointsSPtr orderedPoints, bool isVertical, VoidFunctions& creators, int level)
//		: Surface(sortHelp, coordinates, orderedPoints, isVertical, creators, level)
//	{
//		auto buffer = stack<unsigned char>();
//		auto& by_rank = orderedPoints->_by_rank;
//
//		_points_count = (int)by_rank.size();
//		_first = by_rank.front();
//
//		auto last = by_rank.front();
//		Ranks::iterator i = by_rank.begin();
//		int escapes = 0;
//		for (++i; i != by_rank.end(); ++i)
//		{
//			auto n = (*i - last - 1);
//			while (n > 254)
//			{
//				escapes += 2;
//				n /= 255;
//			}
//			last = *i;
//		}
//
//		auto data = vector<unsigned char>();
//		data.reserve(_points_count + escapes);
//
//		auto capacity = data.capacity();
//
//		last = by_rank.front();
//		i = by_rank.begin();
//		for (++i; i != by_rank.end(); ++i)
//		{
//			auto n = *i - last - 1;
//			while (n > 254)
//			{
//				buffer.push(n % 255);
//				n /= 255;
//			}
//			if (buffer.empty())
//				data.push_back(n);
//			else
//			{
//				do
//				{
//					data.push_back(255);
//					data.push_back(n);
//					n = buffer.top();
//					buffer.pop();
//				} while (!buffer.empty());
//				data.push_back(n);
//			}
//
//			last = *i;
//		}
//
//		_data.swap(data);
//		
//		by_rank.clear();
//		by_rank.shrink_to_fit();
//	}
//
//	int32_t get_points(const Ids& ids, const Coordinates& coordinates, const Rect& rect, const int32_t count, Point* out_points)
//	{
//		using namespace std;
//		int32_t result_count = 0;
//		vector<unsigned char>::iterator it = _data.begin();
//		auto rank = _first;
//		int i = 0;
//		while (true)
//		{
//			auto const & pt = coordinates[rank];
//			if (rect.ly <= pt.y && rect.hy >= pt.y && rect.lx <= pt.x && rect.hx >= pt.x)
//			{
//				create_pt(ids, coordinates, rank, out_points++);
//				if (++result_count == count)
//					break;
//			}
//
//			if (++i == _points_count)
//				break;
//
//			auto increment = 0;
//			auto check = *it++;
//			while (check == 255)
//			{
//				increment *= 255;
//				increment += *it++;
//				check = *it++;
//			}
//			increment *= 255;
//			increment += check;
//
//			rank += (increment+1);
//		}
//		return result_count;
//	}
//};
//
//
//struct SurfaceX
//	: Surface
//{
//	Ranks _rankings;
//
//	SurfaceX(SortHelp& sortHelp, Coordinates& coordinates, OrderedPointsSPtr orderedPoints, bool isVertical, VoidFunctions& creators, int level)
//		: Surface(sortHelp, coordinates, orderedPoints, isVertical, creators, level)
//	{
//		_rankings.swap(orderedPoints->_by_rank);
//	}
//
//	int32_t get_points(const Ids& ids, const Coordinates& coordinates, const Rect& rect, const int32_t count, Point* out_points)
//	{
//		using namespace std;
//		int32_t result_count = 0;
//		for (auto r : _rankings) {
//			auto const & pt = coordinates[r];
//			if (rect.ly <= pt.y && rect.hy >= pt.y && rect.lx <= pt.x && rect.hx >= pt.x)
//			{
//				create_pt(ids, coordinates, r, out_points++);
//				if (++result_count == count)
//					break;
//			}
//		}
//		return result_count;
//	}
//};
//
//SurfacePtr surface_factory(SortHelp& sortHelp, Coordinates& coordinates, OrderedPointsSPtr orderedPoints, bool isVertical, VoidFunctions& creators, int level)
//{
//	switch (level)
//	{
//	case 0:  return SurfacePtr(new Surface0(sortHelp, coordinates, orderedPoints, isVertical, creators, level));
//	//case 1:  return SurfacePtr(new Surface1(sortHelp, coordinates, orderedPoints, isVertical, creators, level));
//	//case 2:  return SurfacePtr(new Surface2(sortHelp, coordinates, orderedPoints, isVertical, creators, level));
//	//case 3:  return SurfacePtr(new SurfaceUChar(sortHelp, coordinates, orderedPoints, isVertical, creators, level));
//	//default: return SurfacePtr(new SurfaceX(sortHelp, coordinates, orderedPoints, isVertical, creators, level));
//	default: return SurfacePtr(new SurfaceUChar(sortHelp, coordinates, orderedPoints, isVertical, creators, level));
//	}
//}
//
//
//Surface::Surface(SortHelp& sortHelp, Coordinates& coordinates, OrderedPointsSPtr orderedPoints, bool isVertical, VoidFunctions& creators, int level)
//{
//	auto pointCount = orderedPoints->_by_rank.size();
//	if (pointCount == 0)
//		return;
//
//	_area = Rect();
//	_area.lx = coordinates[orderedPoints->_by_x.front()].x;
//	_area.hx = coordinates[orderedPoints->_by_x.back()].x;
//	_area.ly = coordinates[orderedPoints->_by_y.front()].y;
//	_area.hy = coordinates[orderedPoints->_by_y.back()].y;
//
//	auto midPoint = pointCount / 2;
//	if (midPoint < 1000) // TODO: Make a constant or remove or something
//		return;
//
//	if (!isVertical)
//	{
//		auto addLeft = [&, orderedPoints, midPoint, level](){
//			auto count = midPoint;
//			auto points = Ranks(count);
//			const auto& byX = orderedPoints->_by_x;
//			for (auto i = 0; i < count; ++i)
//				points[i] = byX[i];
//
//			_left = surface_factory(sortHelp, coordinates, OrderedPointsSPtr(new OrderedPoints(sortHelp, coordinates, points, SortedRanks::ByX)), false, creators, level + 1);
//		};
//		creators.push_back(addLeft);
//
//		auto addRight = [&, orderedPoints, pointCount, midPoint, level](){
//			auto count = pointCount - midPoint;
//			auto points = Ranks(count);
//			const auto& byX = orderedPoints->_by_x;
//			for (auto i = 0; i < count; ++i)
//				points[i] = byX[midPoint+i];
//
//			orderedPoints->_by_x.clear(); // finished with _by_x, so clear
//			orderedPoints->_by_x.shrink_to_fit();
//
//			_right = surface_factory(sortHelp, coordinates, OrderedPointsSPtr(new OrderedPoints(sortHelp, coordinates, points, SortedRanks::ByX)), false, creators, level + 1);
//		};
//		creators.push_back(addRight);
//	}
//
//	auto addBottom = [&, orderedPoints, midPoint, level](){
//		auto count = midPoint;
//		auto points = Ranks(count);
//		const auto& byY = orderedPoints->_by_y;
//		for (auto i = 0; i < count; ++i)
//			points[i] = byY[i];
//
//		_bottom = surface_factory(sortHelp, coordinates, OrderedPointsSPtr(new OrderedPoints(sortHelp, coordinates, points, SortedRanks::ByY)), true, creators, level + 1);
//	};
//	creators.push_back(addBottom);
//
//	auto addTop = [&, orderedPoints, pointCount, midPoint, level](){
//		auto count = pointCount - midPoint;
//		auto points = Ranks(count);
//		const auto& byY = orderedPoints->_by_y;
//		for (auto i = 0; i < count; ++i)
//			points[i] = byY[midPoint + i];
//
//		_top = surface_factory(sortHelp, coordinates, OrderedPointsSPtr(new OrderedPoints(sortHelp, coordinates, points, SortedRanks::ByY)), true, creators, level + 1);
//	};
//	creators.push_back(addTop);
//}
//
//struct SearchContext
//{
//	int32_t _points_count;
//	Coordinates _coordinates;
//	Ids _ids;
//
//	SurfacePtr _topLevel;
//
//	SearchContext(const Point* points_begin, const Point* points_end)
//		: _points_count(0)
//	{
//		auto rankedAll = Points(points_begin, points_end);
//		parallel_sort(begin(rankedAll), end(rankedAll), [](const Point& a, const Point& b) { return a.rank < b.rank; });
//
//		auto points_count = rankedAll.size();
//		if (points_count > std::numeric_limits<int32_t>::max())
//			return;
//
//		_points_count = (int)points_count;
//		auto coordinates = Coordinates(_points_count);
//		auto ids = Ids(_points_count);
//		auto ranks = Ranks(_points_count);
//		for (Points::size_type i = 0; i < _points_count; ++i)
//		{
//			auto& pt = rankedAll[i];
//			if (pt.rank != i) // !! NB. We rely on the rank being just an index !! Another rank table could be put in at the cost of memory.
//				return;
//			coordinates[i] = Coordinate(pt.x, pt.y);
//			ids[i] = pt.id;
//			ranks[i] = pt.rank;
//		}
//		_coordinates.swap(coordinates);
//		_ids.swap(ids);
//
//		auto sortHelp = SortHelp(_points_count);
//
//		auto topLevelPoints = OrderedPointsSPtr(new OrderedPoints(sortHelp, _coordinates, ranks, SortedRanks::ByRank));
//		auto creators = VoidFunctions();
//
//		_topLevel = surface_factory(sortHelp, _coordinates, topLevelPoints, false, creators, 0);
//
//		for (auto i = 0; !creators.empty() && i < 1000/*based on memory*/; ++i)
//		{
//			auto add_a_surface = creators.front();
//			creators.pop_front();
//			add_a_surface();
//		}
//	}
//
//	int32_t search(const Rect& rect, const int32_t count, Point* out_points)
//	{
//		return _topLevel->search(_ids, _coordinates, rect, count, out_points);
//	}
//};
//
//extern "C" __declspec(dllexport)
//SearchContext* __stdcall
//create(const Point* points_begin, const Point* points_end)
//{
//	return new SearchContext(points_begin, points_end);
//}
//
//extern "C" __declspec(dllexport)
//int32_t __stdcall
//search(SearchContext* sc, const Rect rect, const int32_t count, Point* out_points)
//{
//	return sc->search(rect, count, out_points);
//}
//
//extern "C" __declspec(dllexport)
//SearchContext* __stdcall
//destroy(SearchContext* sc)
//{
//
//	delete sc;
//	return nullptr;
//}
