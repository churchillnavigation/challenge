// Entry for the Churchill Navigation contest.

// We assume that floats are IEEE 754 binary32 floating-point numbers.

#include "common_interface.hh"

#include <cassert>
#include <cstdint>
#include <cstring>

#include <algorithm>
#include <array>
#include <limits>
#include <vector>

#include "float_surgery.hh"
#include "it_range.hh"
#include "restricted_algorithm.hh"

#ifdef INSTRUMENT
#include <iostream>
#include <map>

#define MY_SIGTRAP do {__asm__ __volatile__ ("int3; nop;");} while(false)

#endif

#ifndef ZONE_INLINE_CAP
#define ZONE_INLINE_CAP 500
#endif

#ifndef ZONE_BUCKET_CAP
#define ZONE_BUCKET_CAP 10000
#endif

// (4 * ZONE_INLINE_CAP) <= ZONE_BUCKET_CAP

struct alignas(16) point
{
    int32_t id;
    int32_t rank;
    ieee754_b32 x;
    ieee754_b32 y;
};

inline uint64_t z_ordinal_for_point(const point &p)
{
    return bit_zip(
        static_cast<uint64_t>(p.y.bit) << 32 | static_cast<uint64_t>(p.x.bit)
    );
}

inline void unpack_into(const pack_point &p, point &u)
{
    u.id = p.id;
    u.rank = p.rank;
    u.x.flt = p.x;
    u.y.flt = p.y;
}

inline void pack_into(const point &u, pack_point &p)
{
    p.id = static_cast<int32_t>(u.id);
    p.rank = u.rank;
    p.x = u.x.flt;
    p.y = u.y.flt;
}

inline pack_point pack(const point &u)
{
    pack_point ret;
    pack_into(u, ret);
    return ret;
}

struct rect
{
    ieee754_b32 lx;
    ieee754_b32 ly;
    ieee754_b32 hx;
    ieee754_b32 hy;
};

inline rect unpack(const pack_rect &r)
{
    return {{.flt = r.lx}, {.flt = r.ly}, {.flt = r.hx}, {.flt = r.hy}};
}

inline pack_rect pack(const rect &r)
{
    return {r.lx.flt, r.ly.flt, r.hx.flt, r.hy.flt};
}

struct mag_rect
{
    ieee754_b32 x_sml;
    ieee754_b32 x_big;
    ieee754_b32 y_sml;
    ieee754_b32 y_big;
};

inline bool contains_asq(const mag_rect &r, const point &p)
{
    return r.x_sml.bit <= p.x.bit
        && p.x.bit <= r.x_big.bit
        && r.y_sml.bit <= p.y.bit
        && p.y.bit <= r.y_big.bit;
}

struct zone
{
    uint64_t choices;
    uint64_t mask;
};

inline bool operator==(const zone &a, const zone &b)
{
    return a.choices == b.choices && a.mask == b.mask;
}

#ifdef INSTRUMENT
template<class OStream>
OStream& operator<<(OStream &out, const zone &z)
{
    auto saveflags = out.flags();
    out << "{choices:" << std::hex << z.choices << " mask:" << std::hex << z.mask << "}";
    out.flags(saveflags);
    return out;
}
#endif

/// Default partial ordering on zones.
///
/// By default zones, are z-ordered (or Morton-ordered).
///
/// This is a convenient choice for zone partial ordering.  It allows use
/// std::lower_bound and std::upper_bound to select a range of respondent zones.
inline bool operator<(const zone &a, const zone &b)
{
    return (a.choices & a.mask) < (b.choices & a.mask);
}

inline bool lessp_total_order(const zone &a, const zone &b)
{
    if((a.choices & a.mask) == (b.choices & b.mask))
    {
        return a.mask < b.mask;
    }
    else
    {
        return (a.choices & a.mask) < (b.choices & b.mask);
    }
}

inline bool is_tzone(const zone &z)
{
    return z.mask == 0xfffffffffffffffful;
}

inline zone parent(const zone &child)
{
    return {
        child.choices & (child.mask << 2),
        child.mask << 2
    };
}

inline zone parent_same_quadrant(const zone &child)
{
    if(child.mask > 0xC000000000000000ul)
    {
        return parent(child);
    }
    else
    {
        return child;
    }
}

inline zone child(const zone &parent, unsigned int which)
{
    const std::array<uint64_t, 4> child_selectors = {
        0x0000000000000000ul, 0x5555555555555555ul, 0xaaaaaaaaaaaaaaaaul, 0xfffffffffffffffful
    };
    const uint64_t new_choice_mask = ~(parent.mask | (~parent.mask >> 2));

    return {
        parent.choices | (child_selectors[which] & new_choice_mask),
        0xfffffffffffffffful & ~((~parent.mask) >> 2)
    };
}

inline uint64_t lo_tzone(const zone &z)
{
    return z.choices & z.mask;
}

inline uint64_t hi_tzone(const zone &z)
{
   return (z.choices & z.mask) | (0xfffffffffffffffful & ~z.mask);
}

/// Get the edges of a quadrant-confined zone.
///
/// Runtime arguments:
///
///   * z (zone): The subject zone.
///
/// Return value:
///
///   A mag_rect containing the four edges of the zone:
///
///   The values are returned in a relaxed variant of IEEE754 binary32 that does
///   not recognize infinities or NaNs.  In particular, the outermost zones at
///   any particular level WILL have NaNs of some kind as their larger edges.
///
///   This means that, if the resulting values are converted back to floats, NO
///   ARITHMETIC should be performed on them.
///
/// Preconditions:
///
///   * z must be confined to a single quadrant.  Equivalently, its mask must be
///     at least 0xC0000000.
inline mag_rect zone_edges(const zone &z)
{
    const auto choices = bit_uzp(z.choices);
    const auto masks = bit_uzp(z.mask);

    const uint32_t x_choices = choices & 0x00000000fffffffful;
    const uint32_t x_mask    = masks   & 0x00000000fffffffful;

    const uint32_t y_choices = (choices >> 32) & 0x00000000fffffffful;
    const uint32_t y_mask    = (masks   >> 32) & 0x00000000fffffffful;

    return {
        make_ieee754_b32(x_choices & x_mask),
        make_ieee754_b32((x_choices & x_mask) | ~x_mask),
        make_ieee754_b32(y_choices & y_mask),
        make_ieee754_b32((y_choices & y_mask) | ~y_mask)
    };
}

inline bool contains(const zone &a, const zone &b)
{
    return ((a.choices & a.mask) == (b.choices & a.mask))
        && (a.mask <= b.mask);
}

namespace interaction
{
enum detail : unsigned int
{
    none = 0, intersects = 1, contains = 2
};
}

typedef unsigned int interaction_t;

/// Compute the interaction of a with b, assuming same quadrant.
inline interaction_t interaction_asq(
    const mag_rect &a,
    const mag_rect &b
) {
    bool contains = (a.x_sml.bit <= b.x_sml.bit)
        && (a.y_sml.bit <= b.y_sml.bit)
        && (a.x_big.bit >= b.x_big.bit)
        && (a.y_big.bit >= b.y_big.bit);

    bool intersects = !(
        a.x_sml.bit > b.x_big.bit
        || a.x_big.bit < b.x_sml.bit
        || a.y_sml.bit > b.y_big.bit
        || a.y_big.bit < b.y_sml.bit
    );

    return (contains ? interaction::contains : 0)
        | (intersects ? interaction::intersects : 0);
}

inline interaction_t interaction_asq(const mag_rect &r, const zone &z)
{
    return interaction_asq(r, zone_edges(z));
}

struct zone_mapping
{
    zone z;
    std::vector<point>::const_iterator src;
    std::vector<point>::const_iterator lim;
};

inline bool operator<(const zone_mapping &a, const zone_mapping &b)
{
    return a.z < b.z;
}

inline bool operator==(const zone_mapping &a, const zone_mapping &b)
{
    return a.z == b.z
        && a.src == b.src
        && a.lim == b.lim;
}

struct quad_tree
{
    std::vector<point> point_storage;
    std::vector<zone_mapping> leaf_zones;
};

template<class It, class Equal>
It unique_to_n(
    It a_src, It a_lim,
    std::ptrdiff_t n,
    Equal equal
)
{
    if(a_src == a_lim)
    {
        return a_lim;
    }

    std::ptrdiff_t counter = 0;

    It out = a_src;
    ++a_src;
    while(a_src != a_lim)
    {
        if(equal(*out, *a_src))
        {
            if(counter < n)
            {
                ++counter;
                ++out;
                *out = *a_src;
                ++a_src;
            }
            else
            {
                ++counter;
                ++a_src;
            }
        }
        else
        {
            counter = 0;
            ++out;
            *out = *a_src;
            ++a_src;
        }
    }

    ++out;
    return out;
}

inline quad_tree make_quad_tree(std::vector<point>&& points)
{
    quad_tree result;
    result.point_storage = points;

    // Sort the points in leaf order, and then lexicographically further by
    // rank.
    std::sort(
        result.point_storage.begin(),
        result.point_storage.end(),
        [](const point &a, const point &b){
            return (z_ordinal_for_point(a) != z_ordinal_for_point(b))
                ? (z_ordinal_for_point(a) < z_ordinal_for_point(b))
                : a.rank < b.rank;
        }
    );

    // Cut all duplicated points.
    auto new_point_lim = unique_to_n(
        result.point_storage.begin(),
        result.point_storage.end(),
        20,
        [](const point &a, const point &b) {
            return a.x.bit == b.x.bit && a.y.bit == b.y.bit;
        }
    );
    result.point_storage.erase(new_point_lim, result.point_storage.end());


    // Repeatedly subdivide zones that are overfull.  Subdivided zones are not
    // removed.
    std::vector<zone> zones_to_check = {
        child({0x0ul, 0x0ul}, 3),
        child({0x0ul, 0x0ul}, 2),
        child({0x0ul, 0x0ul}, 1),
        child({0x0ul, 0x0ul}, 0)
    };
    while(! zones_to_check.empty())
    {
        zone check_zone = zones_to_check.back();
        zones_to_check.pop_back();

        // Find the points that this zone covers.  We exploit the fact that the
        // points have been sorted into z-order.

        auto check_range_src = std::lower_bound(
            result.point_storage.begin(),
            result.point_storage.end(),
            lo_tzone(check_zone),
            [](const point &p, uint64_t t) {return z_ordinal_for_point(p) < t;}
        );

        auto check_range_lim = std::upper_bound(
            check_range_src,
            result.point_storage.end(),
            hi_tzone(check_zone),
            [](uint64_t t, const point &p) {return t < z_ordinal_for_point(p);}
        );

        auto check_range_size = check_range_lim - check_range_src;

        if(check_range_size == 0)
        {
            continue;
        }
        else if(check_range_size <= ZONE_INLINE_CAP)
        {
            // This zone doesn't contain enough points to merit its own entry in
            // the leaf table.  Rather, the current range will be given as an
            // entry for the parent zone.

            std::sort(
                check_range_src,
                check_range_lim,
                [](const point &a, const point &b){return a.rank < b.rank;}
            );

            result.leaf_zones.push_back(
                {
                    parent_same_quadrant(check_zone),
                    check_range_src,
                    check_range_lim
                }
            );
        }
        else if(is_tzone(check_zone) || check_range_size <= ZONE_BUCKET_CAP)
        {
            // Either this zone is not overfull, or it is a leaf.  Either way,
            // it will not be given children.  Sort its points by rank, so that
            // we can use fast merge algorithms in the lookup phase.
            std::sort(
                check_range_src,
                check_range_lim,
                [](const point &a, const point &b){return a.rank < b.rank;}
            );

            // Add this childless zone to the set of childless zones.
            result.leaf_zones.push_back(
                {check_zone, check_range_src, check_range_lim}
            );
        }
        else
        {
            // Add children to the working stack in reverse z-order.  This way,
            // the children will be visited in forward z-order.

            zones_to_check.push_back(child(check_zone, 3));
            zones_to_check.push_back(child(check_zone, 2));
            zones_to_check.push_back(child(check_zone, 1));
            zones_to_check.push_back(child(check_zone, 0));
        }
    }

    std::sort(
        result.leaf_zones.begin(),
        result.leaf_zones.end(),
        [](const zone_mapping &a, const zone_mapping &b){
            return lessp_total_order(a.z, b.z);
        }
    );

    #ifdef INSTRUMENT
    for(const zone_mapping &mapping : result.leaf_zones)
    {
        std::cout << mapping.z << " size: " << (mapping.lim - mapping.src) << std::endl;
    }

    std::cout << "Total leaves: " << result.leaf_zones.size() << std::endl;
    #endif

    return result;
}

/// A zone paired with a range of the qtree's leaf table.
///
/// Maintaining the range allows us to accelerate our searches as we descend
/// down the qtree.
struct zone_with_leaf_range
{
    zone z;
    std::vector<zone_mapping>::const_iterator check_src;
    std::vector<zone_mapping>::const_iterator check_lim;

    inline zone_with_leaf_range() = default;

    inline zone_with_leaf_range(
        const zone &z_in,
        const std::vector<zone_mapping>::const_iterator check_src_in,
        const std::vector<zone_mapping>::const_iterator check_lim_in
    )
        : z(z_in),
          check_src(check_src_in),
          check_lim(check_lim_in)
    {
    }
};

/// Scratch storage space for a single lookup.
struct quad_scratch
{
    // Since there are only 32 levels in the quadtree, and use depth-first
    // search, this is the maximum number of elements we will need on the stack.
    std::array<zone_with_leaf_range, 128> work_stack;

    std::vector<point> front_buf;
    std::vector<point> back_buf;

    // The ranges used in our repeated merges.  The front range is always a
    // source buffer, and the back range is always an output buffer.  Thus, the
    // front range is initialized empty and the back range is initialized full.
    std::vector<point>::iterator f_src    ;
    std::vector<point>::iterator f_cur_lim;
    std::vector<point>::iterator f_abs_lim;
    std::vector<point>::iterator b_src    ;
    std::vector<point>::iterator b_cur_lim;
    std::vector<point>::iterator b_abs_lim;

    inline quad_scratch(std::size_t exp_query_size)
        : front_buf(exp_query_size),
          back_buf(exp_query_size)
    {
        // Front range empty.
        f_src     = front_buf.begin();
        f_cur_lim = front_buf.begin();
        f_abs_lim = front_buf.end();

        // Back range full.
        b_src     = back_buf.begin();
        b_cur_lim = back_buf.end();
        b_abs_lim = back_buf.end();
    }
};

/// Create a quad_tree scratch storage for lookups.
///
/// The amount of storage space is tuned to the given quad tree and the expected
/// query size.
inline quad_scratch make_quad_scratch(
    const quad_tree &tree,
    std::size_t exp_query_size
) {
    return (quad_scratch(exp_query_size));
}

inline void flip_ranges(quad_scratch &scratch)
{
    using std::swap;
    swap(scratch.f_src,     scratch.b_src);
    swap(scratch.f_cur_lim, scratch.b_cur_lim);
    swap(scratch.f_abs_lim, scratch.b_abs_lim);
    scratch.b_cur_lim = scratch.b_abs_lim;
}

///
///
/// Assumes that query and root_check_zone are in the same quadrant.
__attribute__((hot))
void points_in_rect_asq(
    const quad_tree &qtree,
    quad_scratch &scratch,
    const mag_rect &query,
    const zone &root_check_zone
) {
    auto work_stack_src = scratch.work_stack.begin();
    auto work_stack_lim = scratch.work_stack.begin();

    *work_stack_lim = zone_with_leaf_range(
        root_check_zone,
        qtree.leaf_zones.cbegin(),
        qtree.leaf_zones.cend()
    );
    ++work_stack_lim;

    while(work_stack_lim != work_stack_src)
    {
        --work_stack_lim;
        zone_with_leaf_range cur_zone = *work_stack_lim;

        auto how_int = interaction_asq(query, cur_zone.z);

        if(how_int & interaction::contains)
        {
            // Since the query rect fully contains this zone, we can
            // unconditionally use every point within it.

            // Find every leaf zone that is below this zone.
            auto leaf_src = std::lower_bound(
                cur_zone.check_src,
                cur_zone.check_lim,
                cur_zone.z,
                [](const zone_mapping &a, const zone &b){return a.z < b;}
            );

            auto leaf_lim = std::upper_bound(
                cur_zone.check_src,
                cur_zone.check_lim,
                cur_zone.z,
                [](const zone &a, const zone_mapping &b){return a < b.z;}
            );

            // We still need to access each leaf individually, since the leaves
            // are sorted so that we can use a simple merge.
            for(; leaf_src != leaf_lim; ++leaf_src)
            {
                scratch.b_cur_lim = restricted_merge(
                    (*leaf_src).src,
                    (*leaf_src).lim,
                    scratch.f_src,
                    scratch.f_cur_lim,
                    scratch.b_src,
                    scratch.b_cur_lim,
                    [](const point &a, const point &b){
                        return a.rank < b.rank;
                    }
                );

                flip_ranges(scratch);
            }
        }
        else if(how_int & interaction::intersects)
        {
            // The zone we are currently checking is a leaf zone.
            // Find every leaf zone that is below this zone.
            auto leaf_src = std::lower_bound(
                cur_zone.check_src,
                cur_zone.check_lim,
                cur_zone.z,
                [](const zone_mapping &a, const zone &b){return a.z < b;}
            );

            auto leaf_lim = std::upper_bound(
                cur_zone.check_src,
                cur_zone.check_lim,
                cur_zone.z,
                [](const zone &a, const zone_mapping &b){return a < b.z;}
            );

            // There may be several ranges at this level, either because this
            // zone is a leaf, or because it has some internal storage.
            for(;
                leaf_src != leaf_lim && (*leaf_src).z == cur_zone.z;
                ++leaf_src)
            {
                scratch.b_cur_lim = restricted_merge_if(
                    (*leaf_src).src,
                    (*leaf_src).lim,
                    scratch.f_src,
                    scratch.f_cur_lim,
                    scratch.b_src,
                    scratch.b_cur_lim,
                    [](const point &a, const point &b){return a.rank < b.rank;},
                    [&query](const point &p){
                        return contains_asq(query, p);
                    }
                );

                flip_ranges(scratch);
            }

            if((leaf_lim - leaf_src) == 0)
            {
                continue;
            }
            else
            {
                // The zone we are currently checking is not a leaf zone.
                // Push its direct children onto the working stack.

                *(work_stack_lim) = zone_with_leaf_range(
                    child(cur_zone.z, 3),
                    leaf_src,
                    leaf_lim
                );
                ++work_stack_lim;

                *(work_stack_lim) = zone_with_leaf_range(
                    child(cur_zone.z, 2),
                    leaf_src,
                    leaf_lim
                );
                ++work_stack_lim;

                *(work_stack_lim) = zone_with_leaf_range(
                    child(cur_zone.z, 1),
                    leaf_src,
                    leaf_lim
                );
                ++work_stack_lim;

                *(work_stack_lim) = zone_with_leaf_range(
                    child(cur_zone.z, 0),
                    leaf_src,
                    leaf_lim
                );
                ++work_stack_lim;
            }
        }
        else
        {
            // No interaction.  Let it die.
            continue;
        }
    }
}

it_range<std::vector<point>::iterator> points_in_rect(
    const quad_tree &qtree,
    quad_scratch &scratch,
    const rect &query,
    const std::size_t count
) {
    // Bump out our scratch buffer sizes if we need to.
    if(scratch.front_buf.size() < count)
    {
        scratch.front_buf.resize(count);
        scratch.back_buf.resize(count);

        scratch.f_src = scratch.front_buf.begin();
        scratch.f_abs_lim = scratch.front_buf.end();

        scratch.b_src = scratch.back_buf.begin();
        scratch.b_abs_lim = scratch.back_buf.end();
    }

    // Reset scratch ranges
    scratch.f_cur_lim = scratch.f_src;
    scratch.b_cur_lim = scratch.b_abs_lim;

    // Cut the query rect into four pieces (one for each quadrant, according to
    // their floating point sign bits).

    std::array<zone, 4> root_zones;
    auto root_zones_src = root_zones.begin();
    auto root_zones_lim = root_zones.begin();

    std::array<mag_rect, 4> queries;
    auto queries_src = queries.begin();
    auto queries_lim = queries.begin();

    if(query.hx.flt >= 0.0f && query.hy.flt >= 0.0f)
    {
        *queries_lim = {
            make_ieee754_b32(std::max(0.0f, query.lx.flt)),
            make_ieee754_b32(std::max(0.0f, query.hx.flt)),
            make_ieee754_b32(std::max(0.0f, query.ly.flt)),
            make_ieee754_b32(std::max(0.0f, query.hy.flt))
        };
        ++queries_lim;

        *root_zones_lim = {0x0000000000000000ul, 0xc000000000000000ul};
        ++root_zones_lim;
    }
    if(query.lx.flt <= -0.0f && query.hy.flt >= 0.0f)
    {
        *queries_lim = {
            make_ieee754_b32(std::min(-0.0f, query.hx.flt)),
            make_ieee754_b32(std::min(-0.0f, query.lx.flt)),
            make_ieee754_b32(std::max(0.0f, query.ly.flt)),
            make_ieee754_b32(std::max(0.0f, query.hy.flt))
        };
        ++queries_lim;

        *root_zones_lim = {0x4000000000000000ul, 0xc000000000000000ul};
        ++root_zones_lim;
    }
    if(query.hx.flt >= 0.0f && query.ly.flt <= -0.0f)
    {
        *queries_lim = {
            make_ieee754_b32(std::max(0.0f, query.lx.flt)),
            make_ieee754_b32(std::max(0.0f, query.hx.flt)),
            make_ieee754_b32(std::min(-0.0f, query.hy.flt)),
            make_ieee754_b32(std::min(-0.0f, query.ly.flt))
        };
        ++queries_lim;

        *root_zones_lim = {0x8000000000000000ul, 0xc000000000000000ul};
        ++root_zones_lim;
    }
    if(query.lx.flt <= -0.0f && query.ly.flt <= -0.0f)
    {
        *queries_lim = {
            make_ieee754_b32(std::min(-0.0f, query.hx.flt)),
            make_ieee754_b32(std::min(-0.0f, query.lx.flt)),
            make_ieee754_b32(std::min(-0.0f, query.hy.flt)),
            make_ieee754_b32(std::min(-0.0f, query.ly.flt))
        };
        ++queries_lim;

        *root_zones_lim = {0xc000000000000000ul, 0xc000000000000000ul};
        ++root_zones_lim;
    }

    for(; queries_src != queries_lim; ++queries_src, ++root_zones_src)
    {
        points_in_rect_asq(qtree, scratch, *queries_src, *root_zones_src);
    }

    // We have collected every respondent point into the current front buffer.
    return {scratch.f_src, scratch.f_cur_lim};
}

struct SearchContext
{
    quad_tree tree;
    quad_scratch scratch;

    SearchContext(quad_tree&& tree_in, quad_scratch&& scratch_in)
        : tree(std::forward<quad_tree>(tree_in)),
          scratch(std::forward<quad_scratch>(scratch_in))
    {
    }
};

SearchContext* create(
    const pack_point *p_src,
    const pack_point *p_lim
) {
    std::vector<point> unpacked_points(p_lim - p_src);
    transform_into(
        p_src,
        p_lim,
        unpacked_points.begin(),
        static_cast<void(*)(const pack_point &, point &)>(unpack_into)
    );

    quad_tree tree = make_quad_tree(std::move(unpacked_points));
    quad_scratch scratch = make_quad_scratch(tree, 20);

    SearchContext *sc = new SearchContext(std::move(tree), std::move(scratch));
    return sc;
}

int32_t search(
    SearchContext *sc,
    const pack_rect query,
    const int32_t count,
    pack_point *out_src
) {
    auto answer_range = points_in_rect(
        sc->tree,
        sc->scratch,
        unpack(query),
        count
    );

    // Repack our answer points.
    transform_into(
        answer_range.src,
        answer_range.lim,
        out_src,
        static_cast<void(*)(const point &, pack_point&)>(pack_into)
    );

    return size(answer_range);
}

SearchContext* destroy(SearchContext *sc)
{
    delete sc;
    return nullptr;
}
