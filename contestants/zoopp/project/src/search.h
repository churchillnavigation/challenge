#pragma once

#include <memory>

#include "point_search.h"

struct Partition {
    const Rect bounding_box;
    std::unique_ptr<Partition> subpartition_1;
    std::unique_ptr<Partition> subpartition_2;

    const Point* begin;
    const Point* end;
};


struct SearchResultHolder {
    Point* data;
    size_t size;
    size_t max_size;
};


std::unique_ptr<Partition> create_partition(Point* begin, Point* end);
void search(const Partition& root, const Rect box, SearchResultHolder& out);
