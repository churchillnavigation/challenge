//_____________________________________________________________________________________________________________________________
#include "point_search.h"
#include <vector> // STL is easy, but we should be able to do better
#include <queue>
//_____________________________________________________________________________________________________________________________
// INTERNAL CONSTANTS, TYPES AND STRUCTS:
enum { CITY_PER_NODE = 8 };

// We use our own version of the Point structure, that has more efficient memory alignment, and an integer representation
// instead of the floats (see f2i below for explanation).
struct PointX
{
	int32_t id;
	int32_t rank;
	uint32_t x;
	uint32_t y;
	uint32_t split;
};

struct SearchContext
{
	PointX* m_points;
	size_t m_count;
};
//_____________________________________________________________________________________________________________________________
// INTERNAL PRIVATE STATIC FUNCTIONS:
//.............................................................................................................................
// These functions convert floats to unsigned integers in a way that is fully reversible, and maintain bitwise monotonic ordering
static uint32_t f2i(const float f)
{
	const int32_t i = *reinterpret_cast<const int32_t*>(&f); // bit cast to int32_t
	const int32_t j = i < 0 ? 0x80000000 - i : i; // mirror if negative
	const uint32_t k = j + 0x80000000; // offset to be unsigned
	return k;
}
//.............................................................................................................................
static float i2f(const uint32_t k)
{
	const int32_t j = k - 0x80000000; // offset to be signed
	const int32_t i = j < 0 ? 0x80000000 - j : j; // mirror if negative
	const float f = *reinterpret_cast<const float*>(&i); // bit cast to float
	return f;
}
//.............................................................................................................................
static void CreateTree(PointX* begin, PointX* end, const int32_t split_axis)
{
	class OrderByRank { public: bool operator() (const PointX& a, const PointX& b) const { return a.rank < b.rank; } };
	
	{// Leaf node:
		const size_t count = end - begin;
		if( count <= CITY_PER_NODE * 2 )
		{
			const uint32_t split = begin->split;
			std::sort(begin, end, OrderByRank());
			begin->split = split;
			return;
		}
	}

	PointX* lo = begin + CITY_PER_NODE;
	PointX* hi = lo + (end - lo) / 2;
	
	// Get the most important points first:
	const uint32_t split = begin->split;
	std::partial_sort(begin, lo, end, OrderByRank());
	begin->split = split;

	// Then split the rest:
	if( split_axis )
	{
		class OrderByX { public: bool operator() (const PointX& a, const PointX& b) const { return a.x < b.x; } };
		std::nth_element(lo, hi, end, OrderByX());
		hi->split = hi->x;
	}
	else
	{
		class OrderByY { public: bool operator() (const PointX& a, const PointX& b) const { return a.y < b.y; } };
		std::nth_element(lo, hi, end, OrderByY());
		hi->split = hi->y;
	}

	const int32_t child_split_axis = 1 - split_axis;
	CreateTree(lo, hi, child_split_axis);
	CreateTree(hi, end, child_split_axis);
}
//.............................................................................................................................
template<typename T> uint32_t BubbleInsert(T* results, const int32_t size, int32_t& used, T c)
{
	results[used++] = c;
	T* c0 = results + used - 2;
	T* c1 = c0 + 1;

	while( c1 != results )
	{
		if( (*c0)->rank < (*c1)->rank )
		{
			break;
		}
		T t = *c0;
		*c0 = *c1;
		*c1 = t;

		--c0;
		--c1;
	}

	if( used > size )
	{
		used = size;
	}

	return (*(results + used - 1))->rank;
}
//.............................................................................................................................
static int32_t Query(const PointX** out_results, const int32_t out_count, const PointX* root_begin, const PointX* root_end, Rect rect)
{
	// We use this struct to store node info for breadth first traversal
	struct Node
	{
		const PointX* begin;
		const PointX* end;
		int32_t split_axis;
	};

	static std::queue<Node> node_queue;

	const uint32_t l[2] = { f2i(rect.lx), f2i(rect.ly) };
	const uint32_t h[2] = { f2i(rect.hx), f2i(rect.hy) };

	const Node root = { root_begin, root_end, 1 };
	int32_t results_used = 0;
	node_queue.push(root);
	int32_t lowest_rank = -1;

	while( !node_queue.empty() )
	{
		const Node n = node_queue.front();
		node_queue.pop();

		const size_t cnt = n.end - n.begin;
		if( cnt <= CITY_PER_NODE * 2 )
		{
			for(const PointX* i = n.begin; i != n.end; ++i)
			{
				if( results_used == out_count && i->rank > lowest_rank ) { goto skip_node; }
				if( i->x < l[0] || i->x > h[0] || i->y < l[1] || i->y > h[1] ) { continue; }
				lowest_rank = BubbleInsert(out_results, out_count, results_used, i);
			}
			continue;
		}

		const PointX* lo = n.begin + CITY_PER_NODE;
		const PointX* hi = lo + (n.end - lo) / 2;
		for(const PointX* i = n.begin; i != lo; ++i)
		{
			if( results_used == out_count && i->rank > lowest_rank ) { goto skip_node; }
			if( i->x < l[0] || i->x > h[0] || i->y < l[1] || i->y > h[1] ) { continue; }
			lowest_rank = BubbleInsert(out_results, out_count, results_used, i);
		}

		const uint32_t split = hi->split;
		const int32_t s = 1 - n.split_axis;
		if( split >= l[s] )
		{
			const Node c = { lo, hi, s };
			node_queue.push(c);
		}
		if( split <= h[s] )
		{
			const Node c = { hi, n.end, s };
			node_queue.push(c);
		}
skip_node:;
	}

	return results_used;
}
//_____________________________________________________________________________________________________________________________
// EXPORTED PUBLIC FUNCTIONS:
//.............................................................................................................................
extern "C" SearchContext* __stdcall create(const Point* points_begin, const Point* points_end)
{
	// Create and initialize SearchContext:
	SearchContext* o = new SearchContext;
	o->m_count = points_end - points_begin;
	o->m_points = new PointX[o->m_count];

	// Convert and copy all the points to the more efficient internal format:
	PointX* dst = o->m_points;
	for(const Point* src = points_begin; src != points_end; ++src)
	{
		dst->id = src->id;
		dst->rank = src->rank;
		dst->x = f2i(src->x);
		dst->y = f2i(src->y);
		++dst;
	}

	CreateTree(o->m_points, o->m_points + o->m_count, 1);
	return o;
}
//.............................................................................................................................
extern "C" int32_t __stdcall search(SearchContext* o, const Rect rect, const int32_t count, Point* out_points)
{
	if (o == nullptr) { return 0; }
	static std::vector<const PointX*> results;
	results.resize(count + 1);
	const PointX** r = &*results.begin();
	const int32_t result_count = Query(r, count, o->m_points, o->m_points + o->m_count, rect);
	for(int32_t i = 0; i < result_count; ++i)
	{
		const PointX* src = r[i];
		Point* dst = out_points + i;
		dst->id = static_cast<int8_t>(src->id);
		dst->rank = src->rank;
		dst->x = i2f(src->x);
		dst->y = i2f(src->y);
	}
	return result_count;
}
//.............................................................................................................................
extern "C" SearchContext* __stdcall destroy(SearchContext* o)
{
	delete [] o->m_points;
	delete o;
	return nullptr;
}
//_____________________________________________________________________________________________________________________________
