#include <algorithm>

#include "common_interface.hh"
#include "restricted_algorithm.hh"
#include "unpack.hh"

#include <iostream>

struct SearchContext
{
    std::vector<point> points;
};

SearchContext* create(const pack_point *p_src, const pack_point *p_lim)
{
    SearchContext *sc = new SearchContext();
    sc->points.resize(p_lim - p_src);

    std::transform(
        p_src,
        p_lim,
        sc->points.begin(),
        static_cast<point(*)(const pack_point&)>(unpack)
    );

    return sc;
}

int32_t search(
    SearchContext *sc,
    const pack_rect query_packed,
    const int32_t count,
    pack_point *p_src
) {
    rect query = unpack(query_packed);

    auto match_lim = std::partition(
        sc->points.begin(),
        sc->points.end(),
        [&](const point &p){return contains(query, p);}
    );

    std::sort(
        sc->points.begin(),
        match_lim,
        [](const point &a, const point &b){return a.rank < b.rank;}
    );

    auto p_lim = restricted_transform(
        sc->points.begin(),
        match_lim,
        p_src,
        p_src + count,
        static_cast<pack_point(*)(const point&)>(pack)
    );

    return p_lim - p_src;
}

SearchContext* destroy(SearchContext *sc)
{
    delete sc;
    return nullptr;
}
