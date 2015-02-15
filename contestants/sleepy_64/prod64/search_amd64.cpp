#include "search_amd64.h"
#include <intrin.h>
#include <immintrin.h>
namespace search2d
{
void linear_search8(const geom2d::Point * __restrict points, size_t len, const geom2d::Rectangle&r, searchOutput output)
{
    size_t n8 = len / 8;
    size_t rest = len & 7;

    auto rectMin = _mm256_set_ps(r.a0.y, r.a0.x, r.a0.y, r.a0.x, r.a0.y, r.a0.x, r.a0.y, r.a0.x);
    auto rectMax = _mm256_set_ps(r.a1.y, r.a1.x, r.a1.y, r.a1.x, r.a1.y, r.a1.x, r.a1.y, r.a1.x);
    auto maskLowerPairs = _mm256_castsi256_ps(_mm256_set_epi32(0, 0xffffffff, 0, 0xffffffff, 0, 0xffffffff, 0, 0xffffffff));
    auto maskUpperPairs = _mm256_castsi256_ps(_mm256_set_epi32(0xffffffff, 0, 0xffffffff, 0, 0xffffffff, 0, 0xffffffff, 0));

    const float* p = &points[0].x;

    for (size_t i = 0; i < n8; ++i, p += 8 * 2)
    {
        auto fourPoints = _mm256_load_ps(p);
        auto maskBelow = _mm256_cmp_ps(rectMin, fourPoints, _CMP_LE_OQ);
        auto maskUpper = _mm256_cmp_ps(fourPoints, rectMax, _CMP_LE_OQ);
        auto maskPairs = _mm256_and_ps(maskBelow, maskUpper);
        auto maskPairsShuffled = _mm256_castsi256_ps(_mm256_shuffle_epi32(_mm256_castps_si256(maskPairs), _MM_SHUFFLE(2, 3, 0, 1)));

        auto maskPairsAnded1 = _mm256_and_ps(maskPairs, maskPairsShuffled);

        fourPoints = _mm256_load_ps(p + 8);
        maskBelow = _mm256_cmp_ps(rectMin, fourPoints, _CMP_LE_OQ);
        maskUpper = _mm256_cmp_ps(fourPoints, rectMax, _CMP_LE_OQ);
        maskPairs = _mm256_and_ps(maskBelow, maskUpper);
        maskPairsShuffled = _mm256_castsi256_ps(_mm256_shuffle_epi32(_mm256_castps_si256(maskPairs), _MM_SHUFFLE(2, 3, 0, 1)));

        auto maskPairsAnded2 = _mm256_and_ps(maskPairs, maskPairsShuffled);

        auto lower = _mm256_and_ps(maskPairsAnded1, maskLowerPairs);
        auto upper = _mm256_and_ps(maskPairsAnded2, maskUpperPairs);
        auto result8 = _mm256_or_ps(lower, upper);
        auto test = _mm256_testz_ps(result8, result8);
        if (test == 0)
        {
            uint_fast32_t all = _mm256_movemask_ps(result8);
            static const uint_fast32_t masks[8] = {0x1, 0x4, 0x10, 0x40, 0x2, 0x8, 0x20, 0x80};
            size_t baseIndex = i * 8 ;
            for (size_t pos = 0; pos < 8; ++pos)
            {
                if (all & masks[pos])
                {
                    auto doContinue = output(baseIndex + pos);
                    if (!doContinue)
                    {
                        return;
                    }
                }
            }
        }
    }
    for (size_t i = n8 * 8; i < len; ++i)
    {
        if (point_in_rectangle(points[i], r))
        {
            auto doContinue = output(i);
            if (!doContinue)
            {
                return;
            }
        }
    }
}

}