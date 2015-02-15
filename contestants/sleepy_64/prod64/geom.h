#pragma once
#include <cstdint>
#include <algorithm> //min,max
namespace geom2d{
struct Point
{
    float x,y;
};
struct Rectangle
{
    Point a0, a1;
};

inline
bool disjoint_rectangles(const Rectangle& r0, const Rectangle& r1)
{
    return (r0.a0.x > r1.a1.x || r0.a0.y > r1.a1.y || r0.a1.x < r1.a0.x || r0.a1.y < r1.a0.y);
}
inline
bool point_in_rectangle(const Point& pt, const Rectangle& r)
{
    return pt.x >= r.a0.x && pt.x <= r.a1.x && pt.y >= r.a0.y && pt.y <= r.a1.y;
}
inline
bool operator == (const Point& p0, const Point& p1)
{
    return p0.x == p1.x && p0.y == p1.y;
}
inline
bool operator == (const Rectangle& p0, const Rectangle& p1)
{
    return p0.a0 == p1.a0 && p0.a1 == p1.a1;
}
inline
Rectangle invalid_bbox()
{
    return Rectangle{ Point{FLT_MAX, FLT_MAX}, Point{-FLT_MAX, -FLT_MAX} };
}
inline
void update_bbox(Rectangle& r, const Point& p)
{
    r.a0.x = std::min(r.a0.x, p.x);
    r.a0.y = std::min(r.a0.y, p.y);
    r.a1.x = std::max(r.a1.x, p.x);
    r.a1.y = std::max(r.a1.y, p.y);
}
inline
void update_bbox(Rectangle& r, const Rectangle& r2)
{
    r.a0.x = std::min(r.a0.x, r2.a0.x);
    r.a0.y = std::min(r.a0.y, r2.a0.y);
    r.a1.x = std::max(r.a1.x, r2.a1.x);
    r.a1.y = std::max(r.a1.y, r2.a1.y);
}
}