#include <vector>
#include <algorithm>
#include "pq.h"
#include "Accel.h"

class SecondTry {
	PointV _points;
	unsigned _leafSize;
	KDTree _kdtree;

	unsigned findBestLeafSize(unsigned desiredLeafSize);

public:
	SecondTry(const Input::Point *begin, const Input::Point *end);
	int search(const Rect &rect, const int count, Input::Point *out);
	bool empty() { return _points.empty(); }
};

