// Copyright 2015 Paul Lange
#ifndef FAST_POINT_HEADER
#define FAST_POINT_HEADER

#include "point_search.h"
#include <limits>

struct AlignedPoint {
	char _padding[3];
	int8_t  id;
	int32_t rank;
	float x;
	float y;
	AlignedPoint(){}

	AlignedPoint(const Point &point)
		:x(point.x), y(point.y),
		rank(point.rank), id(point.id) {
	}

	operator Point() {
		Point p;
		p.id = id;
		p.rank = rank;
		p.x = x;
		p.y = y;
		return p;
	}
};

class Rank3b {
	uint8_t b[3];
public:
	Rank3b(){}
	Rank3b(const int32_t &value) {
		const uint32_t v = value | 0u;
		b[0] = v & 0xFF;
		b[1] = (v >>  8) & 0xFF;
		b[2] = (v >> 16) & 0xFF;
	}
	operator int32_t() const {
		int32_t value = *reinterpret_cast<const int32_t*>(&b);
		return value & 0xFFFFFF;
	}
};

#ifdef NDEBUG
struct fastFloat {
	union {
		int32_t i;
		float f;
	};
	void convert() {
		if (i < 0) i = ~i | 0x80000000;
	}
	fastFloat(){}
	explicit fastFloat(const float &f)
		:f(f) {
		convert();
	}
	explicit operator float() const{
		fastFloat reverted(f);
		return reverted.f;
	}
};
inline bool operator  <(const fastFloat &a, const fastFloat &b) { return a.i < b.i; }
inline bool operator  >(const fastFloat &a, const fastFloat &b) { return a.i > b.i; }
inline bool operator <=(const fastFloat &a, const fastFloat &b) { return a.i <= b.i; }
inline bool operator >=(const fastFloat &a, const fastFloat &b) { return a.i >= b.i; }
#else
typedef float fastFloat;
#endif
const fastFloat POS_INFINITY = (fastFloat) std::numeric_limits<float>::infinity();

struct fastPoint {
	fastFloat x;
	fastFloat y;
	int32_t rank;
	int8_t id;

	fastPoint(){}

	explicit fastPoint(const Point &point)
		:x(point.x), y(point.y),
		rank(point.rank), id(point.id) {
		// bitwise copy signed values to unsigned type
		//uint32_t u_rank = point.rank | 0u;
		//uint8_t  u_id   = point.id   | 0u;
			
		// store rank in upper 24 bits so we can still easily
		// sort them and the id won't affect the results (rank is unique)
		//rank = (u_rank << 8) | u_id;
	}

	explicit operator Point() {
		Point p;
		//p.id   = rank & 0xFFu;
		//p.rank = rank >> 8;
		p.id = id;
		p.rank = rank;
		p.x = (float)x;
		p.y = (float)y;
		return p;
	}
};

template<class T> inline bool sort_x(const T &a, const T &b) { return a.x < b.x; }
template<class T> inline bool sort_y(const T &a, const T &b) { return a.y < b.y; }
template<class T> inline bool sort_rank(const T &a, const T &b) { return a.rank < b.rank; }

struct fastRect {
	fastFloat lx;
	fastFloat ly;
	fastFloat hx;
	fastFloat hy;

  	fastRect(const Rect &rect)
		:lx(rect.lx), ly(rect.ly), hx(rect.hx), hy(rect.hy)
	{}
  	fastRect(){}
};


template<typename rect_t, typename point_t>
__forceinline bool is_inside (const rect_t &rect, const point_t &point) {
	if( rect.lx <= point.x &&
		rect.ly <= point.y &&
		rect.hx >= point.x &&
		rect.hy >= point.y )
		return true;
	return false;
}


#endif