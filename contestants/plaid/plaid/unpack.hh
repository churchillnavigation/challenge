#ifndef UNPACK_HH
#define UNPACK_HH

#include <cstdint>

#include "common_interface.hh"
#include "float_surgery.hh"

struct point
{
    int8_t id;
    int32_t rank;
    ieee754_b32 x;
    ieee754_b32 y;
};

bool operator<(const point &a, const point &b)
{
    return a.rank < b.rank;
}

bool operator==(const point &a, const point &b)
{
    return a.id == b.id
        && a.rank == b.rank
        && a.x.bit == b.x.bit
        && a.y.bit == b.y.bit;
}

template<class OStream>
OStream& operator<<(OStream &out, const point &p)
{
    out << "{"
        << "id: " << static_cast<int>(p.id) << " "
        << "rank: " << p.rank << " "
        << "x: " << p.x.flt << " "
        << "y: " << p.y.flt
        << "}";
    return out;
}

inline void unpack_into(const pack_point &p, point &u)
{
    u.id = p.id;
    u.rank = p.rank;
    u.x.flt = p.x;
    u.y.flt = p.y;
}

inline point unpack(const pack_point &p)
{
    point ret;
    unpack_into(p, ret);
    return ret;
}

inline void pack_into(const point &u, pack_point &p)
{
    p.id = u.id;
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

uint32_t z_ordinal_for_point(const point &p)
{
    return bit_ilv_upper(p.x.bit, p.y.bit);
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

inline bool intersects(const rect &a, const rect &b)
{
    return !(a.lx.flt > b.hx.flt || a.hx.flt < b.lx.flt || a.ly.flt > b.hy.flt || a.hy.flt < b.ly.flt);
}

inline bool contains(const rect &r, const point &p)
{
    return r.lx.flt <= p.x.flt
        && p.x.flt  <= r.hx.flt
        && r.ly.flt <= p.y.flt
        && p.y.flt  <= r.hy.flt;
}

inline bool contains(const rect &a, const rect &b)
{
    return (a.lx.flt <= b.lx.flt)
        && (a.ly.flt <= b.ly.flt)
        && (a.hx.flt >= b.hx.flt)
        && (a.hy.flt >= b.hy.flt);
}

#endif
