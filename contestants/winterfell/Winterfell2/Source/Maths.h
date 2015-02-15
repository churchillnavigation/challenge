#pragma once

#include "point_search.h"
#include "Switches.h"

//------------------------------------------------------------------------------------------------

enum Axis
{
	Axis_X = 0,
	Axis_Y = 1
};

struct CoordPair
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

inline void gSetMaxExtents(Rect& inRect)
{
	inRect.lx = -FLT_MAX;
	inRect.ly = -FLT_MAX;
	inRect.hx = FLT_MAX;
	inRect.hy = FLT_MAX;
}

inline void gSetInvMaxExtents(Rect& inRect)
{
	inRect.lx = FLT_MAX;
	inRect.ly = FLT_MAX;
	inRect.hx = -FLT_MAX;
	inRect.hy = -FLT_MAX;
}

inline void gExpandToContain(Rect& inRect, const CoordPair& inCoord)
{
	inRect.lx = std::min(inCoord.x, inRect.lx);
	inRect.hx = std::max(inCoord.x, inRect.hx);
	inRect.ly = std::min(inCoord.y, inRect.ly);
	inRect.hy = std::max(inCoord.y, inRect.hy);
}

//------------------------------------------------------------------------------------------------

template < typename taPointType >
inline bool gIsPointInRect(const taPointType& inPoint, const Rect& inRect)
{
	return	(inPoint.x >= inRect.lx)
		&	(inPoint.x <= inRect.hx)
		&	(inPoint.y >= inRect.ly)
		&	(inPoint.y <= inRect.hy);
}

inline bool gRectOverlapsRect(const Rect& inRect1, const Rect& inRect2)
{
	return	(inRect1.lx <= inRect2.hx) 
		&	(inRect1.hx >= inRect2.lx)
		&	(inRect1.ly <= inRect2.hy)
		&	(inRect1.hy >= inRect2.ly);
}

inline bool gIsRectFullyInsideRect(const Rect& inRect1, const Rect& inRect2)
{
	return	(inRect1.lx >= inRect2.lx) 
		&	(inRect1.hx <= inRect2.hx)
		&	(inRect1.ly >= inRect2.ly)
		&	(inRect1.hy <= inRect2.hy);
}

//------------------------------------------------------------------------------------------------

template <int TAxis>
inline const float& gGetCoord(const Point& inPoint);

template <>
inline const float& gGetCoord<Axis_X>(const Point& inPoint) { return inPoint.x; }

template <>
inline const float& gGetCoord<Axis_Y>(const Point& inPoint) { return inPoint.y; }

template <int TAxis>
inline bool gCompareCoord(const Point& inLHS, const Point& inRHS)
{
	return gGetCoord<TAxis>(inLHS) < gGetCoord<TAxis>(inRHS);
}

//------------------------------------------------------------------------------------------------

template <int TAxis>
inline const float& gGetLower(const Rect& inRect);

template <>
inline const float& gGetLower<Axis_X>(const Rect& inRect) { return inRect.lx; }

template <>
inline const float& gGetLower<Axis_Y>(const Rect& inRect) { return inRect.ly; }

template <int TAxis>
inline const float& gGetUpper(const Rect& inRect);

template <>
inline const float& gGetUpper<Axis_X>(const Rect& inRect) { return inRect.hx; }

template <>
inline const float& gGetUpper<Axis_Y>(const Rect& inRect) { return inRect.hy; }

//------------------------------------------------------------------------------------------------
