#define _SCL_SECURE_NO_WARNINGS
#include <vector>
#include <algorithm>
#include "pq.h"
#include "Accel.h"

class FirstTry {
	Quantizer _q;
	PriorityQueue<20> _pq;
public:
	FirstTry(const Input::Point *begin, const Input::Point *end);
	int search(const Input::Rect &rect, const int count, Input::Point *out);
};

