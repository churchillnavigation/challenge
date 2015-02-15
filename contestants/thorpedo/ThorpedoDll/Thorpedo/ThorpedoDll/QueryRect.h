//Brian Thorpe 2015
#ifndef QUERY_RECT_H
#define QUERY_RECT_H

#include "CommonStructs.h"
#include "QueryPoint.h"
class QueryRect {

public:
	float m_LX;
	float m_LY;
	float m_HX;
	float m_HY;

	QueryRect(const Rect r);
	QueryRect(float lx, float ly, float hx, float hy);
	bool ContainsPoint(float x, float y) const;
	bool ContainsRect(float lx, float ly, float hx, float hy) const;
	bool IntersectsRect(float lx, float ly, float hx, float hy) const;

	static QueryRect QueryRect::NormalizedRectFromRect(const Rect r, float xmin, float xrange, float ymin, float yrange, float scaling);
	static bool QueryRect::RectContainsPoint(float lx, float ly, float hx, float hy, float px, float py);
};
#endif