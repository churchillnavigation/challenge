/* Given 10 million uniquely ranked points on a 2D plane, design a datastructure and an algorithm that can find the 20
most important points inside any given rectangle. The solution has to be reasonably fast even in the worst case, while
also not using an unreasonably large amount of memory.
Create an x64 Windows DLL, that can be loaded by the test application. The DLL and the functions will be loaded
dynamically. The DLL should export the following functions: "create", "search" and "destroy". The signatures and
descriptions of these functions are given below. You can use any language or compiler, as long as the resulting DLL
implements this interface. */

/* This standard header defines the sized types used. */
#include <stdint.h>
#include <vector>
#include <queue>
#include <iostream>
#include <stdint.h>
#include <time.h>
#include <algorithm>
#include "priority_deque.hpp"

using namespace std;
using boost::container::priority_deque;


/* The following structs are packed with no padding. */
#pragma pack(push, 1)

/* Defines a point in 2D space with some additional attributes like id and rank. */
struct Point
{
	int8_t id;
	int32_t rank;
	float x;
	float y;

	Point(){}
	Point(int8_t _id, int32_t _rank, float _x, float _y) :id(_id), rank(_rank), x(_x), y(_y){}

	inline bool operator< (const Point& rhs) const
	{ 
		return rank < rhs.rank;
	}
	inline bool operator== (const Point& rhs) const{
		return id == rhs.id && rank == rhs.rank && x == rhs.x && y == rhs.y;
	}

	friend ostream& operator<<(ostream& os, const Point& p);

};

ostream& operator<<(ostream& os, const Point& p) {
	os << "{" << (int)(p.id) << ", " << p.rank << ", " << p.x << ", " << p.y << "}";
	return os;
}

// TEMPLATE STRUCT less
struct PointLess
	: public binary_function<Point*, Point*, bool>
{	// functor for operator<
	bool operator()(const Point* _Left, const Point* _Right) const
	{	// apply operator< to operands
		return (_Left->rank < _Right->rank);
	}
};

template<class T>
class PtrIterator : public std::iterator<std::input_iterator_tag, T>
{
	T* p;
public:
	PtrIterator(T* x) :p(x) {}
	PtrIterator(const PtrIterator& mit) : p(mit.p) {}
	PtrIterator& operator++() { ++p; return *this; }
	PtrIterator operator++(int) { PtrIterator tmp(*this); operator++(); return tmp; }
	bool operator==(const PtrIterator& rhs) { return p == rhs.p; }
	bool operator!=(const PtrIterator& rhs) { return p != rhs.p; }
	T& operator*() { return *p; }
	PtrIterator operator+(int n) { return PtrIterator(p + n); }

};

/* Defines a rectangle, where a point (x,y) is inside, if x is in [lx, hx] and y is in [ly, hy]. */
struct Rect
{
	float lx;
	float ly;
	float hx;
	float hy;

	Rect(){};
	Rect(float _lx, float _ly, float _hx, float _hy) : lx(_lx), ly(_ly), hx(_hx), hy(_hy){}
	bool contains(const Point& p) const { 
		return p.x >= lx && p.x <= hx && p.y >= ly && p.y <= hy; 
	}
	bool contains(const Rect& r) const { return r.lx >= lx && r.hx <= hx && r.ly >= ly && r.hy <= hy; }
	bool intersects(const Rect& r) const { return  lx < r.hx && hx > r.lx && ly < r.hy && hy > r.ly; }
	void normalise() {
		if (lx>hx) swap(lx, hx);
		if (ly>hy) swap(ly, hy);
	}
	Rect ll() const { return Rect(lx, ly, lx + (hx - lx) / 2, ly + (hy - ly) / 2); }
	Rect lr()const { return Rect(lx + (hx - lx) / 2, ly, hx, ly + (hy - ly) / 2); }
	Rect hl()const { return Rect(lx, ly + (hy - ly) / 2, lx + (hx - lx) / 2, hy); }
	Rect hr()const { return Rect(lx + (hx - lx) / 2, ly + (hy - ly) / 2, hx, hy); }

	friend ostream& operator<<(ostream& os, const Rect& r);

};
#pragma pack(pop)

ostream& operator<<(ostream& os, const Rect& r){
	os << "{" << r.lx << ", " << r.ly << ", " << r.hx << ", " << r.hy << "}";
	return os;
}

template<class T>
struct PackedVector {

	int numItems;
	T items[1];

	PtrIterator<T> begin()  { return PtrIterator<T>(&(items[0])); }
	PtrIterator<T>  end()   { return begin() + numItems; }
	PackedVector<T>* next() { return (PackedVector<T>*)(((uint8_t*)this) + sizeof(PackedVector<T>) + sizeof(T)* numItems); }
};


struct NodeSearchOp{

	int l;
	int ix;
	int iy;
	Rect bounds;
	Rect qrect;
	NodeSearchOp(int _l, int _ix, int _iy, const Rect& _bounds, const Rect& _qrect) : l(_l), ix(_ix), iy(_iy), bounds(_bounds), qrect(_qrect) {}
};


/* Declaration of the struct that is used as the context for the calls. */
struct SearchContext {

	int _levels;
	int _nQuery;
	int* _levelOfs;
	int* _levelSize;
	int _nnodes;
	priority_deque<Point> ** _pdeqs;
	PackedVector<Point>** _toplists;
	PackedVector<Point>* _toplistdata;
	vector<Point> _outliers;
	Rect _bounds;

	int _ncontains = 0;
	int _nintersects = 0;
	int _ndescend = 0;

	unsigned long _tq1 = 0;
	unsigned long  _tq2 = 0;
	unsigned long  _tend = 0;
	unsigned long  _ttotal = 0;

	SearchContext(int levels, Rect bounds, int nQuery);
		
	void insert(const Point& p);
	void outlier(const Point& p);
	__inline Point insertPDeq(priority_deque<Point>** pdeq, const Point& item, int	limit=INT_MAX);
	__inline int SearchContext::nodeOffset(int l, int ix, int iy);
	__inline void SearchContext::enqueueNodeSearchOp(int _l, int _ix, int _iy, const Rect& _bounds, const Rect&  qrect);

	int SearchContext::query(Rect qrect, int count, Point* points);
	void SearchContext::queryNode(int l, int ix, int iy, const Rect& nodeBounds, const Rect& qrect, vector<Point>*& results, vector<Point>*& buf, int nresults);
	void SearchContext::freeze();
	void SearchContext::dump(ostream& f);
};

static Point ptNotInserted(0, -1, 0, 0);
static Point ptInserted(0, -2, 0, 0);


/* Load the provided points into an internal data structure. The pointers follow the STL iterator convention, where
"points_begin" points to the first element, and "points_end" points to one past the last element. The input points are
only guaranteed to be valid for the duration of the call. Return a pointer to the context that can be used for
consecutive searches on the data. */
extern "C" __declspec(dllexport) SearchContext* create(const Point* points_begin, const Point* points_end);

/* Search for "count" points with the smallest ranks inside "rect" and copy them ordered by smallest rank first in
"out_points". Return the number of points copied. "out_points" points to a buffer owned by the caller that
can hold "count" number of Points. */
extern "C" __declspec(dllexport) int32_t search(SearchContext* sc, const Rect rect, const int32_t count, Point* out_points);

/* Release the resources associated with the context. Return nullptr if successful, "sc" otherwise. */
extern "C" __declspec(dllexport) SearchContext* destroy(SearchContext* sc);

/* Load the provided points into an internal data structure. The pointers follow the STL iterator convention, where
"points_begin" points to the first element, and "points_end" points to one past the last element. The input points are
only guaranteed to be valid for the duration of the call. Return a pointer to the context that can be used for
consecutive searches on the data. */
typedef SearchContext* (__stdcall* T_create)(const Point* points_begin, const Point* points_end);

/* Search for "count" points with the smallest ranks inside "rect" and copy them ordered by smallest rank first in
"out_points". Return the number of points copied. "out_points" points to a buffer owned by the caller that
can hold "count" number of Points. */
typedef int32_t(__stdcall* T_search)(SearchContext* sc, const Rect rect, const int32_t count, Point* out_points);

/* Release the resources associated with the context. Return nullptr if successful, "sc" otherwise. */
typedef SearchContext* (__stdcall* T_destroy)(SearchContext* sc);
