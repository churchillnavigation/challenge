#ifndef IT_RANGE_HH
#define IT_RANGE_HH

#include <iterator>

template<class It>
struct it_range
{
    It src;
    It lim;

    it_range() = default;

    it_range(It src_in, It lim_in)
        : src(src_in),
          lim(lim_in)
    {
    }

    it_range(const it_range& other) = default;

    It begin() const
    {
        return src;
    }

    It end() const
    {
        return lim;
    }

    std::size_t size() const
    {
        return std::distance(src, lim);
    }
};

template<class It>
std::size_t size(const it_range<It> &r)
{
    return r.size();
}

#endif
