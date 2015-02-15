#include <algorithm>
#include <cstddef>
#include <memory>

#include "point_search.h"
#include "search.h"

// -------------------------------------------------------------------------------------------------
using std::unique_ptr;
using std::make_unique;
using std::copy;


// -------------------------------------------------------------------------------------------------
extern "C" {
    SearchContext* create(const Point* points_begin, const Point* points_end);
    SearchContext* destroy(SearchContext* sc);
    int32_t search(SearchContext* sc, const Rect rect, const int32_t count, Point* out);
}

struct SearchContext {
    unique_ptr<Point[]> data;
    unique_ptr<Partition> root;
};


// -------------------------------------------------------------------------------------------------
SearchContext* create(const Point* begin, const Point* end) {
    if (begin == nullptr || end == nullptr) return nullptr;

    ptrdiff_t count = end - begin;
    unique_ptr<Point[]> data_copy = make_unique<Point[]>(count);

    copy(begin, end, data_copy.get());
    return new SearchContext {
        .data = move(data_copy),
        .root = create_partition(data_copy.get(), data_copy.get() + count)
    };
}


SearchContext* destroy(SearchContext* sc) {
    delete sc;
    return nullptr;
}


int32_t search(SearchContext* sc, const Rect rect, const int32_t count, Point* out) {
    if (sc == nullptr || out == nullptr || count <= 0) return 0;

    SearchResultHolder holder = SearchResultHolder {
        .data = out,
        .size = 0,
        .max_size = (size_t) count
    };

    search(*(sc->root), rect, holder);
    return holder.size;
}
