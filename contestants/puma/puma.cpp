/**

Copyright (c) 2015 Lance C. Simons

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

**/


#include <stdio.h>
#include <vector>
#include <queue>
#include <algorithm>
#include "point_search.h"

#ifdef win32
	#define DLLEXPORT __declspec(dllexport)
	#define DLLCALL __stdcall
#else
	#define DLLEXPORT
	#define DLLCALL
#endif

extern "C" {
SearchContext* DLLCALL create(const Point* points_begin, const Point* points_end);
int32_t DLLCALL search(SearchContext* sc, const Rect rect, const int32_t count, Point* out_points);
SearchContext* DLLCALL destroy(SearchContext* sc);
}

bool operator==(Point &a, Point &b)
{
	return a.id == b.id && a.x == b.x && a.y == b.y && a.rank == b.rank;
}

struct PointSet {
	int count;
	float   *x;
	float   *y;
	int32_t  *rank;
	int8_t   *id;
};

//////////////////////////////////////////////////////////////////////////////////////////
// Axis-Aligned Bounding Box, to make life a little easier

class BBOX
{
  public:
	
	enum Relation {OVERLAP, ENCLOSED, DISJOINT};
	
	float mx,Mx,my,My;

	BBOX(Rect r);	
	BBOX(PointSet ps, int32_t *indices, int count);

	bool contains(PointSet ps, int32_t index) const;
	bool contains(float x, float y) const;
	Relation compare(const BBOX &other) const;

	void print() const { printf("[%f %f]x [%f %f]y", mx, Mx, my, My); }
};

BBOX::BBOX(Rect r)
 : mx(r.lx), Mx(r.hx), my(r.ly), My(r.hy)
{ }

BBOX::BBOX(PointSet ps, int32_t *indices, int count)
{
	float x,y;

	if (count > 0)
	{
		x = ps.x[indices[0]];
		y = ps.y[indices[0]];
	
		mx = Mx = x;
		my = My = y;

		for (int i = 1; i < count; ++i)
		{
			x = ps.x[indices[i]];
			y = ps.y[indices[i]];
			
			if (x < mx) mx = x;
			if (y < my) my = y;
			if (Mx < x) Mx = x;
			if (My < y) My = y;
		}
	}
	else // Invalid BBOX will just fail all queries
	{
		mx = my = 1.;
		Mx = My = 0.;
	}
}

bool BBOX::contains(PointSet ps, int32_t index) const
{
	return contains(ps.x[index], ps.y[index]);
}

bool BBOX::contains(float x, float y) const
{
	return mx <= x && x <= Mx && my <= y && y <= My;
}

BBOX::Relation BBOX::compare(const BBOX &other) const
{
	int inside = 0;
	
	if (contains(other.mx, other.my)) inside++;
	if (contains(other.Mx, other.my)) inside++;
	if (contains(other.mx, other.My)) inside++;
	if (contains(other.Mx, other.My)) inside++;
	
	if (inside == 4) return ENCLOSED;
	if (inside  > 0) return OVERLAP;

	if (other.Mx < mx) return DISJOINT;
	if (other.My < my) return DISJOINT;
	if (Mx < other.mx) return DISJOINT;
	if (My < other.my) return DISJOINT;

    return OVERLAP;
}

//////////////////////////////////////////////////////////////////////////////////////////
// Iterators

typedef int32_t rank;

class PointIterator {
  public:	
	virtual bool hasNext(int32_t max) = 0;
	virtual rank top() = 0;
	virtual int32_t consume() = 0;
	virtual bool ready() = 0;
};

class LeafIterator: PointIterator {
  public:
	LeafIterator(PointSet *ps, int32_t *indices, int num_indices, BBOX *region);

	bool hasNext(int32_t max);
	rank top();
	int32_t consume();
	bool ready();

  private:
	PointSet *ps;
	int32_t *indices;
	int num_indices;
	BBOX *region;
	int index;
};

class InternalIterator: PointIterator {
  public:
	InternalIterator(PointSet *ps, std::vector<int32_t> *indices);

	bool hasNext(int32_t max);
	rank top();
	int32_t consume();
	bool ready();

  private:
	PointSet *ps;
	std::vector<int32_t> *indices;
	int index;
};


LeafIterator::LeafIterator(PointSet *_ps, int32_t *_indices, int _num_indices, BBOX *_region)
 : ps(_ps), indices(_indices), num_indices(_num_indices), region(_region), index(0)
{ }

bool LeafIterator::hasNext(int32_t max)
{
	int i = 20;
	while (index < num_indices && ! region->contains(*ps, indices[index]) && i --> 0)
		index++;

	while (index < num_indices && ! region->contains(*ps, indices[index]) && ps->rank[index] < max)
		index++;

	return index < num_indices;
}

rank LeafIterator::top()
{
	if (index < num_indices) return ps->rank[indices[index]];
	return 0x7FFFFFFF;
}

int32_t LeafIterator::consume()
{
	return indices[index++];
}

bool LeafIterator::ready()
{
	if (index < num_indices)
		return region->contains(*ps, indices[index]);
	return false;
}


InternalIterator::InternalIterator(PointSet *_ps, std::vector<int32_t> *_indices)
 : ps(_ps), indices(_indices), index(0)
 { }

bool InternalIterator::hasNext(int32_t max)
{
	return index < indices->size();
} 

rank InternalIterator::top()
{
	if (index < indices->size()) return ps->rank[(*indices)[index]];
	return 0x7FFFFFFF;
}

int32_t InternalIterator::consume()
{
	return (*indices)[index++];
}

bool InternalIterator::ready()
{
	return index < indices->size();
}

//////////////////////////////////////////////////////////////////////////////////////////
// Enhanced Bubble-Up Quadtree

float min(float a, float b) { if (a < b) return a; return b; }
float max(float a, float b) { if (a > b) return a; return b; }
int32_t min(int32_t a, int32_t b) { if (a < b) return a; return b; }
int32_t max(int32_t a, int32_t b) { if (a > b) return a; return b; }

void interleave(PointSet ps, std::vector<int32_t> &src0, std::vector<int32_t> &src1, std::vector<int32_t> &dst, int count)
{
	int i0, i1;
	i0 = i1 = 0;

	while (i0 < src0.size() && i1 < src1.size() && i0+i1 < count)
	{
		if (ps.rank[src0[i0]] < ps.rank[src1[i1]])
		{
			dst.push_back(src0[i0]);
			i0++;
		}
		else
		{
			dst.push_back(src1[i1]);
			i1++;
		}
	}

	while (i0 < src0.size() && i0+i1 < count)
	{
		dst.push_back(src0[i0]);
		i0++;
	}

	while (i1 < src1.size() && i0+i1 < count)
	{
		dst.push_back(src1[i1]);
		i1++;
	}
}

class BUQTree
{
  public:
	BUQTree(PointSet ps, int32_t *_indices, int count, int MAX_CAPACITY=2000, int MAX_CANDIDATES=20);
	~BUQTree();
	
	BUQTree *left, *right;
	int32_t min_value;
	
	int32_t *indices;
	int num_indices;

	std::vector<int32_t> best;
	BBOX bounds;
};

BUQTree::BUQTree(PointSet ps, int32_t *_indices, int count, int MAX_CAPACITY, int MAX_CANDIDATES)
 : bounds(ps, indices, count), best(), indices(_indices), num_indices(count), left(NULL), right(NULL)
{
	if (num_indices <= MAX_CAPACITY)
	{
		std::sort(indices, indices + num_indices, [&ps](int32_t i, int32_t j){ return (ps.rank[i] < ps.rank[j]); });
		min_value = num_indices > 0 ? ps.rank[indices[0]] : 0x7FFFFFFF;
		

		int num_keep = min(MAX_CANDIDATES, num_indices);
		best.reserve(num_keep);
		for (int i = 0; i < num_keep; i++) best.push_back(indices[i]);
	}
	else
	{
		if ((bounds.Mx - bounds.mx) > (bounds.My - bounds.my))
			std::sort(indices, indices + num_indices, [&ps](int32_t i, int32_t j){ return (ps.x[i] < ps.x[j]); });
		else
			std::sort(indices, indices + num_indices, [&ps](int32_t i, int32_t j){ return (ps.y[i] < ps.y[j]); });
		
		int half = num_indices / 2;
		left  = new BUQTree(ps, indices,                      half, MAX_CAPACITY, MAX_CANDIDATES);
		right = new BUQTree(ps, indices + half, num_indices - half, MAX_CAPACITY, MAX_CANDIDATES);
				
		min_value = min(left->min_value, right->min_value);
		interleave(ps, left->best, right->best, best, MAX_CANDIDATES);
	}
}

BUQTree::~BUQTree()
{
	if (left ) delete left;
	if (right) delete right;
}

BUQTree* maketree(PointSet ps, int MAX_CAPACITY=2000, int MAX_CANDIDATES=20)
{
	int32_t *indices = new int32_t[ps.count];
	for (int32_t i = 0; i < ps.count; i++) indices[i] = i;

	return new BUQTree(ps, indices, ps.count, MAX_CAPACITY, MAX_CANDIDATES);
}

//////////////////////////////////////////////////////////////////////////////////////////
// Search functionality

struct nugget {
	rank r;
	PointIterator *iter;

	nugget(rank _r, PointIterator *_iter) : r(_r), iter(_iter) {};
};

bool operator<(nugget const &a, nugget const &b) { return a.r > b.r; }

std::vector<int32_t> query_BUQTree(PointSet ps, int count, BBOX region, BUQTree *root)
{
	std::priority_queue<nugget> queue; 

	std::vector<BUQTree*> front;
	front.push_back(root);

	while (! front.empty())
	{
		BUQTree *node = front.back();
		front.pop_back();

		if (node->left) // is this an internal node?
		{
			auto compare = region.compare(node->bounds);
			
			if (compare == BBOX::Relation::ENCLOSED)
				queue.emplace(node->min_value, (PointIterator*) new InternalIterator(&ps, &node->best));

			if (compare == BBOX::Relation::OVERLAP)
			{
				front.push_back(node->left);
				front.push_back(node->right);
			}
		}
		else // nope. leaf.
		{
			LeafIterator *iter = new LeafIterator(&ps, node->indices, node->num_indices, &region);
			queue.emplace(node->min_value, (PointIterator*) iter);
		}
	}

	std::vector<int32_t> best;
	int32_t stop;

	while (!queue.empty() && best.size() < count)
	{
		nugget n = queue.top();
		queue.pop();

		if (!queue.empty()) stop = queue.top().r;
		               else stop = 0x7FFFFFFF;

		if (! n.iter->ready() )
		{
			if (n.iter->hasNext(stop))
			{
				n.r = n.iter->top();
				queue.push(n);
			}
			continue;
		}

		best.push_back( n.iter->consume() );

		if (n.iter->hasNext(stop))
		{
			n.r = n.iter->top();
			queue.push(n);
		}
	}

	return best;
}

//////////////////////////////////////////////////////////////////////////////////////////
// API

struct SearchContext
{
	BUQTree *root;
	PointSet ps;
};

DLLEXPORT
SearchContext* DLLCALL create(const Point* points_begin, const Point* points_end)
{
	SearchContext *sc = new SearchContext();
	int count = points_end - points_begin;

	sc->ps.count = count;
	sc->ps.x = new float[count];
	sc->ps.y = new float[count];
	sc->ps.rank = new int32_t[count];
	sc->ps.id   = new int8_t[count];

	for (int i = 0; i < count; i++)
	{
		sc->ps.x[i]    = points_begin[i].x;
		sc->ps.y[i]    = points_begin[i].y;
		sc->ps.rank[i] = points_begin[i].rank;
		sc->ps.id[i]   = points_begin[i].id;
	}

	sc->root = maketree(sc->ps, 4000, 20);
	return sc;
}

DLLEXPORT
int32_t DLLCALL search(SearchContext* sc, const Rect rect, const int32_t count, Point* out_points)
{
	std::vector<int32_t> res = query_BUQTree(sc->ps, count, BBOX(rect), sc->root);
	
	for (int i = 0; i < res.size(); i++)
	{
		out_points[i].x    = sc->ps.x[res[i]];
		out_points[i].y    = sc->ps.y[res[i]];
		out_points[i].rank = sc->ps.rank[res[i]];
		out_points[i].id   = sc->ps.id[res[i]];
	}

	return res.size();
}

DLLEXPORT
SearchContext* DLLCALL destroy(SearchContext* sc)
{
	delete [] sc->ps.x;
	delete [] sc->ps.y;
	delete [] sc->ps.rank;
	delete [] sc->ps.id;

	delete [] sc->root->indices;
	delete sc->root;

	delete sc;
	return NULL;
}
