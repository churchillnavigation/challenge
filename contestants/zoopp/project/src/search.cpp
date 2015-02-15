#include <algorithm>
#include <cstddef>
#include "search.h"

// -------------------------------------------------------------------------------------------------
using std::min;
using std::max;
using std::sort;
using std::lower_bound;
using std::make_unique;


// -------------------------------------------------------------------------------------------------
const ptrdiff_t MAX_PARTITON_DATA_SIZE = 16384;
static auto x_comparator = [] (const Point& lhs, const Point& rhs) { return lhs.x < rhs.x; };
static auto y_comparator = [] (const Point& lhs, const Point& rhs) { return lhs.y < rhs.y; };
static auto rank_comparator = [] (const Point& lhs, const Point& rhs) { return lhs.rank < rhs.rank; };


// -------------------------------------------------------------------------------------------------
std::unique_ptr<Partition> create_partition_impl(Point* begin, Point* end, uint32_t depth);
std::unique_ptr<Partition> create_node_partition(Point* begin, Point* end, uint32_t depth);
std::unique_ptr<Partition> create_leaf_partition(Point* begin, Point* end);
Rect compute_bounding_box(const Point* begin, const Point* end);


// -------------------------------------------------------------------------------------------------
std::unique_ptr<Partition> create_partition(Point* begin, Point* end) {
    return create_partition_impl(begin, end, 0);
}

std::unique_ptr<Partition> create_partition_impl(Point* begin, Point* end, uint32_t depth) {
    if (end - begin <= MAX_PARTITON_DATA_SIZE) {
        return create_leaf_partition(begin, end);
    }
    return create_node_partition(begin, end, depth);
}


std::unique_ptr<Partition> create_node_partition(Point* begin, Point* end, uint32_t depth) {
    sort(begin, end, depth % 2 == 0? x_comparator : y_comparator);

    ptrdiff_t length = end - begin;
    ptrdiff_t pivot = length / 2;

    return make_unique<Partition>(Partition {
        .bounding_box = compute_bounding_box(begin, end),
        .subpartition_1 = create_partition_impl(begin, begin + pivot, depth + 1),
        .subpartition_2 = create_partition_impl(begin + pivot, end, depth + 1),
    });
}


std::unique_ptr<Partition> create_leaf_partition(Point* begin, Point* end) {
    sort(begin, end, rank_comparator);
    return make_unique<Partition>(Partition {
        .bounding_box = compute_bounding_box(begin, end),
        .subpartition_1 = nullptr,
        .subpartition_2 = nullptr,
        .begin = begin,
        .end = end
    });
}


Rect compute_bounding_box(const Point* begin, const Point* end) {
    Rect result = Rect {
        .lx = begin->x,
        .ly = begin->y,
        .hx = begin->x,
        .hy = begin->y
    };

    for (const Point* p = begin + 1; p < end; ++p) {
        result.lx = min(result.lx, p->x);
        result.ly = min(result.ly, p->y);
        result.hx = max(result.hx, p->x);
        result.hy = max(result.hy, p->y);
    }

    return result;
}

// -------------------------------------------------------------------------------------------------
/**
 * Check if a and b intersect and return the result.
 */
bool intersects(const Rect a, const Rect b) {
    return (a.lx <= b.hx && a.hx >= b.lx &&
            a.ly <= b.hy && a.hy >= b.ly);
}


/**
 * Check if p is inside rect and return the result.
 */
bool contains(const Rect rect, const Point p) {
    return (rect.lx <= p.x && p.x <= rect.hx &&
            rect.ly <= p.y && p.y <= rect.hy);
}


void search(const Partition& root, const Rect box, SearchResultHolder& out) {
    if (!intersects(box, root.bounding_box)) return;
    if (root.begin == nullptr) {
        search(*root.subpartition_1, box, out);
        search(*root.subpartition_2, box, out);
        return;
    }

    for (const Point* it = root.begin; it != root.end; ++it) {
        if (out.size == out.max_size && it->rank > out.data[out.size - 1].rank) return;
        if (!contains(box, *it)) continue;

        Point* begin = out.data;
        Point* end = out.data + out.size;
        Point* insertion_it = lower_bound(begin, end, *it, rank_comparator);

        if (out.size < out.max_size) {
            if (insertion_it != end) {
                std::copy_backward(insertion_it, end, end + 1);
            }
            *insertion_it = *it;
            ++out.size;
       } else if (insertion_it != end) {
            std::copy_backward(insertion_it, end - 1, end);
            *insertion_it = *it;
       } else {
           return;
       }
    }
}
