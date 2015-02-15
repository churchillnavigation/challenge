#include <limits.h>
#include "pq.h"

void PriorityQueue_AVX2::clear()
{
    _size = _current = 0;
    _maxrank = INT_MAX;
    _isPopping = false;

    __m256i max = _mm256_set1_epi32(_maxrank);
    for (int i = 0; i < 5; ++i)
        _mm256_store_si256(_rv + i, max);
}

// Returns true if the item is inserted, false if it's duplicate.
bool PriorityQueue_AVX2::push(PqItem item)
{
    _ASSERTE(!_isPopping);

    const __m256i mrank = _mm256_set1_epi32(item.rank);
    int field;
    __m256i gtmask;
    int reg = compare(mrank, field, gtmask);

    if (reg < 0)
        return false;

    shift(reg);
    insert(reg, field, gtmask, item);
    ++_size;
    return true;
}

// Compare rank with all values currently in the queue.  Returns -1 if the value already exists
// or is larger than all values.
// Otherwise, returns the index of the register in which the value should be inserted.
// Mask is replicated to both lanes, so it can be used for both value and rank lane.
int PriorityQueue_AVX2::compare(__m256i mrank, int &field, __m256i &gtmask)
{
    static const __m256i eq4mask = _mm256_set_epi32(0, 0, 0, 0, -1, -1, -1, -1);
    __m256i eq, eq4;
    int reg, mask;

    // Because items are sorted in ascending order within each (double) register, the mask after GT
    // comparison must be of the form 000...1111, which is one less than a power of two.
    {
        __m256i r0_7 = _mm256_permute2x128_si256(_rv[1], _rv[0], 0x20);		// [0 .. 7]
        gtmask = _mm256_cmpgt_epi32(r0_7, mrank);
        mask = _mm256_movemask_ps(_mm256_castsi256_ps(gtmask));
        eq = _mm256_cmpeq_epi32(r0_7, mrank);
        _ASSERTE(((mask + 1) & mask) == 0);
        reg = 1;
    }

    if (!mask) {
        __m256i r8_15 = _mm256_permute2x128_si256(_rv[3], _rv[2], 0x20);	// [8 .. 15]
        gtmask = _mm256_cmpgt_epi32(r8_15, mrank);
        mask = _mm256_movemask_ps(_mm256_castsi256_ps(gtmask));
        eq = _mm256_or_si256(eq, _mm256_cmpeq_epi32(r8_15, mrank));
        _ASSERTE(((mask + 1) & mask) == 0);
        reg = 3;
    }

    if (!mask) {
        gtmask = _mm256_cmpgt_epi32(_rv[4], mrank);							// [16 .. 19]; don't care about value
        eq4 = _mm256_and_si256(eq4mask, _mm256_cmpeq_epi32(mrank, _rv[4])); // .. ditto
        mask = _mm256_movemask_ps(_mm256_castsi256_ps(gtmask)) & 0xF;       // ignore comparison with values
        eq = _mm256_or_si256(eq, eq4);
        _ASSERTE(((mask + 1) & mask) == 0);
        reg = 4;
    }

    if (_mm256_movemask_ps(_mm256_castsi256_ps(eq)) != 0)
        mask = 0;
    if (!mask)
        return -1;

    // Adjust register according to mask (higher 128-bits i double register: one register lower)
    // There is no "previous" register to test against for equality if we need to insert in the
    // very first register.  Also duplicate the same mask to both lanes.

    if (mask > 0xF) {
        mask >>= 4;
        --reg;
        gtmask = _mm256_permute2x128_si256(gtmask, gtmask, 0x11);           // replicate high lane to both
    }
    else {
        gtmask = _mm256_permute2x128_si256(gtmask, gtmask, 0x00);           // replicate low lane to both
    }

    switch (mask) {
    case 15: field = 3; break;
    case 7: field = 2; break;
    case 3: field = 1; break;
    case 1: field = 0; break;
    }

    return reg;
}

// Shift all registers AFTER reg, but leave reg unchanged (it has to be blended afterwards)
void PriorityQueue_AVX2::shift(const int reg)
{
    if (reg == 4) return;

    _rv[4] = _mm256_alignr_epi8(_rv[3], _rv[4], 4);
    if (reg == 3) return;

    _rv[3] = _mm256_alignr_epi8(_rv[2], _rv[3], 4);
    if (reg == 2) return;

    _rv[2] = _mm256_alignr_epi8(_rv[1], _rv[2], 4);
    if (reg == 1) return;

    _rv[1] = _mm256_alignr_epi8(_rv[0], _rv[1], 4);
    _ASSERTE(reg == 0);
}

void PriorityQueue_AVX2::insert(const int reg, const int field, __m256i gtmask, PqItem item)
{
    // Make place at the given field in the register.
    _rv[reg] = _mm256_blendv_epi8(
        _rv[reg],
        _mm256_srli_si256(_rv[reg], 4),
        gtmask);

    // Broadcast rank and value to their lanes. TODO! USE BROADCAST FROM MEMORY!
    __m128i mzero = _mm_setzero_si128();
    __m128i mrank = _mm_insert_epi32(mzero, item.rank, 0);
    __m128i mvalue = _mm_insert_epi32(mzero, item.value, 0);

    switch (field) {
    case 1:
        mrank = _mm_slli_si128(mrank, 4 * 1);
        mvalue = _mm_slli_si128(mvalue, 4 * 1);
        break;
    case 2:
        mrank = _mm_slli_si128(mrank, 4 * 2);
        mvalue = _mm_slli_si128(mvalue, 4 * 2);
        break;
    case 3:
        mrank = _mm_slli_si128(mrank, 4 * 3);
        mvalue = _mm_slli_si128(mvalue, 4 * 3);
        break;
    }

    __m256i mnew = _mm256_inserti128_si256(_mm256_setzero_si256(), mrank, 0);
    mnew = _mm256_inserti128_si256(mnew, mvalue, 1);

    // Insertion mask.
    gtmask = _mm256_xor_si256(gtmask, _mm256_srli_si256(gtmask, 4));
    _rv[reg] = _mm256_blendv_epi8(_rv[reg], mnew, gtmask);
}

PqItem PriorityQueue_AVX2::pop()
{
    if (!_isPopping++)
        linearize();

    _ASSERTE(_current < size());
    PqItem ret{ _lranks[_current], _lvalues[_current] };
    ++_current;
    return ret;
}

void PriorityQueue_AVX2::linearize()
{
    for (int i = 0; i < 5; ++i) {
        __m128i ranks = _mm256_extracti128_si256(_rv[i], 0);
        __m128i values = _mm256_extracti128_si256(_rv[i], 1);

        // Reverse order within each register before storing.
        _mm_store_si128((__m128i*)_lranks + i, _mm_shuffle_epi32(ranks, 0x1B));
        _mm_store_si128((__m128i*)_lvalues + i, _mm_shuffle_epi32(values, 0x1B));
    }
}
