#include "SecondTry.h"
#include "Accel.h"

#define LEAFSIZE 3072

/*
Observed tests reveal:
- the coordinates seem to cover a rather narrow dynamic range; from about 0.1 to 1000
- EXCEPT: the last four points are on the edges of a square [-1e10,1e10]^2
*/

template <class Function>
__int64 time_call(Function&& f)
{
    LARGE_INTEGER li1, li2;
    QueryPerformanceCounter(&li1);
    f();
    QueryPerformanceCounter(&li2);
    return li2.QuadPart - li1.QuadPart;
}

SecondTry::SecondTry(const Input::Point *begin, const Input::Point *end)
	:
	_points(CopyInput(begin, end)),
	_leafSize(findBestLeafSize(LEAFSIZE)),
	_kdtree(_points, _leafSize)
{
    // __int64 t = time_call(PqTest);
    // printf("PQ time=%llu\n", t);
}

int SecondTry::search(const Input::Rect &rect, const int count, Input::Point *out)
{
	_ASSERTE(count <= PriorityQueue::N);
	auto points = _kdtree.overlap(rect);
    const unsigned sz = std::min((unsigned)points.size(), (unsigned)count);
	
#if 0
	auto slowResult = SlowSearch(_points, rect);
	slowResult.resize(sz);
	_ASSERTE(points == slowResult);
#endif

	for (unsigned i = 0; i < sz; ++i)
		out[i] = points[i].asInputPoint();
	return sz;
}

// With good leaf size, we can end up with one level less in the tree!
unsigned SecondTry::findBestLeafSize(unsigned desiredLeafSize)
{
	unsigned n = (unsigned)_points.size();
	while (n >= desiredLeafSize)
		n = (n + (n & 1)) / 2;	// always divide even
	return 2 * n;
}
