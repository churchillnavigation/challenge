#pragma once

#include <stdint.h>
#include "Rect.h"
#include "Point.h"

#pragma pack(push, 1)

struct DataRect
{
	float lx;
	float ly;
	float hx;
	float hy;

	inline Rect toRect() const {
		Rect newRect;
		newRect.lx = lx;
		newRect.ly = ly;
		newRect.hx = hx;
		newRect.hy = hy;
		return newRect;
	}
};

struct DataPoint
{
	int8_t id;
	int32_t rank;
	float x;
	float y;

	inline Point toPoint() const {
		return Point(id, rank, x, y);
	}

	DataPoint() {}

	DataPoint(const Point & point) {
		id = (int8_t)point.id;
		rank = (int32_t)point.rank;
		x = point.x;
		y = point.y;
	}
};
#pragma pack(pop)


