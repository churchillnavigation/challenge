// dllmain.cpp : Defines the entry point for the DLL application.
#include <iostream>
#include <cmath>
#include <array>
#include <queue>
#include <algorithm>
#include <cinttypes>
#include <climits>
#include <tuple>

using namespace std;

/* The following structs are packed with no padding. */
#pragma pack(push, 1)
/* Defines a point in 2D space with some additional attributes like id and rank. */
struct Point
{
	int8_t id;
	int32_t rank;
	float x;
	float y;
};

/* Defines a rectangle, where a point (x,y) is inside, if x is in [lx, hx] and y is in [ly, hy]. */
struct Rect
{
	float lx;
	float ly;
	float hx;
	float hy;

	inline bool contains(const Point* p) const
	{
		return p->x >= lx && p->x <= hx &&
			p->y >= ly && p->y <= hy;
	}

	/*inline bool contains(const Rect& r) const
	{
	return r.lx >= lx && r.hx <= hx &&
	r.ly >= ly && r.hy <= hy;
	}

	inline bool intersect(const Rect& r) const
	{
	return lx <= r.hx && hx >= r.lx &&
	ly <= r.hy && hy >= r.ly;
	}*/
};

#pragma pack(pop)

inline bool pointp_cmp_rank(const Point* a, const Point* b) {
	return a->rank < b->rank;
}

inline bool point_cmp_rank(const Point& a, const Point& b) {
	return a.rank < b.rank;
}

inline bool point_cmp_x(const Point& a, const Point& b) {
	return a.x < b.x;
}

inline bool point_cmp_y(const Point& a, const Point& b) {
	return a.y < b.y;
}


template <typename T>
class Region
{
	T* memblock;
	T* cursor;
	size_t itemCount;
public:
	Region() {
		cursor = memblock = NULL;
		itemCount = 0;
	}
	~Region()
	{
		dealloc();
	}
	void alloc(size_t itemCount)
	{
		dealloc();
		cursor = memblock = new T[itemCount]{};
		this->itemCount = itemCount;
	}
	void dealloc() {
		if (memblock)
			delete[] memblock;
	}
	inline T* next()
	{
		/*if (usedBlocks() >= itemCount)
		cout << " CRASH @" << usedBlocks() << endl;*/
		return cursor++;
	}
	size_t usedBlocks() const
	{
		return cursor - memblock;
	}
};


template<typename T, size_t T_SIZE>
class ArrayQueue
{
	static_assert(T_SIZE && !(T_SIZE & (T_SIZE - 1)), "T_SIZE must be a power of 2");
	uint32_t frontPos;
	uint32_t backPos;
	T items[T_SIZE];
public:

	inline T pop()
	{
		return items[frontPos++ /*% T_SIZE*/];
	}

	inline void push(T t)
	{
		items[backPos++ /*% T_SIZE*/] = t;
	}

	inline bool empty()
	{
		return backPos == frontPos;
	}

	void clear()
	{
		//cout << frontPos << " " << backPos << endl;
		frontPos = backPos = 0;
	}
};


struct Quad; // forward dec.

class Searcher {
	int32_t maxResultRank;
	uint32_t maxResults;
	uint32_t resultSize;
	array<const Point*, 20> results;
public:
	ArrayQueue<const Quad*, 8 * 1024> queue;

	inline void initSearch(const uint32_t searchSize)
	{
		maxResults = searchSize;
		resultSize = 0;
		maxResultRank = INT_MAX;
		queue.clear();
	}

	void pushResult(const Point* point)
	{
		if (resultSize < maxResults)
		{
			results[resultSize++] = point;
			if (resultSize == maxResults) {
				make_heap(begin(results), end(results), pointp_cmp_rank);
				maxResultRank = results.front()->rank;
			}
		}
		else
		{
			pop_heap(begin(results), end(results), pointp_cmp_rank);
			results.back() = point;
			push_heap(begin(results), end(results), pointp_cmp_rank);
			maxResultRank = results.front()->rank;
		}
	}

	inline int32_t copyResults(Point* outPoints)
	{
		sort(begin(results), begin(results) + resultSize, pointp_cmp_rank);
		for (auto i = 0u; i < resultSize; i++)
			outPoints[i] = *results[i];
		return resultSize;
	}

	inline int32_t maxAcceptedRank() const
	{
		return maxResultRank;
	}

	inline int searchSize() const
	{
		return maxResults;
	}
};


struct Quad
{
	static const auto MAX_LEAF_POINTS = 128;
	static const auto MIN_MIDDLE_POINTS_LVL = 3;
	static const uint16_t MIDDLE_POINTS = 32;
	const Point* points;
	uint16_t pointCount;
	uint16_t level;
	int32_t minRank;
	array<Quad*, 2> others;
	float middle;

	void init(uint16_t level)
	{
		this->level = level;
		minRank = INT_MAX;
		others[0] = others[1] = NULL;
		points = NULL;
		pointCount = 0;
	}

	static void insertPoints(Region<Quad>& region, Quad* q, Point* a, Point* b)
	{
		// build in breadth first order to minimize L1 misses on query
		queue<tuple<Quad*, Point *, Point*>> queue;
		queue.push(make_tuple(q, a, b));

		while (!queue.empty())
		{
			auto q = get<0>(queue.front());
			a = get<1>(queue.front());
			b = get<2>(queue.front());
			queue.pop();

			auto totalPoints = b - a;

			if (totalPoints <= MAX_LEAF_POINTS)
			{
				if (totalPoints)
				{
					sort(a, b, point_cmp_rank);
					q->pointCount = totalPoints;
					q->points = a;
					q->minRank = q->points[0].rank;
				}
			}
			else
			{
				const auto newLevel = q->level + 1;
				q->others[0] = region.next();
				q->others[0]->init(newLevel);
				q->others[1] = region.next();
				q->others[1]->init(newLevel);

				if (MIDDLE_POINTS && q->level >= MIN_MIDDLE_POINTS_LVL)
				{
					q->points = a;
					q->pointCount = min(MIDDLE_POINTS, (uint16_t)totalPoints);
					partial_sort(a, a + q->pointCount, b, point_cmp_rank);
					q->minRank = q->points[0].rank;

					a += q->pointCount;
					totalPoints -= q->pointCount;
				}
				else
				{
					q->minRank = min_element(a, b, point_cmp_rank)->rank;
				}

				const auto half = totalPoints / 2;
				if (q->level & 1)
				{
					nth_element(a, a + half, b, point_cmp_x);
					q->middle = a[half].x;
				}
				else
				{
					nth_element(a, a + half, b, point_cmp_y);
					q->middle = a[half].y;
				}

				queue.push(make_tuple(q->others[0], a, a + half));
				queue.push(make_tuple(q->others[1], a + half, b));
			}
		}
	}

	void insertPoints(Region<Quad>& region, Point* a, Point* b)
	{
		Quad::insertPoints(region, this, a, b);
	}

	static void search(const Quad* q, const Rect& rect, Searcher& search)
	{
		// search in breadth first order
		while (true)
		{
			for (auto p = q->points, end = q->points + q->pointCount; p < end; p++) {
				if (p->rank > search.maxAcceptedRank())
					goto nextQuad; // nothing to do here anymore
				if (rect.contains(p))
					search.pushResult(p);
			}

			if (q->others[0] != q->others[1])
			{
				if (q->level & 1)
				{
					if (q->others[0]->minRank <= q->others[1]->minRank)
					{
						if (rect.lx <= q->middle && q->others[0]->minRank < search.maxAcceptedRank())
							search.queue.push(q->others[0]);
						if (rect.hx >= q->middle && q->others[1]->minRank < search.maxAcceptedRank())
							search.queue.push(q->others[1]);
					}
					else
					{
						if (rect.hx >= q->middle && q->others[1]->minRank < search.maxAcceptedRank())
							search.queue.push(q->others[1]);
						if (rect.lx <= q->middle && q->others[0]->minRank < search.maxAcceptedRank())
							search.queue.push(q->others[0]);
					}
				}
				else
				{
					if (q->others[0]->minRank <= q->others[1]->minRank)
					{
						if (rect.ly <= q->middle && q->others[0]->minRank < search.maxAcceptedRank())
							search.queue.push(q->others[0]);
						if (rect.hy >= q->middle && q->others[1]->minRank < search.maxAcceptedRank())
							search.queue.push(q->others[1]);
					}
					else
					{
						if (rect.hy >= q->middle && q->others[1]->minRank < search.maxAcceptedRank())
							search.queue.push(q->others[1]);
						if (rect.ly <= q->middle && q->others[0]->minRank < search.maxAcceptedRank())
							search.queue.push(q->others[0]);
					}
				}
			}

		nextQuad:
			if (search.queue.empty())
				break;
			q = search.queue.pop();
		}
	}

	void search(const Rect& rect, Searcher& search) const
	{
		Quad::search(this, rect, search);
	}
};

/* Declaration of the struct that is used as the context for the calls. */
class SearchContext
{
	Region<Point> pointRegion;
	Region<Quad> quadRegion;
	Quad* quad;
	Searcher searcher;
public:
	SearchContext() {
		quad = NULL;
	}

	void insertPoints(const Point* points_begin, const Point* points_end) {
		const auto pointCount = points_end - points_begin;

		pointRegion.alloc(pointCount);

		const auto quadCount = 1 + (pointCount / Quad::MAX_LEAF_POINTS + 1) * 4;
		quadRegion.alloc(quadCount);
		quad = quadRegion.next();
		quad->init(0);

		auto my_points = pointRegion.next();
		memcpy(my_points, points_begin, pointCount * sizeof(Point));

		quad->insertPoints(quadRegion, my_points, my_points + pointCount);
	}

	int32_t search(const Rect& rect, const int32_t count, Point* out_points) {
		// Searcher should be local in the real world :)
		searcher.initSearch(count);
		quad->search(rect, searcher);
		return searcher.copyResults(out_points);
	}
};

/* Load the provided points into an internal data structure. The pointers follow the STL iterator convention, where
"points_begin" points to the first element, and "points_end" points to one past the last element. The input points are
only guaranteed to be valid for the duration of the call. Return a pointer to the context that can be used for
consecutive searches on the data. */
extern "C" __declspec(dllexport) SearchContext* __stdcall create(Point* points_begin, const Point* points_end)
{
	auto sc = new SearchContext();
	sc->insertPoints(points_begin, points_end);

	//cout << "used Pblocks " << sc->pointRegion.usedBlocks() << endl;
	//cout << "used Qblocks " << sc->quadRegion.usedBlocks() << endl;
	return sc;
}

/* Search for "count" points with the smallest ranks inside "rect" and copy them ordered by smallest rank first in
"out_points". Return the number of points copied. "out_points" points to a buffer owned by the caller that
can hold "count" number of Points. */
extern "C" __declspec(dllexport) int32_t __stdcall search(SearchContext* sc, const Rect rect, const int32_t count, Point* out_points)
{
	return sc->search(rect, count, out_points);
}

/* Release the resources associated with the context. Return nullptr if successful, "sc" otherwise. */
extern "C" __declspec(dllexport) void* __stdcall destroy(SearchContext* sc)
{
	delete sc;
	return NULL;
}
