//Brian Thorpe 2015
#ifndef NORM_RECT_H
#define NORM_RECT_H

#include "CommonStructs.h"
#include "QueryPoint.h"

class NormRect {

public:
	uint32_t m_LX;
	uint32_t m_LY;
	uint32_t m_HX;
	uint32_t m_HY;
	NormRect(uint32_t lx, uint32_t ly, uint32_t hx, uint32_t hy);
	bool ContainsPoint(uint32_t x, uint32_t y) const;
	bool ContainsRect(uint32_t lx, uint32_t ly, uint32_t hx, uint32_t hy) const;
	bool IntersectsRect(uint32_t lx, uint32_t ly, uint32_t hx, uint32_t hy) const;

	static NormRect NormRect::NormalizedRectFromRect(const Rect r, float xmin, float xrange, float ymin, float yrange);
	static bool NormRect::RectContainsPoint(uint32_t lx, uint32_t ly, uint32_t hx, uint32_t hy, uint32_t px, uint32_t py);
};
#endif