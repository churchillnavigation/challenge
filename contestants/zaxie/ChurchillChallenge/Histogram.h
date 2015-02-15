#ifndef HISTOGRAM_H
#define HISTOGRAM_H

#include <vector>
#include "../point_search.h"

class Histogram
{
	std::vector<unsigned> _bins;
	float _min[2], _max[2], _prange[2];
	unsigned _crange[3];

	void findRange(const Point *points, unsigned int count);
	void makeHistogram(const Point *points, unsigned int count);
	void findMinMaxCounts();

public:
	void init(const Point *points, unsigned int count);
	
	template<int dim>
	unsigned int quantize(float v) const {
		float q = ((v - _min[dim]) / _prange[dim]);
		return (unsigned)(q*255);
	}

	float minX() const { return _min[0]; }
	float maxX() const { return _max[0]; }
	float minY() const { return _min[1]; }
	float maxY() const { return _max[1]; }

	// Only non-zero bins.
	unsigned int minCount() const { return _crange[0]; }
	unsigned int maxCount() const { return _crange[1]; }
	unsigned int zeroCount() const { return _crange[2]; }

	void print() const;
};

#endif