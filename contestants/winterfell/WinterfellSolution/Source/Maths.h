#pragma once

#include "point_search.h"
#include "Switches.h"

//------------------------------------------------------------------------------------------------

enum Axis
{
	Axis_X = 0,
	Axis_Y = 1
};

struct _ALIGN_TO(8) CoordPair
{
	float x;
	float y;

	CoordPair() : x(0.0f), y(0.0f) { }
	CoordPair(const Point& inPoint) : x(inPoint.x), y(inPoint.y) { }

	template< int TAxis >
	inline const float& Get() const;
};

template<> inline const float& CoordPair::Get<Axis_X>() const { return x; }
template<> inline const float& CoordPair::Get<Axis_Y>() const { return y; }


//------------------------------------------------------------------------------------------------

template < typename taPointType >
static inline bool gIsPointInRect(const taPointType& inPoint, const Rect& inRect)
{
	return inPoint.x >= inRect.lx
		&& inPoint.x <= inRect.hx
		&& inPoint.y >= inRect.ly
		&& inPoint.y <= inRect.hy;
}

static inline bool gRectOverlapsRect(const Rect& inRect1, const Rect& inRect2)
{
	return inRect1.lx <= inRect2.hx
		&& inRect1.hx >= inRect2.lx
		&& inRect1.ly <= inRect2.hy
		&& inRect1.hy >= inRect2.ly;
}

//------------------------------------------------------------------------------------------------

template <int TAxis>
static inline const float& gGetCoord(const Point& inPoint);

template <>
static inline const float& gGetCoord<Axis_X>(const Point& inPoint) { return inPoint.x; }

template <>
static inline const float& gGetCoord<Axis_Y>(const Point& inPoint) { return inPoint.y; }

//------------------------------------------------------------------------------------------------

template <int TAxis>
static inline const float& gGetLower(const Rect& inRect);

template <>
static inline const float& gGetLower<Axis_X>(const Rect& inRect) { return inRect.lx; }

template <>
static inline const float& gGetLower<Axis_Y>(const Rect& inRect) { return inRect.ly; }

template <int TAxis>
static inline const float& gGetUpper(const Rect& inRect);

template <>
static inline const float& gGetUpper<Axis_X>(const Rect& inRect) { return inRect.hx; }

template <>
static inline const float& gGetUpper<Axis_Y>(const Rect& inRect) { return inRect.hy; }

//------------------------------------------------------------------------------------------------
