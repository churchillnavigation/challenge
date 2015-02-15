#ifndef COMMON_INTERFACE_HH
#define COMMON_INTERFACE_HH

#include <cstdint>

#pragma pack(push, 1)

struct pack_point
{
    int8_t id;
    int32_t rank;
    float x;
    float y;
};

struct pack_rect
{
    float lx;
    float ly;
    float hx;
    float hy;
};

#pragma pack(pop)

inline bool operator==(const pack_point &a, const pack_point &b)
{
    return a.id == b.id && a.rank == b.rank && a.x == b.x && a.y == b.y;
}

template <class OStream>
OStream& operator<<(OStream &out, const pack_point &p)
{
    out << "{"
        << "id: " << static_cast<int>(p.id) << " "
        << "rank: " << p.rank << " "
        << "x: " << p.x << " "
        << "y: " << p.y
        << "}";
    return out;
}

template <class OStream>
OStream& operator<<(OStream &out, const pack_rect &r)
{
    out << "{"
        << "lx: " << r.lx << " "
        << "hx: " << r.hx << " "
        << "ly: " << r.ly << " "
        << "hy: " << r.hy
        << "}";
    return out;
}

struct SearchContext;

typedef SearchContext* (* create_fn_t)(
    const pack_point* points_begin,
    const pack_point* points_end
);

typedef int32_t (* search_fn_t)(
    SearchContext* sc,
    const pack_rect rect,
    const int32_t count,
    pack_point* out_points
);

typedef SearchContext* (* destroy_fn_t)(SearchContext* sc);

extern "C" {

// Load the provided points into an internal data structure. The pointers follow
// the STL iterator convention, where "points_begin" points to the first
// element, and "points_end" points to one past the last element. The input
// points are only guaranteed to be valid for the duration of the call. Return a
// pointer to the context that can be used for consecutive searches on the data.
__attribute__((visibility ("default")))
SearchContext* create(
    const pack_point *p_src,
    const pack_point *p_lim
);

// Search for "count" points with the smallest ranks inside "rect" and copy them
// ordered by smallest rank first in "out_points". Return the number of points
// copied. "out_points" points to a buffer owned by the caller that can hold
// "count" number of Points.
__attribute__((visibility ("default")))
int32_t search(
    SearchContext *sc,
    const pack_rect rect,
    const int32_t count,
    pack_point *out_points
);

// Release the resources associated with the context. Return nullptr if
// successful, "sc" otherwise.
__attribute__((visibility ("default")))
SearchContext* destroy(
    SearchContext *sc
);

}

#endif
