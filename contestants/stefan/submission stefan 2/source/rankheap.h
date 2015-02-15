#ifndef RANKHEAP_H
#define RANKHEAP_H

#include "point_search.h"

#include <vector>
#include <algorithm>
#include <stdint.h>

class RankHeap
{
public:

    RankHeap() {}
    RankHeap(size_t capacity)
    {
        m_data.resize(capacity);
        m_begin = std::begin(m_data);
        m_end = m_begin;
    }

    size_t size() const {return std::distance(begin(), end());}

    bool full() const {return m_end == m_data.end();}

    point_index top() const {return *m_begin;}

    void reset(size_t capacity) {
        m_data.resize(capacity);
        m_begin = std::begin(m_data);
        m_end = m_begin;
    }

    void push(point_index index) throw() {
        if (m_end == m_data.end()) {
            if (index < top()) {
                std::pop_heap(m_begin, m_end);
                *(m_end-1) = index;
                std::push_heap(m_begin, m_end);
            }
        } else {
            *m_end++ = index;
            std::push_heap(m_begin, m_end);
        }
    }

    /**
     * Sort the range. This locks all existing items in-place and creates an smaller max-heap
     * with capacity-size() elements.
     */
    void sort() {
        std::sort_heap(m_begin, m_end);
        m_begin = m_end;
    }
    std::vector<point_index>::const_iterator begin() const {return std::begin(m_data);}
    std::vector<point_index>::const_iterator end() const {return m_end;}

    friend std::vector<point_index>::const_iterator begin(const RankHeap &heap) {return heap.begin();}
    friend std::vector<point_index>::const_iterator end(const RankHeap &heap) {return heap.end();}

private:
    std::vector<point_index> m_data;
    std::vector<point_index>::iterator m_begin;
    std::vector<point_index>::iterator m_end;
};

#endif // RANKHEAP_H
