#include <ftl_search/slice.h>

slice_t::slice_t()
{
    m_start = NULL;
    m_length = 0;
}

slice_t::slice_t(const slice_t &other)
{
    m_start = other.m_start;
    m_length = other.m_length;
}

slice_t &
slice_t::operator=(const slice_t &other)
{
    m_start = other.m_start;
    m_length = other.m_length;

    return *this;
}

slice_t::slice_t(Point *start, size_t length)
{
    m_start = start;
    m_length = length;
}
