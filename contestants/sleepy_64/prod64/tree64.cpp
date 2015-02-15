#include "tree64.h"
#include "search_amd64.h"
#include <algorithm>
#include <cassert>

namespace search2d{
size_t search_node(const SearchContext& c, const Node&node, const Rectangle& r, const uint32_t maxOutput, vector<FoundPoint>& outPoints);
Rectangle partition_node(SearchContext& c, Node& node, vector<FoundPoint>& points, Range pointsRange);
void sort_points_by_rank(vector<FoundPoint>& points, Range pointsRange);
bool output_point(const FoundPoint&o, vector<FoundPoint>& outPoints, const size_t maxOutput);
inline
NodeIndex invalid_node_index()
{
    return 0;
}
size_t search(const SearchContext& c, const Rectangle& r, const uint32_t maxOutput, vector<FoundPoint>& outPoints)
{
    if (geom2d::disjoint_rectangles(r, c.global_bbox)) return 0;
    return search_node(c, c.nodes[0], r, maxOutput, outPoints);
}

size_t search_node(const SearchContext& c, const Node&node, const Rectangle& r, const uint32_t maxOutput, vector<FoundPoint>& outPoints)
{
    if (!geom2d::disjoint_rectangles(r, node.ownBbox))
    {
        auto outputAccepted = true;
        linear_search8(&c.points[node.own.b], node.own.size(), r, [&c, &node, maxOutput, &outPoints, &outputAccepted](size_t i)->bool
        {
            size_t index = node.own.b + i;
            outputAccepted = output_point(FoundPoint{ c.points[index], c.ranks[index], c.payload[index] }, outPoints, maxOutput);
            return outputAccepted;
        });

        if (!outputAccepted && outPoints.size() >= maxOutput)
        {
            return outPoints.size();
        }
    }

    uint_fast64_t children = 0, mask = 1;
    for (uint_fast64_t i = 0; i < kMaxNodeChildren; ++i, mask = mask << 1u)
    {
        if (node.children[i] == invalid_node_index()) continue;
        const Rectangle& ch = node.bboxes[i];
        children |= !geom2d::disjoint_rectangles(ch, r) ? mask : 0u;
    }

    for (uint_fast64_t i = 0, mask = 1; i < kMaxNodeChildren; ++i, mask = mask << 1u)
    {
        if ((children & mask) != 0)
        {
            search_node(c, c.nodes[node.children[i]], r, maxOutput, outPoints);
        }
    }
    return outPoints.size();
}

SearchContext* create_context(vector<FoundPoint>& points)
{
    assert(points.size() < INT32_MAX && "arbitrary limit based on requirement to ranks to be unique");

    SearchContext* result = new SearchContext;
    SearchContext& c= *result;
    size_t len = points.size();
    c.points.reserve(len);
    c.ranks.reserve(len);
    c.payload.reserve(len);

    Range r = { 0, PtIndex(len) };
    Node n0 = {};
    c.nodes.resize(1);
    c.global_bbox = partition_node(c, n0, points, r);
    result->nodes[0] = n0;
    return result;
}

Rectangle partition_node(SearchContext& c, Node& node, vector<FoundPoint>& points, Range pointsRange)
{
    //TODO(Sleepy): this has to be smarter for big data
    sort_points_by_rank(points, pointsRange);

    size_t ownNodePointsCount = std::min<size_t>(pointsRange.size(), kMaxNodePoints);
    Range own = { pointsRange.b, pointsRange.b + PtIndex(ownNodePointsCount) };
    Rectangle bbox = invalid_bbox();
    for (PtIndex i = own.b; i != own.e; ++i)
    {
        const auto& pt = points[i];
        c.points.push_back(pt.p);
        c.ranks.push_back(pt.rank);
        c.payload.push_back(pt.payload);
        geom2d::update_bbox(bbox, pt.p);
    }
    node.own = own;
    node.ownBbox = bbox;
    for (auto i = 0; i < kMaxNodeChildren; ++i)
    {
        node.children[i] = invalid_node_index();
        node.bboxes[i] = invalid_bbox();
    }

    if (ownNodePointsCount >= pointsRange.size())
    {
        node.bbox = bbox;
        return bbox;
    }

    enum Direction
    {
        D_X, D_Y,
    };
    array<Direction, kMaxNodeChildren> directions = { D_X, };
    array<Range, kMaxNodeChildren> ranges = { own.e, pointsRange.e };
    uint32_t nranges = 1;

    while (nranges < kMaxNodeChildren)
    {
        auto prange = std::max_element(&ranges[0], &ranges[nranges], [](const Range& a, const Range& b) { return a.size() < b.size(); });
        ptrdiff_t rangeIndex = prange - &ranges[0];
        Range r = ranges[rangeIndex];
        if (r.size() < kMaxNodePoints) break;

        //TODO(Sleepy): if u ever want this to finish in reasonable time here has to be a cap on range length.
        Direction direction = directions[rangeIndex];
        if (direction == D_X)
        {
            std::sort(points.begin() + r.b, points.begin() + r.e, [](const FoundPoint& a, const FoundPoint& b)
            {
                return a.p.x < b.p.x;
            });
        }
        else
        {
            std::sort(points.begin() + r.b, points.begin() + r.e, [](const FoundPoint& a, const FoundPoint& b)
            {
                return a.p.y < b.p.y;
            });
        }

        PtIndex pivot = r.b + r.size() / 2 ;
        Range lower = Range{ r.b, pivot + 1 };
        Range upper = Range{ pivot + 1, r.e };
        
        ranges[rangeIndex] = lower;
        ranges[nranges] = upper;

        auto nextDirection = direction == D_X ? D_Y : D_X;
        directions[nranges] = directions[rangeIndex] = nextDirection;

        ++nranges;
    }
    std::sort(ranges.begin(), ranges.begin() + nranges, [](const Range& a, const Range& b){ return a.b < b.b; });
#ifndef NDEBUG
    assert(node.own.b == pointsRange.b);
    assert(ranges[0].b == node.own.e);
    assert(ranges[nranges-1].e == pointsRange.e);
    for (unsigned i = 1; i < nranges; ++i)
    {
        assert(ranges[i - 1].e == ranges[i].b);
    }
#endif
    const NodeIndex childrenBase = (NodeIndex)c.nodes.size();
    c.nodes.resize(childrenBase + nranges, Node{});
    array<Node, kMaxNodeChildren> nodes = {};
     
    for (auto i = 0u; i < nranges; ++i)
    {
        Range r = ranges[i];
        Node& child = nodes[i];
        auto childBbox = partition_node(c, child, points, r);
        node.bboxes[i] = childBbox;
        node.children[i] = childrenBase + i;
        update_bbox(bbox, childBbox);
    }

    for (NodeIndex i = 0; i < nranges; ++i)
    {
        c.nodes[childrenBase + i] = nodes[i];
    }
    node.bbox = bbox;
    return bbox;
}

void sort_points_by_rank(vector<FoundPoint>& points, Range pointsRange)
{
    std::sort(points.begin() + pointsRange.b, points.begin() + pointsRange.e, [](const FoundPoint& a, const FoundPoint& b) 
    {
#ifndef NDEBUG
        assert(a.rank != b.rank && "non unique ranked point detected");
#endif
        return a.rank < b.rank; 
    });
}

bool output_point(const FoundPoint&o, vector<FoundPoint>& outPoints, const size_t maxOutput)
{
    size_t pos = outPoints.size();
    for (; pos >= 1; --pos)
    {
        if (outPoints[pos - 1].rank < o.rank) break;
    }
    if (pos >= maxOutput) return false;
    outPoints.insert(outPoints.begin() + pos, o);
    if (outPoints.size() > maxOutput)
    {
        outPoints.pop_back();
    }
    return true;
}

}