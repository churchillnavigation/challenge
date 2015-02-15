#include "stdafx.h"
#include "QueryPoints.h"
#include <algorithm>
#include <stack>
#include <tuple>

#include <assert.h>

// Sort points until this approximate limit.
// The value must be greater than 0.
static const size_t BinSize = 10000;

static void GetBounds(const std::vector<Point>& points, Rect& bounds) {
    for (auto p : points) {
        if (p.x < bounds.lx)
            bounds.lx = p.x;
        else if (p.x > bounds.hx)
            bounds.hx = p.x;
        if (p.y < bounds.ly)
            bounds.ly = p.y;
        else if (p.y > bounds.hy)
            bounds.hy = p.y;
    }
}

QueryPoints::QueryPoints(const Point* points_begin, const Point* points_end) {
    points.assign(points_begin, points_end);
    results.reserve(points.size());
}

QueryPoints::~QueryPoints()
{
}

// public function to prepare the search structure
void QueryPoints::MakeQueryStructure() {
    CreateChild(0, points.size(), 0);
}

// find the points within and on the rectangle
void QueryPoints::Search(const Rect& bnds) {
    bounds = bnds;
    target.x = 0.5f * (bnds.lx + bnds.hx);
    target.y = 0.5f * (bnds.ly + bnds.hy);
    results.clear();
    FindPoints(0, points.size(), 0);
}

std::vector<Point>& QueryPoints::GetResults() {
    return results;
}

// Implementation:

// within or on the boundary of the rectangle
bool QueryPoints::InsideSearchRect(const Point& point) {
    return !(bounds.lx > point.x ||
        bounds.ly > point.y ||
        bounds.hx < point.x ||
        bounds.hy < point.y);
}

// discriminate between x and y partition
inline bool QueryPoints::IsEven(size_t depth) {
    return 0 == (depth & 1);
}

// Is algoritm at partition limit?
inline bool QueryPoints::IsLeaf(size_t low, size_t high) {
    return (high - low) < BinSize;
}

// Is the point to the left or below the rectangle?
inline bool QueryPoints::BlockedBelow(const Point& point, size_t depth) {
    bool even = IsEven(depth);
    return (even && (point.x < bounds.lx)) || (!even && (point.y < bounds.ly));
}

// Is the point to the right or above the rectangle?
inline bool QueryPoints::BlockedAbove(const Point& point, size_t depth) {
    bool even = IsEven(depth);
    return (even && (point.x > bounds.hx)) || (!even && (point.y > bounds.hy));
}

// check points in branch leaves, if any
inline void QueryPoints::GetLeafPoints(size_t low, size_t high) {
    if (low != high)
        std::for_each(points.begin() + low, points.begin() + high, [&](const Point& p){if (InsideSearchRect(p)) results.push_back(p); });
}

// Recursively create the search structure.
void QueryPoints::CreateChild(size_t low, size_t high, size_t depth) {
    if (IsLeaf(low, high))
        return;
    size_t mid = low + ((high - low) >> 1);
    auto begin = points.data();
    if (IsEven(depth))
        std::nth_element(begin + low, begin + mid, begin + high, [](const Point& lhs, const Point& rhs){return lhs.x < rhs.x; });
    else
        std::nth_element(begin + low, begin + mid, begin + high, [](const Point& lhs, const Point& rhs){return lhs.y < rhs.y; });
    CreateChild(low, mid, depth + 1);
    CreateChild(mid + 1, high, depth + 1);
}

// Gather inside points while backtracking using recusive style.
void QueryPoints::Gather(size_t low, size_t high, size_t depth)
{
    if (IsLeaf(low, high)) {
        GetLeafPoints(low, high);
        return;
    }

    size_t mid = (low + high) >> 1;
    const Point& point = points[mid];
    if (InsideSearchRect(point))
        results.push_back(point);

    if (!BlockedBelow(point, depth))
        Gather(low, mid, depth + 1);
    if (!BlockedAbove(point, depth))
        Gather(mid + 1, high, depth + 1);
}

// Get points inside rectangle.  First search for the center of the rectangle
// in the query structure then gather points while backtracking the stack.
void QueryPoints::FindPoints(size_t low, size_t high, size_t depth)
{
    if (IsLeaf(low, high)) {
        GetLeafPoints(low, high);
        return;
    }

    size_t mid = low + ((high - low) >> 1);
    const Point& point = points[mid];
    if (InsideSearchRect(point))
        results.push_back(point);

    bool lessThan = (IsEven(depth) && target.x < point.x) || (!IsEven(depth) && target.y < point.y);
    if (lessThan)
        FindPoints(low, mid, depth + 1);
    else
        FindPoints(mid + 1, high, depth + 1);
    if (lessThan)
        Gather(mid + 1, high, depth + 1);
    else
        Gather(low, mid, depth + 1);
}
