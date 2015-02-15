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

/// Interleave the lower 16 bits of A and B.
///
/// Modified from the Stanford library of bit-twiddling hacks [1].
///
/// Return value:
///
///   A 32-bit unsigned integer with the 16 lowest bits of A in the odd
///   positions, and the 16 lowest bits of B in the even positions.
///
/// References:
///
///   [1] https://graphics.stanford.edu/~seander/bithacks.html#InterleaveBMN
inline uint32_t bit_ilv_lower(uint32_t even, uint32_t odd)
{
    static constexpr uint32_t m[] = {
        0x55555555, 0x33333333, 0x0F0F0F0F, 0x00FF00FF
    };
    static constexpr uint32_t s[] = {1, 2, 4, 8};

    even &= 0x0000ffff;
    odd &= 0x0000ffff;

    for(int i = 3; i >= 0; --i)
    {
        even = (even | (even << s[i])) & m[i];
        odd = (odd | (odd << s[1])) & m[1];
    }

    return (odd << 1) | even;
}

inline uint32_t bit_ilv_upper(uint32_t even, uint32_t odd)
{
    static constexpr uint32_t m[] = {
        0xaaaaaaaa, 0xcccccccc, 0xf0f0f0f0, 0xff00ff00
    };

    static constexpr uint32_t s[] = {
        1, 2, 4, 8
    };

    // Mask to the upper 16 bits.
    even &= 0xffff0000;
    odd &= 0xffff0000;

    for(int i = 3; i>=0; --i)
    {
        even = (even | (even >> s[i])) & m[i];
        odd = (odd | (odd >> s[i])) & m[i];
    }

    return odd | (even >> 1);
}

inline std::array<uint32_t, 2> bit_dlv_lower(uint32_t in)
{
    static constexpr uint32_t m[] = {
        0x33333333, 0x0f0f0f0f, 0x00ff00ff, 0x0000ffff
    };
    static constexpr uint32_t s[] = {1, 2, 4, 8};

    std::array<uint32_t, 2> r = {
        in        & 0x55555555,
        (in >> 1) & 0x55555555
    };

    for(int i = 0; i < 4; ++i)
    {
        r[0] = (r[0] | (r[0] >> s[i])) & m[i];
        r[1] = (r[1] | (r[1] >> s[i])) & m[i];
    }

    return r;
}

/// Deinterleave IN into 16 odd bits and 16 even bits.
///
/// Return value:
///
///   Array containing the 16 even bits of IN in the 16 upper bits of index 0,
///   and the 16 odd bits of IN in the upper 16 bits of index 1.
inline std::array<uint32_t, 2> bit_dlv_upper(uint32_t in)
{
    static constexpr uint32_t m[] = {
        0xcccccccc, 0xf0f0f0f0, 0xff00ff00, 0xffff0000
    };
    static constexpr uint32_t s[] = {
        1, 2, 4, 8
    };

    std::array<uint32_t, 2> r = {
        (in << 1) & 0xaaaaaaaa,
        in        & 0xaaaaaaaa
    };

    for(int i = 0; i < 4; ++i)
    {
        r[0] = (r[0] | (r[0] << s[i])) & m[i];
        r[1] = (r[1] | (r[1] << s[i])) & m[i];
    }

    return r;
}

#endif
