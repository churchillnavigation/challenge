#include "FirstTry.h"
#include "Accel.h"

static bool PointInRectangle(const Input::Rect &rect, const Point &p)
{
	return p.data<0>() >= rect.lx && p.data<0>() <= rect.hx && p.data<1>() >= rect.ly && p.data<1>() <= rect.hy;
}

/*
Observed tests reveal:
- the coordinates seem to cover a rather narrow dynamic range; from about 0.1 to 1000
- EXCEPT: the last four points are on the edges of a square [-1e10,1e10]^2
*/

FirstTry::FirstTry(const Input::Point *begin, const Input::Point *end) :
	_q(begin, end-begin)
{
	if (!_q.points().empty())
		_q.print();
}

int FirstTry::search(const Input::Rect &rect, const int count, Input::Point *out)
{
	if (count != 20)
		abort();

	_pq.clear();

	std::for_each(_q.points().begin(), _q.points().end(),
		[=](const Point &p) {
		if (PointInRectangle(rect, p))
			_pq.push(p.rank(), &p);
	});

	unsigned sz = _pq.size();
	for (unsigned i = 0; i < sz; ++i) {
		const Point  *p = static_cast<const Point*>(_pq.pop());
		*out++ = p->asInputPoint();
	}
	return (int)sz;
}

