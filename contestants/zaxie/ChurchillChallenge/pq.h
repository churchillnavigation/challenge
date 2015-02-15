#ifndef PQ_H
#define PQ_H

#include <vector>
#include <algorithm>
#include <crtdbg.h>
#include <immintrin.h>

struct PqItem
{
    unsigned rank;
    unsigned value;

    bool operator<(const PqItem &other) const
    {
        return rank < other.rank;
    }
};

class PriorityQueue_STL
{
public:
    enum { N = 20 };

private:
    std::vector<PqItem> _data;
    unsigned _current;

public:
    PriorityQueue_STL()
    {
        clear();
    }
    void clear()
    {
        _data.clear();
        _data.reserve(N + 4);
        _current = 0;
    }
    unsigned size() const
    {
        return (unsigned)_data.size();
    }
    bool hasPlace(unsigned rank)
    {
        return size() < N || rank < _data.back().rank;
    }
    void push(PqItem item)
    {
        auto p = std::lower_bound(_data.begin(), _data.end(), item);
        _ASSERTE(p == _data.end() || p->rank != item.rank);
        if (p != _data.end() && p->rank == item.rank)
            abort();
        _data.insert(p, item);
        if (size() > N)
            _data.pop_back();
        _ASSERTE(size() <= N);
    }
    PqItem pop()
    {
        if (_current >= size())
            abort();
        return _data[_current++];
    }
};

class PriorityQueue_AVX2
{
public:
	enum { N = 20 };

private:
	// High 128-bit lane contains values corresponding to keys in the low lane.
	// This is because 256-instructions operate over individual 128-bit lanes.
	// Since we have only RIGHT double-width shift (VPALIGNR), the maximum value
	// is the RIGHTMOST within each lane. The keys & values are thus stored like
	// this (numbers within registers are sorting order)
	//
	//                 VAL       KEY
	// Offset Field  7 --- 4 - 3 --- 0
	//   [0]       [ 0 ..  3 | 0 ..  3]
	//   [1]       [ 4 ..  7 | 4 ..  7]
	//   [2]       [ 8 .. 11 | 8 .. 11]
	//   [3]       [12 .. 15 |12 .. 15]
	//   [4]       [16 .. 19 |16 .. 19]
	//
	// Push and pop cannot be mixed without clear() inbetween, so we rearrange to linear
	// order only on the first pop().

	__m256i _rv[5];
    __declspec(align(32)) unsigned _lranks[20], _lvalues[20];
	unsigned _size, _current, _maxrank;
	bool _isPopping;

	int compare(__m256i mrank, int &field, __m256i &gtmask);
    void shift(const int reg);
    void insert(const int reg, const int field, __m256i gtmask, PqItem item);
    void linearize();

public:
	PriorityQueue_AVX2()
	{
		clear();
	}
	unsigned size() const
	{
		static_assert(N <= 20, "hard-coded to max 20 elements");
		return _size < N ? _size : N;
	}
	bool hasPlace(unsigned rank)
	{
		return size() < N || rank < _maxrank;
	}

	void clear();
	bool push(PqItem item);
	PqItem pop();
};

#endif
