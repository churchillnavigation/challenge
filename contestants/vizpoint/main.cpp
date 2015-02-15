// [Vizpoint]
// Challenge Entry for the Churchill Navigation Programming Challenge
// Solution using Intel Compiler + Intel Thread Building Blocks + simple point distribution analysis
// 9-Feb-2015
// Vincent Vollers <v.m.vollers@gmail.com>

#include <tbb/blocked_range.h>
#include "../../point_search.h"
#include <tbb/tbb.h>
#include <iterator>
#include <vector>
#include "AssocVector.h"

using namespace std;
using namespace tbb;
using namespace Loki;

#define GRID_SIZE 40
#define MAX_GRID_SIZE 39
#define GRID_SIZEF 40.0

template<class It>
It myadvance(It it, int32_t n) {
	advance(it, n);
	return it;
}

struct vizpoint {
	int32_t rank; // rank or lowest rank
	float x;
	float y;
	float min_x;
	float max_x;
	float min_y;
	float max_y;
	float width;
	float height;
	vector<Point> collection;
};

typedef AssocVector<int, Point*> map_type;
typedef combinable<map_type> local_results_type;

struct SearchContext
{
	SearchContext(const Point* &points_begin, const Point* &points_end);
	int32_t search(const Rect &rect, const int32_t &count, Point* &out_points);
	void find_vizpoint(const shared_ptr<vizpoint> &v, const int32_t &count, const Rect &rect);
	void sort_vizpoint(shared_ptr<vizpoint> &v);

	vector<Point> points;
	local_results_type local_results;

	shared_ptr<vizpoint> the_grid[GRID_SIZE][GRID_SIZE];
	shared_ptr<vizpoint> outliers;
	float low_x;
	float high_x;
	float low_y;
	float high_y;
	float height;
	float width;
};

template<typename C> void shrinkContainer(C &container) {
	if (container.size() != container.capacity()) {
		C tmp = container;
		swap(container, tmp);
	}
}

void clear_vizpoint(shared_ptr<vizpoint> v) {
	v->collection.clear();
	v->rank = 0;
	v->x = 0;
	v->y = 0;
	v->min_x = FLT_MAX;
	v->min_y = FLT_MAX;
	v->max_x = FLT_MIN;
	v->max_y = FLT_MIN;
	v->width = 0;
	v->height = 0;
}

void clear_vizpoint(vizpoint* v) {
	v->collection.clear();
	v->rank = 0;
	v->x = 0;
	v->y = 0;
	v->min_x = FLT_MAX;
	v->min_y = FLT_MAX;
	v->max_x = FLT_MIN;
	v->max_y = FLT_MIN;
	v->width = 0;
	v->height = 0;
}

void SearchContext::sort_vizpoint(shared_ptr<vizpoint> &v) {
	if (v->collection.size() == 0) {
		return;
	}

	int min_rank = INT_MAX;

	bool start = true;

	// gather some statistics about the points in this bucket
	for (auto i = v->collection.begin(); i != v->collection.end(); ++i){
		Point &p = *i;
		if (start) {
			v->max_x = p.x;
			v->max_y = p.y;
			v->min_x = p.x;
			v->min_y = p.y;
			start = false;
		}

		if (p.x > v->max_x) v->max_x = p.x;
		if (p.y > v->max_y) v->max_y = p.y;
		if (p.x < v->min_x) v->min_x = p.x;
		if (p.y < v->min_y) v->min_y = p.y;
		if (p.rank < min_rank) min_rank = p.rank;
	}

	v->rank = min_rank;

	v->width = v->max_x - v->min_x;
	v->height = v->max_y - v->min_y;

	v->x = (v->width / 2.0f) + v->min_x;
	v->y = (v->height / 2.0f) + v->min_y;

	// sort the points in this bucket
	parallel_sort(begin(v->collection), end(v->collection), [&](Point a, Point b){return a.rank < b.rank; });
}

void SearchContext::find_vizpoint(const shared_ptr<vizpoint> &v, const int32_t &count, const Rect &rect) {
	int max_rank;

	map_type& results = local_results.local();

	int32_t cnt = results.size();
	if (cnt > 0) {
		max_rank = results.rbegin()->first;
		if (cnt == count && max_rank < v->rank) { // if there is no interesting point in this bucket, just continue
			return;
		}
	}
	else {
		max_rank = INT_MIN;
	}
	if (v->min_x >= rect.lx && v->max_x <= rect.hx &&
		v->min_y >= rect.ly && v->max_y <= rect.hy) {
		// fully inside? add the [count] best points
		vector<Point>::iterator it_start = begin(v->collection);
		vector<Point>::iterator it_end = begin(v->collection);
		advance(it_end, min(count, v->collection.size()));

		// if inside, just insert the best [count] points from this grid
		for (vector<Point>::iterator i = it_start; i != it_end; i = next(i)) {
			Point &p = (*i);
			results[p.rank] = &p;
		}
		if (results.size() > count) {
			results.erase(myadvance(results.begin(), min(count, results.size())), results.end()); // resize to max [count]
		}
	}
	else {
		// if not inside, insert the points which are
		for (vector<Point>::iterator i = begin(v->collection); i != end(v->collection); i = next(i)) {
			Point &p = (*i);
			if (cnt == count) { // if our current results are already full
				if (p.rank > max_rank) { // stop inserting when this point (sorted) is already less good, rest of the points is not interesting anymore
					break;
				}
				// if this point is better, insert in the local results
				if (rect.lx <= p.x && rect.hx >= p.x &&
					rect.ly <= p.y && rect.hy >= p.y) {
					results[p.rank] = &p;
					results.erase(--results.end()); // delete last element
				}
			}
			else {
				// our current results is not full, so every point is interesting regardless of rank
				if (rect.lx <= p.x && rect.hx >= p.x &&
					rect.ly <= p.y && rect.hy >= p.y) {
					results[p.rank] = &p;
					cnt++;
					if (p.rank > max_rank) {
						max_rank = p.rank;
					}
				}
			}
		}
	}
}

SearchContext::SearchContext(const Point* &points_begin, const Point* &points_end)
:points(points_begin, points_end)
{
	// no point in doing anything if there are no points
	if (begin(points) == end(points)) {
		return;
	}

	// now to determine the spatial distribution
	// define the 'outlying' points
	int ninety = (int)(0.01 * points.size()); // < 1 %
	int ninetyr = (int)(0.99 * points.size()); // > 99 %

	parallel_sort(begin(points), end(points), [&](Point a, Point b){return a.x == b.x ? a.y < b.y : a.x < b.x; });

	low_x = points[ninety].x;
	high_x = points[ninetyr].x;
	float min_x = points[0].x;
	float max_x = points[points.size() - 1].x;

	parallel_sort(begin(points), end(points), [&](Point a, Point b){return a.y == b.y ? a.x < b.x : a.y < b.y; });

	low_y = points[ninety].y;
	high_y = points[ninetyr].y;
	float min_y = points[0].y;
	float max_y = points[points.size() - 1].y;

	width = high_x - low_x;
	height = high_y - low_y;

	// finally sort by rank
	parallel_sort(begin(points), end(points), [&](Point a, Point b){return a.rank < b.rank; });

	// prepare 'the grid'
	for (int x = 0; x < GRID_SIZE; x++){
		for (int y = 0; y < GRID_SIZE; y++) {
			the_grid[x][y] = shared_ptr<vizpoint>(new vizpoint());
			clear_vizpoint(the_grid[x][y]);
		}
	}
	outliers = shared_ptr<vizpoint>(new vizpoint());

	// sort all the points into the grid
	for (auto j = begin(points); j != end(points); j = next(j)) {
		Point &p = *j;

		int x = (GRID_SIZEF / width) * (p.x - low_x);
		int y = (GRID_SIZEF / height) * (p.y - low_y);

		if (x >= 0 && x < GRID_SIZE && y >= 0 && y < GRID_SIZE) {
			the_grid[x][y]->collection.push_back(p);
		}
		else {
			// those 'outside' the 99% distribution are considered 'weird outliers'
			outliers->collection.push_back(p);
		}
	}

	// precalculate the point buckets for easier comparison later
	for (int x = 0; x < GRID_SIZE; x++){
		for (int y = 0; y < GRID_SIZE; y++) {
			sort_vizpoint(the_grid[x][y]);
		}
	}
	sort_vizpoint(outliers);
}

int32_t
SearchContext::search(const Rect &rect, const int32_t &c, Point* &out_points)
{
	if (points.size() == 0) {
		return 0;
	}
	map_type results; // final result bucket

	// first let's determine the jobs we have to do
	vector<shared_ptr<vizpoint>> todo;

	// this is the part of the grid we have to consider
	int begin_x = (GRID_SIZEF / width) * (rect.lx - low_x);
	int begin_y = (GRID_SIZEF / height) * (rect.ly - low_y);
	int end_x = (GRID_SIZEF / width) * (rect.hx - low_x);
	int end_y = (GRID_SIZEF / height) * (rect.hy - low_y);

	// clamped on the maximum grid
	// if we have to clamp we have to also search the outliers
	bool outlier = false;
	if (begin_x < 0) {
		outlier = true;
		begin_x = 0;
	}
	else if (begin_x > MAX_GRID_SIZE) {
		outlier = true;
		begin_x = MAX_GRID_SIZE;
	}

	if (begin_y < 0) {
		outlier = true;
		begin_y = 0;
	}
	else if (begin_y > MAX_GRID_SIZE) {
		outlier = true;
		begin_y = MAX_GRID_SIZE;
	}

	if (end_x < 0) {
		outlier = true;
		end_x = 0;
	}
	else if (end_x > MAX_GRID_SIZE) {
		outlier = true;
		end_x = MAX_GRID_SIZE;
	}

	if (end_y < 0) {
		outlier = true;
		end_y = 0;
	}
	else if (end_y > MAX_GRID_SIZE) {
		outlier = true;
		end_y = MAX_GRID_SIZE;
	}

	for (int x = begin_x; x <= end_x; ++x) {
		for (int y = begin_y; y <= end_y; ++y) {
			if (the_grid[x][y]->collection.size() > 0) {
				todo.push_back(the_grid[x][y]);
			}
		}
	}

	if (outlier) {
		todo.push_back(outliers);
	}

	// ok time to search
	local_results.clear();

	// tbb parallel for
	// not sure if this is actually using threadpooling properly
	// had good results with a large 'grain size' (400) which seems to indicate there is not optimal thread pooling (large spinup time)
	parallel_for(
		blocked_range<int32_t>(0, todo.size(), 400),
		[&todo, this, &c, &rect](const blocked_range<int32_t>& range) {
			for (int32_t i = range.begin(); i < range.end(); ++i) find_vizpoint(todo[i], c, rect);
		}
	);

	// combine all the thread-local results to the final results
	local_results.combine_each([&results](const map_type& localSet) {
		results.insert(localSet.begin(), localSet.end());
	});

	int size = min(c, results.size());
	int s = 0;

	// push the points into the outgoing set
	for (auto j = begin(results); j != end(results) && (++s <= size); j = next(j)) {
		*(out_points++) = *(*j).second;
	}

	return size;
}

// dll entry points
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