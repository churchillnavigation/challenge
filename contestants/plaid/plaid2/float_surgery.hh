#ifndef FLOAT_SURGERY_HH
#define FLOAT_SURGERY_HH

#include <cstdint>

#include <array>

/// Welcome to Dr. Nick's standard-incompliant floating-point surgery lecture!
///
/// Hi, everybody!

union ieee754_b32
{
    float    flt;
    uint32_t bit;
};

ieee754_b32 make_ieee754_b32(float in)
{
    ieee754_b32 retval;
    retval.flt = in;
    return retval;
}

ieee754_b32 make_ieee754_b32(uint32_t in)
{
    ieee754_b32 retval;
    retval.bit = in;
    return retval;
}

inline uint32_t filter_negative_zero(uint32_t flt)
{
    return (flt & 0x7fffffff) ? flt : 0;
}

inline uint32_t sgn(uint32_t flt_bits)
{
    return flt_bits & 0x80000000;
}


inline bool relaxed_compare(uint32_t flta, uint32_t fltb)
{
    uint32_t maga = flta & 0x7fffffff;
    uint32_t magb = fltb & 0x7fffffff;

    // We do not need to handle distinguish between +/- 0.0f, because we filter
    // -0.0f out in our input phase (see unpack_into).

    if(sgn(flta) ^ sgn(fltb))
    {
        // The numbers have differing sign.
        return sgn(flta);
    }
    else
    {
        // The numbers have the same sign.
        return (sgn(flta) ? magb : maga) < (sgn(flta) ? maga : magb);
    }
}

///
///
/// Note:
///
///   * declare as constexpr after gcc 5 arrives.
template<class IntType, class ShiftType>
IntType delta_swap(IntType num, ShiftType delta, IntType mask)
{
    IntType y = (num ^ (num >> delta)) & mask;
    return num ^ y ^ (y << delta);
}

///
///
/// Note:
///
///   * declare as constexpr after gcc 5 arrives.
inline uint64_t bit_zip(uint64_t in)
{
    constexpr uint64_t m [] = {
        0x00000000ffff0000ul,
        0x0000ff000000ff00ul,
        0x00f000f000f000f0ul,
        0x0c0c0c0c0c0c0c0cul,
        0x2222222222222222ul
    };

    for(unsigned int k = 0; k < 5; ++k)
    {
        in = delta_swap(in, 1 << (4-k), m[k]);
    }

    return in;
}

///
///
/// Note:
///
///   * declare as constexpr after gcc 5 arrives.
inline uint64_t bit_uzp(uint64_t in)
{
    constexpr uint64_t m [] = {
        0x00000000ffff0000ul,
        0x0000ff000000ff00ul,
        0x00f000f000f000f0ul,
        0x0c0c0c0c0c0c0c0cul,
        0x2222222222222222ul
    };

    for(unsigned int k = 0; k < 5; ++k)
    {
        in = delta_swap(in, 1 << (k), m[4-k]);
    }

    return in;
}

#endif
