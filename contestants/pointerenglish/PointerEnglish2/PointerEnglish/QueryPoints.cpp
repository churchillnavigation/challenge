#include "stdafx.h"
#include "QueryPoints.h"
#include <algorithm>
#include <stack>
#include <tuple>

#include <assert.h>

QueryPoints::QueryPoints(const Point* points_begin, const Point* points_end) {
    points.assign(points_begin, points_end);
    results.reserve(points.size());
	binSize = 1000;
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
inline bool QueryPoints::InsideSearchRect(const Point& point) {
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
    return (high - low) < binSize;
}

// check points in branch leaves, if any
inline void QueryPoints::GetLeafPoints(size_t low, size_t high) {
    if (low != high)
        std::for_each(points.begin() + low, points.begin() + high, 
			[&](const Point& p){if (InsideSearchRect(p)) results.push_back(p); });
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

//#define RECURSIVE
#ifdef RECURSIVE

// Gather inside points while backtracking using recusive style.
void QueryPoints::Gather(size_t low, size_t high, size_t depth)
{
    if (IsLeaf(low, high)) {
        GetLeafPoints(low, high);
        return;
    }

    size_t mid = low + ((high - low) >> 1);
    const Point& point = points[mid];
    if (InsideSearchRect(point))
        results.push_back(point);

    bool even = IsEven(depth);
    if (!((even && (point.x < bounds.lx)) || (!even && (point.y < bounds.ly))))
        Gather(low, mid, depth + 1);
    if (!((even && (point.x > bounds.hx)) || (!even && (point.y > bounds.hy))))
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

#else

// Gather inside points using iterative style.
void QueryPoints::Gather(size_t low, size_t high, size_t depth)
{
	if (IsLeaf(low, high)) {
		GetLeafPoints(low, high);
		return;
	}

	std::stack<std::tuple<size_t, size_t, size_t, int>> stack;
	enum StateIndex { LOW, HIGH, DEPTH, CASE };
	while (!stack.empty())
		stack.pop();
	stack.push(std::make_tuple(low, high, depth, 0));

	while (!stack.empty()) {
		auto& tuple = stack.top(); // constraint: don't pop stack until finished with tuple

		low = std::get<LOW>(tuple);
		high = std::get<HIGH>(tuple);
		if (IsLeaf(low, high)) {
			GetLeafPoints(low, high);
			stack.pop();
			continue;
		}
		depth = std::get<DEPTH>(tuple);
		bool even = IsEven(depth);

		size_t mid = low + ((high - low) >> 1);
		const Point& point = points[mid];

		switch (std::get<CASE>(tuple)) {
		case 0:
			std::get<CASE>(tuple) = 1;
			if (InsideSearchRect(point))
				results.push_back(point);
			if (!((even && (point.x < bounds.lx)) || (!even && (point.y < bounds.ly)))) {
				stack.push(std::make_tuple(low, mid, depth + 1, 0));
				continue;
			}
		case 1:
			stack.pop();
			if (!((even && (point.x > bounds.hx)) || (!even && (point.y > bounds.hy))))
				stack.push(std::make_tuple(mid + 1, high, depth + 1, 0));
		}
	}
}

// Get points inside rectangle.  First search for the center of the rectangle
// in the query structure then gather points while backtracking the stack.
void QueryPoints::FindPoints(size_t low, size_t high, size_t depth)
{
	if (IsLeaf(low, high)) {
		GetLeafPoints(low, high);
		return;
	}

	struct State {
        size_t low;
        size_t high;
        size_t depth;
    };
    std::stack<State> stack;

	while (!IsLeaf(low, high)) {
        size_t mid = low + ((high - low) >> 1);
        const Point& point = points[mid];
        if (InsideSearchRect(point))
            results.push_back(point);

        bool even = IsEven(depth);
        if ((even && target.x < point.x) || (!even && target.y < point.y)) {
            stack.push({ mid + 1, high, depth + 1 });
            high = mid;
        }
        else {
            stack.push({ low, mid, depth + 1 });
            low = mid + 1;
        }
        depth++;
    }
	GetLeafPoints(low, high);

    while (!stack.empty()) {
        const auto& state = stack.top();
        Gather(state.low, state.high, state.depth);
        stack.pop();
    }
}
#endif
