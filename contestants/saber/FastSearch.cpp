#include "FastSearch.hpp"

#include <algorithm>
#include <iostream>
#include <cstdint>
#include <memory>
#include <vector>

typedef std::uint8_t  uint8;
typedef std::uint16_t uint16;
typedef std::uint32_t uint32;
typedef std::uint64_t uint64;

typedef std::int8_t  int8;
typedef std::int16_t int16;
typedef std::int32_t int32;
typedef std::int64_t int64;

template<typename T>
T min(T a, T b)
{
    return a < b ? a : b;
}
template<typename T>
T max(T a, T b)
{
    return a > b ? a : b;
}

struct AlignedPoint
{
    float x, y;
    int32_t id;
    int32_t rank;

    float operator[](int i) {
        return (&x)[i];
    }
};

struct Query
{
    float lx, ly;
    float hx, hy;

    float min(int dim) const {
        return (&lx)[dim];
    }
    float max(int dim) const {
        return (&hx)[dim];
    }
    float &min(int dim) {
        return (&lx)[dim];
    }
    float &max(int dim) {
        return (&hx)[dim];
    }
};

struct KdNode {
    float x, y;
    int32 minRank;
    int32 rank, id;
    uint32 childDescriptor;

    bool isLeaf() const {
        return (childDescriptor & 0x80000000u) != 0;
    }
    bool hasLeftChild() const {
        return (childDescriptor & 0x40000000u) != 0;
    }
    bool hasRightChild() const {
        return (childDescriptor & 0x20000000u) != 0;
    }
    int splitPlane() const {
        return (childDescriptor >> 28) & 1;
    }
    uint32 childIndex() const {
        return childDescriptor & 0x0FFFFFFFu;
    }

    void setLeaf(bool enabled) {
        if (enabled) childDescriptor |=  0x80000000u;
        else         childDescriptor &= ~0x80000000u;
    }
    void setLeftChild(bool enabled) {
        if (enabled) childDescriptor |=  0x40000000u;
        else         childDescriptor &= ~0x40000000u;
    }
    void setRightChild(bool enabled) {
        if (enabled) childDescriptor |=  0x20000000u;
        else         childDescriptor &= ~0x20000000u;
    }
    void setSplitPlane(int plane) {
        if (plane) childDescriptor |=  0x10000000u;
        else       childDescriptor &= ~0x10000000u;
    }
    void setChildIndex(uint32 index) {
        childDescriptor = (childDescriptor & 0xF0000000) | index;
    }
    void fromPoint(const AlignedPoint &p) {
        x = p.x;
        y = p.y;
        minRank = rank = p.rank;
        id = p.id;
    }

    float operator[](int i) const {
        return (&x)[i];
    }
};

class KdTree {
    const int RecursionLimit = 125;

    float _minX, _minY;
    float _maxX, _maxY;

    int _childCount;
    std::unique_ptr<KdNode[]> _tree;

    void recursiveBuild(int dst, int start, int end, std::vector<AlignedPoint> &points)
    {
        float minX = points[start].x, maxX = points[start].x;
        float minY = points[start].y, maxY = points[start].y;
        int minRank = points[start].rank;
        for (int i = start + 1; i < end; ++i) {
            minX = min(minX, points[i].x);
            minY = min(minY, points[i].y);
            maxX = max(maxX, points[i].x);
            maxY = max(maxY, points[i].y);
            minRank = min(minRank, points[i].rank);
        }

        int dim = (maxX - minX) > (maxY - minY) ? 0 : 1;
        std::sort(points.begin() + start, points.begin() + end, [&](AlignedPoint a, AlignedPoint b) {
            return a[dim] < b[dim];
        });

        int childIndex = _childCount;
        int node;
        if (end - start <= RecursionLimit)
            node = start;
        else
            node = (end + start)/2;
        _tree[dst].fromPoint(points[node]);
        _tree[dst].minRank = minRank;
        _tree[dst].setChildIndex(childIndex);
        _tree[dst].setSplitPlane(dim);
        _tree[dst].setLeftChild(false);
        _tree[dst].setRightChild(false);
        _tree[dst].setLeaf(false);

        if (end - start <= RecursionLimit) {
            if (end - start > 1)
                _tree[dst].setRightChild(true);

            std::sort(points.begin() + start + 1, points.begin() + end, [&](AlignedPoint a, AlignedPoint b) {
                return a.rank < b.rank;
            });

            for (int i = start + 1; i < end; ++i) {
                _tree[_childCount].fromPoint(points[i]);
                _tree[_childCount].setLeaf(true);
                _childCount++;
            }
            _tree[_childCount - 1].setLeaf(false);
            _tree[_childCount - 1].setLeftChild(false);
            _tree[_childCount - 1].setRightChild(false);
            return;
        }

        _tree[dst].setLeftChild(true);

        if (end - start > 2) {
            _tree[dst].setRightChild(true);
            _childCount += 2;

            recursiveBuild(childIndex + 1, node + 1, end, points);
        } else {
            _childCount++;
        }
        recursiveBuild(childIndex, start, node, points);
    }

public:
    inline int rangeQuery(Query rect, const int k, Point *result) const
    {
        if (!_tree || _minX > rect.hx || _minY > rect.hy || _maxX < rect.lx || _maxY < rect.ly)
            return 0;

        int stack[32];
        int *stackPtr = stack;

        int nodeIdx = 0;

        int *buffer = reinterpret_cast<int *>(result);
        int *ranks = buffer + k + 1;

        int pointCount = 0, maxRank = 0x7FFFFFFF;

        while (true) {
            const KdNode *node;
reiterate:
            do {
                node = &_tree[nodeIdx++];
                if (node->minRank > maxRank) {
                    if (stackPtr == stack) {
                        goto finish;
                    } else {
                        nodeIdx = *--stackPtr;
                        goto reiterate;
                    }
                }

                if (node->rank < maxRank && node->x >= rect.lx && node->y >= rect.ly && node->x <= rect.hx && node->y <= rect.hy) {
                    int i;
                    for (i = pointCount; i > 0; --i) {
                        if (ranks[i - 1] < node->rank)
                            break;
                        ranks[i] = ranks[i - 1];
                        buffer[i] = buffer[i - 1];
                    }
                    ranks[i] = node->rank;
                    buffer[i] = nodeIdx - 1;
                    pointCount = min(pointCount + 1, k);
                    if (pointCount == k)
                        maxRank = ranks[k - 1];
                }
            } while (node->isLeaf());
            nodeIdx--;

            int dim = node->splitPlane();
            bool traverseLeft  = (rect.min(dim) <= (*node)[dim]) && node->hasLeftChild();
            bool traverseRight = (rect.max(dim) >= (*node)[dim]) && node->hasRightChild();

            uint32 idx = node->childIndex();
            if (traverseLeft && traverseRight) {
                *stackPtr++ = idx;
                nodeIdx = idx + 1;
            } else if (traverseLeft || traverseRight) {
                nodeIdx = idx + (traverseRight && node->hasLeftChild());
            } else {
                if (stackPtr == stack)
                    break;
                else
                    nodeIdx = *--stackPtr;
            }
        }

finish:
        for (int i = pointCount - 1; i >= 0; --i) {
            int nodeIdx = buffer[i];
            result[i].x = _tree[nodeIdx].x;
            result[i].y = _tree[nodeIdx].y;
            result[i].rank = _tree[nodeIdx].rank;
            result[i].id   = _tree[nodeIdx].id;
        }

        return pointCount;
    }

    KdTree(std::vector<AlignedPoint> points)
    : _childCount(1)
    {
        if (points.empty())
            return;

        _minX = points[0].x, _maxX = points[0].x;
        _minY = points[0].y, _maxY = points[0].y;
        for (size_t i = 1; i < points.size(); ++i) {
            _minX = min(_minX, points[i].x);
            _minY = min(_minY, points[i].y);
            _maxX = max(_maxX, points[i].x);
            _maxY = max(_maxY, points[i].y);
        }

        _tree.reset(new KdNode[points.size()]);

        recursiveBuild(0, 0, int(points.size()), points);
    }
};

struct SearchContext {
    std::unique_ptr<KdTree> tree;
};

SearchContext *__stdcall create(const Point* points_begin, const Point* points_end)
{
    std::vector<AlignedPoint> points(points_end - points_begin);
    for (const Point *i = points_begin; i != points_end; ++i)
        points[i - points_begin] = AlignedPoint{i->x, i->y, int32(i->id), i->rank};

    return new SearchContext{std::unique_ptr<KdTree>{new KdTree(std::move(points))}};
}

int32_t __stdcall search(SearchContext *sc, const Rect rect, const int32_t count, Point* out_points)
{
    int c = sc->tree->rangeQuery(Query{rect.lx, rect.ly, rect.hx, rect.hy}, count, out_points);

    return c;
}

SearchContext *__stdcall destroy(SearchContext* sc)
{
    delete sc;
    return nullptr;
}
