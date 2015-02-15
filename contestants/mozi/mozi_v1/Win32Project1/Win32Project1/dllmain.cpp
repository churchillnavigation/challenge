// dllmain.cpp : Defines the entry point for the DLL application.
#include <cinttypes>
//#include <iostream>
#include <array>
#include <vector>
#include <algorithm>
#include <future>
#include <atomic>

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
};

#pragma pack(pop)

template <typename T>
class Region
{
	T* memblock;
	atomic<T*> cursor;
	int itemCount;
public:
	Region(int itemCount)
	{
		cursor = memblock = new T[itemCount];
		this->itemCount = itemCount;
	}
	~Region()
	{
		delete[] memblock;
	}
	inline T* next()
	{
		return cursor++;
	}
	size_t usedBlocks() const
	{
		return cursor - memblock;
	}
};

inline bool point_cmp_rank(const Point* a, const Point* b) {
	return a->rank < b->rank;
}

inline bool point_cmp_x(const Point& a, const Point& b) {
	return a.x < b.x;
}

inline bool point_cmp_y(const Point& a, const Point& b) {
	return a.y < b.y;
}

class Searcher {
	uint32_t searchSize;
	vector<const Point*> results;
	int32_t maxResultRank;
public:

	inline void startSearch(const uint32_t searchSize)
	{
		this->searchSize = searchSize;
		results.clear();
		results.reserve(searchSize);
		maxResultRank = INT_MAX;
	}

	inline void pushResult(const Point* point)
	{
		if (results.size() < searchSize)
		{
			results.push_back(point);
			if (results.size() == searchSize) {
				make_heap(begin(results), end(results), point_cmp_rank);
				maxResultRank = results.front()->rank;
			}
		}
		else
		{
			pop_heap(begin(results), end(results), point_cmp_rank);
			results.back() = point;
			push_heap(begin(results), end(results), point_cmp_rank);
			maxResultRank = results.front()->rank;
		}
	}

	int32_t copyResults(Point* outPoints)
	{
		const auto resultCount = min((uint32_t)results.size(), searchSize);
		sort(begin(results), end(results), point_cmp_rank);
		for (size_t i = 0; i < resultCount; i++)
			outPoints[i] = *results[i];
		return resultCount;
	}

	inline int32_t maxAcceptedRank() const
	{
		return maxResultRank;
	}
};

//int deepest_level = 0;

struct Quad
{
	int32_t minRank;
	uint32_t totalPoints;
	uint16_t maxPointsInline;
	uint16_t level;
	float middle;
	array<Quad*, 2> others;
	vector<const Point*> points;

	void init(uint16_t level) {
		this->level = level;
		minRank = INT_MAX;
		totalPoints = 0;
		maxPointsInline = max(4096, level * level * 8);

		//if (level > deepest_level) {
		//	deepest_level = level;
		//	//cout << " level " << level << endl;
		//}
	}

	int32_t insertPoints(Region<Quad>& region, Point* a, Point* b) {
		totalPoints = b - a;
		if (totalPoints <= maxPointsInline)
		{
			if (totalPoints) {
				points.reserve(totalPoints);
				for (auto p = a; p < b; p++)
					points.push_back(p);
				sort(begin(points), end(points), point_cmp_rank);
				minRank = points.front()->rank;
			}
		}
		else
		{
			const auto newLevel = level + 1;
			others[0] = region.next();
			others[0]->init(newLevel);
			others[1] = region.next();
			others[1]->init(newLevel);

			const auto half = totalPoints / 2;
			if (level & 1) {
				nth_element(a, a + half, b, point_cmp_x);
				middle = a[half].x;
			}
			else {
				nth_element(a, a + half, b, point_cmp_y);
				middle = a[half].y;
			}

			int32_t r1, r2;
			if (level <= 2) // 2 + 4 + 8 = 14 threads to divide and conquer!
			{
				auto f1 = async([&]() { return others[0]->insertPoints(region, a, a + half); });
				auto f2 = async([&]() { return others[1]->insertPoints(region, a + half, b); });
				r1 = f1.get();
				r2 = f2.get();
			}
			else {
				r1 = others[0]->insertPoints(region, a, a + half);
				r2 = others[1]->insertPoints(region, a + half, b);
			}

			if (r1 < minRank)
				minRank = r1;
			if (r2 < minRank)
				minRank = r2;
		}

		return minRank;
	}

	void search(const Rect& rect, Searcher& search) const {
		if (totalPoints <= maxPointsInline)
		{
			for (const auto p : points) {
				if (p->rank > search.maxAcceptedRank())
					break; // nothing to do here anymore
				if (rect.contains(p))
					search.pushResult(p);
			}
		}
		else
		{
			if (level & 1) {
				if (rect.lx < middle)
					others[0]->search(rect, search);
				if (rect.hx >= middle)
					others[1]->search(rect, search);
			}
			else
			{
				if (rect.ly < middle)
					others[0]->search(rect, search);
				if (rect.hy >= middle)
					others[1]->search(rect, search);
			}
		}
	}
};

/* Declaration of the struct that is used as the context for the calls. */
class SearchContext
{
public:
	Region<Point> pointRegion;
	Region<Quad> quadRegion;
	Quad* quad;
	Searcher searcher;

	SearchContext(int numPoints, int numQuads)
		: pointRegion(numPoints), quadRegion(numQuads)
	{
		quad = quadRegion.next();
		quad->init(0);
	}


};

/* Load the provided points into an internal data structure. The pointers follow the STL iterator convention, where
"points_begin" points to the first element, and "points_end" points to one past the last element. The input points are
only guaranteed to be valid for the duration of the call. Return a pointer to the context that can be used for
consecutive searches on the data. */
extern "C" __declspec(dllexport) SearchContext* __stdcall create(const Point* points_begin, const Point* points_end)
{
	const auto pointsCount = points_end - points_begin;
	auto sc = new SearchContext(pointsCount, 50000);

	auto my_points = sc->pointRegion.next();
	memcpy(my_points, points_begin, pointsCount * sizeof(Point));

	sc->quad->insertPoints(sc->quadRegion, my_points, my_points + pointsCount);

	//cout << "level " << deepest_level << endl;
	//cout << "used Pblocks " << sc->pointRegion.usedBlocks() << endl;
	//cout << "used Qblocks " << sc->quadRegion.usedBlocks() << endl;
	return sc;
}

/* Search for "count" points with the smallest ranks inside "rect" and copy them ordered by smallest rank first in
"out_points". Return the number of points copied. "out_points" points to a buffer owned by the caller that
can hold "count" number of Points. */
extern "C" __declspec(dllexport) int32_t __stdcall search(SearchContext* sc, const Rect rect, const int32_t count, Point* out_points)
{
	sc->searcher.startSearch(count);
	sc->quad->search(rect, sc->searcher);

	auto resultCount = sc->searcher.copyResults(out_points);

	//cout << resultCount << " results " << endl;

	return resultCount;
}

/* Release the resources associated with the context. Return nullptr if successful, "sc" otherwise. */
extern "C" __declspec(dllexport) void* __stdcall destroy(SearchContext* sc)
{
	delete sc;
	return NULL;
}
