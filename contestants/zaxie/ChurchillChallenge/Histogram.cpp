#define _CRT_SECURE_NO_WARNINGS
#include <Windows.h>
#undef min
#undef max

#include <stdio.h>
#include <algorithm>
#include "Histogram.h"

template<typename T>
static inline void umin(T &min, T v)
{
	min = std::min(min, v);
}

template<typename T>
static inline void umax(T &max, T v)
{
	max = std::max(max, v);
}

void Histogram::init(const Point *points, unsigned int count)
{
	_bins.resize(256 * 256);

	_min[0] = _min[1] = _max[0] = _max[1] = FLT_MIN;
	_crange[0] = 0; _crange[1] = 0; _crange[2] = 0;
	
	findRange(points, count);
	makeHistogram(points, count);
	findMinMaxCounts();
}

void Histogram::findRange(const Point *points, unsigned int count)
{
	_min[0] = _max[0] = points[0].x;
	_min[1] = _max[1] = points[0].y;

	for (unsigned i = 1; i < count; ++i)
	{
		umin(_min[0], points[i].x);
		umax(_max[0], points[i].x);
		umin(_min[1], points[i].y);
		umax(_max[1], points[i].y);
	}

	_prange[0] = _max[0] - _min[0];
	_prange[1] = _max[1] - _min[1];
}

void Histogram::makeHistogram(const Point *points, unsigned int count)
{
	std::fill(_bins.begin(), _bins.end(), 0);

	for (unsigned i = 0; i < count; ++i) {
		unsigned x = quantize<0>(points[i].x);
		unsigned y = quantize<1>(points[i].y);
		++_bins[x + y * 256];
	}
}

void Histogram::findMinMaxCounts()
{
	_crange[0] = 65535; _crange[1] = 0; _crange[2] = 0;

	for (unsigned i = 0; i < 256 * 256; ++i) {
		if (!_bins[i]) {
			++_crange[2];
		}
		else {
			umin(_crange[0], _bins[i]);
			umax(_crange[1], _bins[i]);
		}
	}
}

void Histogram::print() const
{
	char buf[1024];
	int n = 0;

	n += sprintf(buf + n, "\nX(%f,%f)=%f", minX(), maxX(), _prange[0]);
	n += sprintf(buf + n, "; Y(%f,%f)=%f", minY(), maxY(), _prange[1]);
	n += sprintf(buf + n, "; C(0,m,M)=%u,%u,%u", zeroCount(), minCount(), maxCount());
	n += sprintf(buf + n, "\n");

	OutputDebugStringA(buf);
	printf(buf);
}
