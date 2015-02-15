#pragma once

#include "Rect.h"
#include "Point.h"


class BoundsContains {
public:
	BoundsContains(Rect bounds) {
		_bounds = bounds;
	}

	inline bool operator()(const Point& point) const {
		return _bounds.Contains(point);
	}

private:
	Rect _bounds;
};