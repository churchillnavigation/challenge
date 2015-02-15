// zindorsky.cpp : Defines the exported functions for the DLL application.

#include "stdafx.h"
#include "dll.h"
#include <vector>
#include <algorithm>
#include <thread>
#include <mutex>
#include <limits>
#include <cassert>
#include <math.h>
#include <mmintrin.h>

#undef min
#undef max

//Predicate to order array of pointers to points by rank
struct PointRankPred {
	bool operator () (Point const* a, Point const* b) const { return a->rank < b->rank; }
};

//Use vectorized floats for fast in-rectangle checks.
class RectComp {
public:
	explicit RectComp(Rect const& r)
	{
		data.m128_f32[0] = r.lx;
		data.m128_f32[1] = r.ly;
		data.m128_f32[2] = -r.hx;
		data.m128_f32[3] = -r.hy;
	}

private:
	__m128 data;
	friend class PointComp;
};

class PointComp {
public:
	explicit PointComp(Point const& p)
	{
		data.m128_f32[0] = p.x;
		data.m128_f32[1] = p.y;
		data.m128_f32[2] = -p.x;
		data.m128_f32[3] = -p.y;
	}

	bool in_rect(RectComp const& rect) const
	{
		__m128 res = _mm_cmplt_ps(data,rect.data);
		return !res.m128_i64[0] && !res.m128_i64[1];
	}

private:
	__m128 data;
};

//Region of space with points sorted by rank
class Cell {
public:
	explicit Cell(Rect const& bounds)
		: bounds(bounds)
	{
	}

	//move cstr
	Cell(Cell && other)
		: bounds(other.bounds)
		, points(std::move(other.points))
		, point_comps(std::move(other.point_comps))
	{
	}

	//move assignment
	Cell & operator = (Cell && other)
	{
		bounds = other.bounds;
		points = std::move(other.points);
		point_comps = std::move(other.point_comps);
		return *this;
	}

	//Add a point to this cell, in rank-sorted order.
	void add(Point const* p)
	{
		assert(p->x >= bounds.lx && p->x < bounds.hx && p->y >= bounds.ly && p->y < bounds.hy);
		//point adding is done in parallel on several threads, so we need to synchronize here:
		std::lock_guard<std::mutex> guard(mutex);
		auto s = std::lower_bound(points.begin(),points.end(),p,PointRankPred());
		points.insert(s,p);	
	}

	//Given a sorted (by rank) array of pointers to Points and a rectangle, add any points from this cell with lower rank (in order).
	void contribute(Point const* top[], size_t & topsz, size_t count, RectComp const& rect) const
	{
		const size_t sz = points.size();
		//If there are no points in this cell, or if this cell's top ranked point is greater than the last entry in the top array, bail out now.
		if(sz==0 || (topsz>=count && points.front()->rank >= top[count-1]->rank)) { return; }

		size_t last_insert = 0; //keep track of last insertion point to speed up std::lower_bound below
		for(size_t i=0; i<sz; ++i) {
			Point const* p = points[i];
			//Check if this point beats the last entry in top. If not, nothing else will either, so bail out now.
			if(topsz>=count && p->rank >= top[count-1]->rank) { return; }
			//Check if this point is in the rectangle.
			if(!point_comps[i].in_rect(rect)) {
				continue;
			}
			//Find insertion point in top array.
			Point const* * q = std::lower_bound(top+last_insert,top+topsz,p,PointRankPred());	
			if(topsz < count) {
				++topsz;
			}
			//insert
			size_t pos = q-top;
			memmove(q+1,q,sizeof(Point const*)*(topsz-pos-1));
			*q = p;
			last_insert = pos+1;
		}
	}

	//Same as contribute, but with the assumption that this cell is completely interior to the given rectangle (so no check is needed).
	void contribute_interior(Point const* top[], size_t & topsz, size_t count) const
	{
		const size_t sz = points.size();
		if(sz==0 || (topsz>=count && points.front()->rank >= top[count-1]->rank)) { return; }

		//If this is the first cell to be checked, just copy this cell's top points to top.
		if(topsz==0) {
			topsz = count < sz ? count : sz;
			memcpy(top,&points[0],sizeof(Point const*)*topsz);
			return;
		}
		//same as contribute, but without the rectangle check:
		size_t last_insert = 0;
		for(size_t i=0; i<sz; ++i) {
			Point const* p = points[i];
			if(topsz>=count && p->rank >= top[count-1]->rank) { return; }
			Point const* * q = std::lower_bound(top+last_insert,top+topsz,p,PointRankPred());	
			if(topsz < count) {
				++topsz;
			}
			size_t pos = q-top;
			memmove(q+1,q,sizeof(Point const*)*(topsz-pos-1));
			*q = p;
			last_insert = pos+1;
		}
	}

	//Create a vector of PointComps corresponding to the points, for fast in-rectangle checks
	void create_point_comps()
	{
		point_comps.reserve(points.size());
		for(auto it=points.begin(); it!=points.end(); ++it) {
			point_comps.emplace_back(**it);
		}
	}

	size_t point_count() const
	{
		return points.size();
	}

private:
	Rect bounds;
	std::vector<Point const*> points;
	std::vector<PointComp> point_comps;
	std::mutex mutex;

	//no copy (move only)
	Cell(Cell const&);
	Cell & operator = (Cell const&);
};

//Create a sorted vector of just the coordinates from one axis.
void sort_axis(std::vector<Point> const& source, std::vector<float> & dest, bool x)
{
	dest.reserve(source.size());
	if(x) {
		for(auto it=source.begin(); it!=source.end(); ++it) {
			dest.push_back(it->x);
		}
	} else {
		for(auto it=source.begin(); it!=source.end(); ++it) {
			dest.push_back(it->y);
		}
	}
	std::sort(dest.begin(),dest.end());
}

struct SearchContext {
	SearchContext(const Point* points_begin, const Point* points_end)
		: points(points_begin,points_end)
	{
		if(points.empty()) { 
			return;
		}

		//Create sorted x and y coordinates (in parallel)
		std::vector<float> x_axis, y_axis;
		std::thread xsort(sort_axis,std::cref(points),std::ref(x_axis),true), ysort(sort_axis,std::cref(points),std::ref(y_axis),false);
		xsort.join();
		ysort.join();

		//Partition space into cells, roughly by how many point would be in the cell.
		const size_t sizer = 48; //seems to give best balance between large and small cells.
		const size_t sqsz = static_cast<size_t>( sqrt(points.size()) ) * sizer;

		x_part.reserve(x_axis.size()/sqsz+1);
		y_part.reserve(y_axis.size()/sqsz+1);
		assert(x_axis.size() == y_axis.size());
		for(size_t i=0; i<x_axis.size(); i+=sqsz) {
			x_part.push_back(x_axis[i]);
			y_part.push_back(y_axis[i]);
		}
		float lastx = _nextafterf(x_axis.back(),std::numeric_limits<float>::max()), lasty=_nextafterf(y_axis.back(),std::numeric_limits<float>::max());
		//clear out x_axis and y_axis early to reduce memory footprint:
		std::vector<float>().swap(x_axis); 
		std::vector<float>().swap(y_axis); 

		//Create cells based on x and y partition lines
		cells.reserve(x_part.size()*y_part.size());
		for(size_t i=0; i<x_part.size(); ++i) {
			float lx=x_part[i];
			float hx = i<x_part.size()-1 ? x_part[i+1] : lastx;
			for(size_t j=0; j<y_part.size(); ++j) {
				float ly=y_part[j];
				float hy = j<y_part.size()-1 ? y_part[j+1] : lasty;
				Rect r = {lx,ly,hx,hy};
				cells.emplace_back(r);
			}
		}

		//Now put all points in their corresponding cells.
		//We can do this in parallel since each placement is independent of any other one.
		const size_t core_count = 4;
		std::vector<std::thread> threads;
		Point const* points_curr = &points[0];
		Point const* points_next = points_curr+points.size()/core_count;
		for(size_t i=0; i<core_count; ++i) {
			if(i==core_count-1) { points_next = &points[0]+points.size(); }
			threads.emplace_back(&SearchContext::async_map_points,this,points_curr,points_next);
			points_curr = points_next;
			points_next += points.size()/core_count;
		}
		for(size_t i=0; i<core_count; ++i) {
			threads[i].join();
		}

		//Now create the PointComp objects for each point in each cell. (again, in parallel)
		threads.clear();
		Cell *cell_curr = &cells[0], *cell_next = cell_curr+cells.size()/core_count;
		for(size_t i=0; i<core_count; ++i) {
			if(i==core_count-1) { cell_next = &cells[0]+cells.size(); }
			threads.emplace_back(&SearchContext::async_create_point_comps,this,cell_curr,cell_next);	
			cell_curr = cell_next;
			cell_next += cells.size()/core_count;
		}
		for(size_t i=0; i<core_count; ++i) {
			threads[i].join();
		}
	}

	int32_t search(Rect const& rect, const int32_t count, Point* out_points)
	{
		if(count == 0 || points.empty() || cells.empty() || x_part.empty() || y_part.empty()) { return 0; }

		//Find the edges of the rectangle in the cell grid.
		size_t lxindex = std::upper_bound(x_part.cbegin(),x_part.cend(),rect.lx) - x_part.cbegin() - 1;
		size_t lyindex = std::upper_bound(y_part.cbegin(),y_part.cend(),rect.ly) - y_part.cbegin() - 1;
		size_t hxindex = std::upper_bound(x_part.cbegin(),x_part.cend(),rect.hx) - x_part.cbegin() - 1;
		size_t hyindex = std::upper_bound(y_part.cbegin(),y_part.cend(),rect.hy) - y_part.cbegin() - 1;

		//Compute the corners of the rectangle in the cell grid.
		size_t ll=lxindex*y_part.size() + lyindex;
		size_t ul=lxindex*y_part.size() + hyindex;
		size_t lr=hxindex*y_part.size() + lyindex;
		size_t ur=hxindex*y_part.size() + hyindex;

		size_t width=lr-ll, height=ul-ll;

		assert(ur-ul == width && ur-lr == height);
		assert(width % y_part.size() == 0);

		std::vector<Point const*> top(static_cast<size_t>(count),nullptr);
		Point const* *top0 = &top[0];
		size_t topsz = 0;

		//First do the cells that are completely interior to the rectangle. This avoids having to do costly in-rectangle checks.
		for(size_t i=ll+y_part.size(); i<lr; i+=y_part.size()) {
			for(size_t j=1; j<height; ++j) {
				cells[i+j].contribute_interior(top0,topsz,count);
			}
		}

		//Now do the rest of the cells.
		RectComp rc(rect);
		//left and right edges
		for(size_t i=ll; i<=ul; ++i) {
			cells[i].contribute(top0,topsz,count,rc);
			if(width!=0) {
				cells[i+width].contribute(top0,topsz,count,rc);
			}
		}
		//top and bottom edges
		for(size_t i=ll+y_part.size(); i<lr; i+=y_part.size()) {
			cells[i].contribute(top0,topsz,count,rc);
			if(height != 0) {
				cells[i+height].contribute(top0,topsz,count,rc);
			}
		}

		//Copy to the output param and we're done
		assert(topsz <= count);	
		for(size_t i=0; i<topsz; ++i) {
			*out_points++ = *top[i];
		}
		return static_cast<int32_t>(topsz);
	}

private:
	std::vector<Point> points;
	std::vector<float> x_part, y_part;
	std::vector<Cell> cells;

	//Calculate where in the cell grid a given point is.
	size_t point_to_cell_index(float x, float y) const
	{
		auto xit = std::upper_bound(x_part.cbegin(),x_part.cend(),x);
		assert(xit != x_part.cbegin());
		auto yit = std::upper_bound(y_part.cbegin(),y_part.cend(),y);
		assert(yit != y_part.cbegin());
		size_t xi=xit-x_part.cbegin()-1, yi=yit-y_part.cbegin()-1;
		size_t index = xi*y_part.size()+yi;
		assert(index < cells.size());
		return index;
	}

	//asynchronous mapping of points into cells
	void async_map_points(Point const* points_begin, Point const* points_end)
	{
		for(auto p=points_begin; p!=points_end; ++p) {
			cells[point_to_cell_index(p->x,p->y)].add(p);
		}
	}

	//asynchronous creation of point comp objects for the cells
	void async_create_point_comps(Cell* begin, Cell* end)
	{
		for(Cell* c=begin; c!=end; ++c) {
			c->create_point_comps();
		}
	}
};

//DLL exports
SearchContext* create(const Point* points_begin, const Point* points_end)
{
	return new SearchContext(points_begin,points_end);
}

int32_t search(SearchContext* sc, const Rect rect, const int32_t count, Point* out_points)
{
	return sc->search(rect,count,out_points);
}

SearchContext* destroy(SearchContext* sc)
{
	delete sc;
	return nullptr;
}
