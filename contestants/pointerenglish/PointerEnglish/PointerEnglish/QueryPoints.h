#pragma once

#include <vector>
#include "point_search.h"

struct SearchContext{};

class QueryPoints : public SearchContext
{
    std::vector<Point> points;
    Rect bounds;
    Point target;
    std::vector<Point> results;
public:
    QueryPoints(const Point* points_begin, const Point* points_end);
    virtual ~QueryPoints();
    void MakeQueryStructure();
    void Search(const Rect& bnds);
    std::vector<Point>& GetResults();
private:
    void CreateChild(size_t low, size_t high, size_t depth);
    bool InsideSearchRect(const Point& point);
    static bool IsEven(size_t depth);
    static bool IsLeaf(size_t low, size_t high);
    bool BlockedBelow(const Point& point, size_t depth);
    bool BlockedAbove(const Point& point, size_t depth);
    void GetLeafPoints(size_t low, size_t high);
    void Gather(size_t low, size_t high, size_t depth);
    void FindPoints(size_t low, size_t high, size_t depth);
};


