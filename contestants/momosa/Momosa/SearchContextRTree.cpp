/*
 * Copyright (c) 2015 Patrick Moore
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <ppl.h>
#include "SearchContextImpl.hpp"
#include "RTree.hpp"
#include "iterators.hpp"
#include "statistics.hpp"
#include "profile.hpp"

//
//
//
class SearchContextRTree::Impl
{
public:
    Impl(Point const* points_begin, Point const* points_end);
    ~Impl();

    int32_t search_impl(Rect const& rect, int32_t const count, Point* out_points);

private:
    template<class Reporter>
    void search_tree(Rect const& region, Reporter& reporter);

    template<std::size_t I, typename Iter, class Reporter>
    void search_linear(Iter first, Iter last, Rect const& region, Reporter& reporter);

    template<std::size_t I>
    double calculate_contained_percentage(Rect const& region)
    {
        const auto phi_lo = statistics::calculate_contained_percentage(get_dim_coord_lo<I>(region), get_dim_coord<I>(m_mean), get_dim_coord<I>(m_stddev));
        const auto phi_hi = statistics::calculate_contained_percentage(get_dim_coord_hi<I>(region), get_dim_coord<I>(m_mean), get_dim_coord<I>(m_stddev));

        if(get_dim_coord_lo<I>(region) < get_dim_coord<I>(m_mean) && get_dim_coord_hi<I>(region) > get_dim_coord<I>(m_mean))
        {
            return abs(phi_hi + phi_lo);
        }

        return abs(phi_hi - phi_lo);
    }

private:
    typedef RTree<Point, rtree_parameters<64>> rtree_t;

    static const size_t partition_size = 150000;
    static const size_t linear_search_threshold = 800;

    std::vector<rtree_t> m_trees;
    std::vector<Point> m_results;
    std::vector<Point> m_points_sorted[2];
    statistics::Point m_mean;
    statistics::Point m_stddev;
    Rect mbr;
};

SearchContextRTree::Impl::Impl(Point const* points_begin, Point const* points_end)
{
    if(std::distance(points_begin, points_end) <= 0) { return; }

    std::vector<Point> points;

    points.insert(points.begin(), points_begin, points_end);
    points.erase(std::remove_if(points.begin(), points.end(), [](Point const& p){ return (abs(p.x) > 1.0e9 || abs(p.y) > 1.0e9); }  ), points.end());

    concurrency::parallel_sort(points.begin(), points.end());

    m_points_sorted[0] = points;
    concurrency::parallel_sort(m_points_sorted[0].begin(), m_points_sorted[0].end(), [](Point const& p1, Point const& p2) { return p1.x < p2.x; } );

    m_points_sorted[1] = points;
    concurrency::parallel_sort(m_points_sorted[1].begin(), m_points_sorted[1].end(), [](Point const& p1, Point const& p2) { return p1.y < p2.y; } );

    initialize(mbr);

    statistics::calculator stat_calc;
    for(auto& p : points)
    {
        extend_bounds(mbr, p);

        stat_calc.apply(p);
    }

    m_mean = stat_calc.mean;
    m_stddev = stat_calc.calculate_std_dev();

    m_trees.reserve(points.size() / partition_size + 1);

    auto beginIt = points.begin();
    auto endIt = points.end();

    auto startIt = beginIt;
    auto lastIt = startIt + std::min(partition_size, static_cast<size_t>(endIt - startIt));

    while(startIt != endIt)
    {
        m_trees.emplace_back(startIt, lastIt);

        startIt = lastIt != endIt ? lastIt : endIt;
        lastIt = startIt + std::min(partition_size, static_cast<size_t>(endIt - startIt));
    }
}

SearchContextRTree::Impl::~Impl()
{
}

template<class Reporter>
void SearchContextRTree::Impl::search_tree(Rect const& region, Reporter& reporter)
{
    for(auto& tree : m_trees)
    {
        tree.query(region, reporter);
        if(m_results.size() >= m_results.capacity()) { break; }
    }
}

template<std::size_t I, typename Iter, class Reporter>
void SearchContextRTree::Impl::search_linear(Iter first, Iter last, Rect const& region, Reporter& reporter)
{
    auto start = std::lower_bound(first, last, get_dim_coord_lo<I>(region), [](Point const& p, float v) { return get_dim_coord<I>(p) < v; });
    for(; start != last && get_dim_coord<I>(*start) <= get_dim_coord_hi<I>(region); ++start)
    {
        auto& p = *start;
        static const std::size_t K = (I + 1) % 2;
        if(get_dim_coord<K>(p) >= get_dim_coord_lo<K>(region) && get_dim_coord<K>(p) <= get_dim_coord_hi<K>(region))
        {
            *reporter = p;
        }
    }
}

int32_t SearchContextRTree::Impl::search_impl(Rect const& region, int32_t const count, Point* out_points)
{
    if(!intersects(region, mbr)) { return 0; }

    m_results.clear();
    m_results.reserve(count);

    //
    // Attempt to reduce worst case scenarios.
    // 
    // Worst case for the tree search is a search region that hits nearly all node mbr's but does not include many points.  In
    // this case, all trees in the partition are searched with a significant number of nodes in each tree being visited.  Instead,  a 
    // linear search of a list sorted in a dimension is superior, except in the case where many points are in the region.
    // 
    // The goal is to only use the linear search if there are little to no points within the search region, otherwise use the tree search.
    //
    // If statistically a significant low amount of points fall within a dimension of the region, then perform linear search 
    // otherwise perform a tree search.
    //
    auto reporter = min_constrained_inserter(m_results);

    const double phi[2] = { calculate_contained_percentage<0>(region), calculate_contained_percentage<1>(region) };
    const auto dim = phi[0] < phi[1] ? 0 : 1;

    const auto num_points_probability = static_cast<std::size_t>(phi[dim] * m_points_sorted[dim].size());

    if(num_points_probability > linear_search_threshold)
    {
        search_tree(region, reporter);
    }
    else
    {
        auto first = m_points_sorted[dim].begin();
        auto last = m_points_sorted[dim].end();

        if(dim == 0)
        {
            search_linear<0>(first, last, region, reporter);
        }
        else
        {
            search_linear<1>(first, last, region, reporter);
        }
    }

    std::sort(m_results.begin(), m_results.end());
    memcpy(out_points, m_results.data(), sizeof(Point)*m_results.size());

    return static_cast<int32_t>(m_results.size());
}

//
//
//
SearchContextRTree::SearchContextRTree(Point const* points_begin, Point const* points_end) 
    : m_impl(new SearchContextRTree::Impl(points_begin, points_end))
{
}

SearchContextRTree::~SearchContextRTree() 
{
}

int32_t SearchContextRTree::search_impl(Rect const& rect, int32_t const count, Point* out_points)
{
    return m_impl->search_impl(rect, count, out_points);
}
