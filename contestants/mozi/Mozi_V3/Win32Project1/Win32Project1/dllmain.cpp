// dllmain.cpp : Defines the entry point for the DLL application.
#include <cinttypes>
#include <iostream>
#include <array>
#include <vector>
#include <algorithm>
#include <queue>

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

class Searcher {
	int32_t maxResults;
	vector<const Point*> results;
	int32_t maxResultRank;
public:

	Searcher()
	{
		results.reserve(100);
	}

	void initSearch(const int32_t searchSize)
	{
		maxResults = searchSize;
		results.clear();
		results.reserve(searchSize);
		maxResultRank = INT_MAX;
	}

	void pushResult(const Point* point)
	{
#if 1
		if (results.size() < maxResults)
		{
			results.push_back(point);
			if (results.size() == maxResults) {
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
#else
		results.push_back(point);
		if (results.size() == searchSize)
			maxResultRank = (*max_element(begin(results), end(results), pointp_cmp_rank))->rank;
		else if (results.size() > searchSize)
			maxResultRank = max(maxResultRank, point->rank);
#endif
	}

	int32_t copyResults(Point* outPoints)
	{
		//const auto resultCount = min((uint32_t)results.size(), searchSize);
		const auto resultCount = results.size();
		sort(begin(results), end(results), pointp_cmp_rank);
		for (size_t i = 0; i < resultCount; i++)
			outPoints[i] = *results[i];
		return resultCount;
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
	static const auto MAX_LEAF_POINTS = 4 * 1024;
	static const auto cachedChildPoints = 1024;
	array<Quad*, 2> others;
	int32_t minRank;
	float middle;
	//Rect bounds;
	uint16_t level;
	uint16_t pointCount;
	const Point* points;
	//vector<const Point*> childrenPoints;

	void init(uint16_t level) {
		this->level = level;
		minRank = INT_MAX;
		others[0] = others[1] = NULL;
		points = NULL;
		pointCount = 0;
	}

	int32_t insertPoints(Region<Quad>& region, Point* a, Point* b) {
		const auto totalPoints = b - a;

		if (totalPoints <= MAX_LEAF_POINTS)
		{
			if (totalPoints)
			{
				sort(a, b, point_cmp_rank);
				pointCount = totalPoints;
				points = a;
				minRank = points[0].rank;

				/*bounds.lx = bounds.hx = a->x;
				bounds.ly = bounds.hy = a->y;
				for (auto p = a; p < b; p++)
				{
					bounds.lx = min(bounds.lx, p->x);
					bounds.hx = max(bounds.hx, p->x);
					bounds.ly = min(bounds.ly, p->y);
					bounds.hy = max(bounds.hy, p->y);
				}*/

				/*
				childrenPoints.reserve(pointCount);
				for (size_t i = 0; i < pointCount; i++)
				childrenPoints.push_back(&points[i]);*/
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
			if (level & 1)
			{
				nth_element(a, a + half, b, point_cmp_x);
				middle = a[half].x;
			}
			else
			{
				nth_element(a, a + half, b, point_cmp_y);
				middle = a[half].y;
			}

			const auto r1 = others[0]->insertPoints(region, a, a + half);
			const auto r2 = others[1]->insertPoints(region, a + half, b);
			minRank = min(minRank, min(r1, r2));

			/*bounds.lx = min(others[0]->bounds.lx, others[1]->bounds.lx);
			bounds.hx = max(others[0]->bounds.hx, others[1]->bounds.hx);
			bounds.ly = min(others[0]->bounds.ly, others[1]->bounds.ly);
			bounds.hy = max(others[0]->bounds.hy, others[1]->bounds.hy);*/

			/*childrenPoints.reserve(others[0]->childrenPoints.size() + others[1]->childrenPoints.size());
			merge(begin(others[0]->childrenPoints), end(others[0]->childrenPoints),
				  begin(others[1]->childrenPoints), end(others[1]->childrenPoints),
				  begin(childrenPoints),
				  pointp_cmp_rank);
			if (childrenPoints.size() > cachedChildPoints)
				childrenPoints.resize(cachedChildPoints);*/
		}

		

		return minRank;
	}


	//void inline pushPointsUnchecked(Searcher& search) const
	//{
	//	for (auto p = points, end = points + pointCount; p < end; p++) {
	//		if (p->rank > search.maxAcceptedRank())
	//			break; // nothing to do here anymore
	//		search.pushResult(p);
	//	}
	//}

	//void inline pushChildrenPointsUnchecked(const Rect& rect, Searcher& search) const
	//{
	//	for (auto p = points, end = points + pointCount; p < end; p++) {
	//		if (p->rank > search.maxAcceptedRank())
	//			break; // nothing to do here anymore
	//		search.pushResult(p);
	//	}
	//}

	void search(const Rect& rect, Searcher& search) const
	{
		/*if (rect.contains(bounds) && childrenPoints.size() >= search.searchSize()) {
			pushChildrenPointsUnchecked(rect, search);
			return;
		}*/
		if (others[0] == others[1])
		{
			for (auto p = points, end = points + pointCount; p < end; p++) {
				if (p->rank > search.maxAcceptedRank())
					break; // nothing to do here anymore
				if (rect.contains(p))
					search.pushResult(p);
			}
		}
		else
		{
#if 1
			if (level & 1)
			{
				if (others[0]->minRank <= others[1]->minRank)
				{
					if (rect.lx <= middle && others[0]->minRank < search.maxAcceptedRank())
						others[0]->search(rect, search);
					if (rect.hx >= middle && others[1]->minRank < search.maxAcceptedRank())
						others[1]->search(rect, search);
				}
				else
				{
					if (rect.hx >= middle && others[1]->minRank < search.maxAcceptedRank())
						others[1]->search(rect, search);
					if (rect.lx <= middle && others[0]->minRank < search.maxAcceptedRank())
						others[0]->search(rect, search);
				}
			}
			else
			{
				if (others[0]->minRank <= others[1]->minRank)
				{
					if (rect.ly <= middle && others[0]->minRank < search.maxAcceptedRank())
						others[0]->search(rect, search);
					if (rect.hy >= middle && others[1]->minRank < search.maxAcceptedRank())
						others[1]->search(rect, search);
				}
				else
				{
					if (rect.hy >= middle && others[1]->minRank < search.maxAcceptedRank())
						others[1]->search(rect, search);
					if (rect.ly <= middle && others[0]->minRank < search.maxAcceptedRank())
						others[0]->search(rect, search);
				}
			}
#else
			others[0]->search(rect, search);
			others[1]->search(rect, search);
#endif
		}
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
extern "C" __declspec(dllexport) SearchContext* __stdcall create( Point* points_begin, const Point* points_end)
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
