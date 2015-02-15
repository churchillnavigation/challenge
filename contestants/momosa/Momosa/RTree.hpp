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

#pragma once

#include <algorithm>
#include <vector>
#include <assert.h>

#include "TaskStack.hpp"
#include "point_search.h"

template <std::size_t MaxElements>
struct default_min_elements
{
    static const std::size_t raw_value = (MaxElements * 4) / 10;
    static const std::size_t value = 1 <= raw_value ? raw_value : 1;
};

template <std::size_t MaxElements, std::size_t MinElements = default_min_elements<MaxElements>::value>
struct rtree_parameters
{
    static const std::size_t max_elements = MaxElements;
    static const std::size_t min_elements = MinElements;

    static std::size_t get_max_elements() { return MaxElements; }
    static std::size_t get_min_elements() { return MinElements; }
};

template <typename Value, typename Parameters>
class RTree
{
public:
    template <typename Iterator>
    explicit RTree(Iterator points_begin, Iterator points_end, Parameters const& parameters = Parameters()) 
        : m_parameters(parameters), m_values_count(0), m_height(0)
    { 
        build(points_begin, points_end); 
    }

    template<typename OutIter>
    void query(Rect const& region, OutIter& out_it)
    {
        if(m_values_count == 0) { return; }

        if(!intersects(region, m_root.mbr)) { return; }

        // note: profiling shows iterative is ~.1.5-2.5 microsecs faster per query
        query_iterative(region, out_it);
    }

private:
    struct Node
    {
        Node() 
            : rank(std::numeric_limits<int32_t>::max())
        {
            initialize(mbr);
        }

        bool is_leaf() const { return !leaf.empty(); }

        int32_t rank;
        Rect mbr;

        std::vector<Node> nodes;
        std::vector<Value> leaf;
    };

    struct subtree_elements_counts
    {
        subtree_elements_counts(std::size_t max_count_, std::size_t min_count_) : max_count(max_count_), min_count(min_count_) {}
        std::size_t max_count;
        std::size_t min_count;
    };

    template <typename Iterator>
    void build(Iterator points_begin, Iterator points_end)
    {
        m_values_count = std::distance(points_begin, points_end);
        if(m_values_count == 0) { return; }

        for(auto it = points_begin; it != points_end; ++it)
        {
            extend_bounds(m_root.mbr, *it);
        }

        const auto elements_count = calculate_subtree_elements_counts(m_values_count, m_parameters, m_height);
        nodesToSearch.reserve(elements_count.max_count);

        auto dim = get_longest_edge(m_root.mbr);
        generate_subtree(points_begin, points_end, m_root.mbr, m_values_count, elements_count, m_root, dim, m_parameters);

        sort_subtree(m_root, [](Node const& n1, Node const& n2) { return n1.rank < n2.rank; } );
    }

    template <typename EIt> inline static
    void generate_subtree(EIt first, EIt last, Rect const& super_mbr, std::size_t values_count, 
                            subtree_elements_counts const& subtree_counts, Node& subtree, std::size_t dim, Parameters const& parameters)
    {
        assert(static_cast<std::size_t>(std::distance(first, last)) == values_count);

        if (subtree_counts.max_count <= 1)
        {
            assert(values_count <= parameters.get_max_elements());

            subtree.leaf.reserve(values_count);
            for(;first != last; ++first)
            {
                auto& e = *first;
                subtree.leaf.push_back(e);
                extend_bounds(subtree.mbr, e);
                if(subtree.rank > e.rank) { subtree.rank = e.rank; }
            }

            return;
        }

        auto next_subtree_counts = subtree_counts;
        next_subtree_counts.max_count /= parameters.get_max_elements();
        next_subtree_counts.min_count /= parameters.get_max_elements();

        auto nodes_count = calculate_nodes_count(values_count, subtree_counts);
        subtree.nodes.reserve(nodes_count);

        partition_subtree(first, last, super_mbr, values_count, subtree_counts, next_subtree_counts,
            subtree, dim, parameters);
    }

    template <typename EIt> inline static
    void partition_subtree(EIt first, EIt last, Rect const& super_mbr, std::size_t values_count,
                           subtree_elements_counts const& subtree_counts,
                           subtree_elements_counts const& next_subtree_counts,
                           Node & elements, std::size_t dim, Parameters const& parameters)
    {
        assert(std::distance(first, last) > 0 && static_cast<std::size_t>(std::distance(first, last)) == values_count);

        assert(subtree_counts.min_count <= values_count);

        if (values_count <= subtree_counts.max_count)
        {
            elements.nodes.emplace_back();
            auto& n = elements.nodes.back();

            dim = (dim + 1) % 2;

            generate_subtree(first, last, super_mbr, values_count, next_subtree_counts, n, dim, parameters);

            extend_bounds(elements.mbr, n.mbr);
            if(elements.rank > n.rank) { elements.rank = n.rank; }

            return;
        }
        
        const auto median_count = calculate_median(values_count, subtree_counts);
        auto median = first + median_count;

        auto first_med_mbr = super_mbr; 
        auto med_last_mbr = super_mbr;

        if(dim == 0)
        {
            nth_element_dimension<0>(first, median, last);
            split_mbr<0>(first, median, first_med_mbr, med_last_mbr);
        }
        else
        {
            nth_element_dimension<1>(first, median, last);
            split_mbr<1>(first, median, first_med_mbr, med_last_mbr);
        }
        
        partition_subtree(first, median, first_med_mbr, median_count, subtree_counts, next_subtree_counts,
                          elements, dim, parameters);
        partition_subtree(median, last, med_last_mbr, values_count - median_count, subtree_counts, next_subtree_counts,
                          elements, dim, parameters);
    }

   
    template<std::size_t I, typename EIt> inline static
    void nth_element_dimension(EIt first, EIt n, EIt last)
    {
        typedef std::iterator_traits<EIt>::value_type it_value_type; 
        std::nth_element(first, n, last, [](it_value_type const& lhs, it_value_type const& rhs) { return get_dim_coord<I>(lhs) < get_dim_coord<I>(rhs); });
    }

    template<std::size_t I, typename EIt> inline static
    void split_mbr(EIt first, EIt median, Rect& first_med_mbr, Rect& med_last_mbr)
    {
        get_dim_coord_hi<I>(first_med_mbr) = median != first ? get_dim_coord<I>(*median) : get_dim_coord<I>(*first);
        get_dim_coord_lo<I>(med_last_mbr) = get_dim_coord<I>(*median);
    }

    inline static
    subtree_elements_counts calculate_subtree_elements_counts(std::size_t elements_count, Parameters const& parameters, std::size_t& height)
    {
        subtree_elements_counts res(1, 1);
        height = 0;

        auto smax = parameters.get_max_elements();
        for ( ; smax < elements_count ; smax *= parameters.get_max_elements(), ++height )
            res.max_count = smax;

        res.min_count = parameters.get_min_elements() * (res.max_count / parameters.get_max_elements());

        return res;
    }

    inline static
    std::size_t calculate_nodes_count(std::size_t count, subtree_elements_counts const& subtree_counts)
    {
        auto n = count / subtree_counts.max_count;
        auto r = count % subtree_counts.max_count;

        if ( 0 < r && r < subtree_counts.min_count )
        {
            auto count_minus_min = count - subtree_counts.min_count;
            n = count_minus_min / subtree_counts.max_count;
            r = count_minus_min % subtree_counts.max_count;
            ++n;
        }

        if ( 0 < r )
            ++n;

        return n;
    }

    inline static
    std::size_t calculate_median(std::size_t count, subtree_elements_counts const& subtree_counts)
    {
        auto n = count / subtree_counts.max_count;
        auto r = count % subtree_counts.max_count;
        auto median_count = (n / 2) * subtree_counts.max_count;

        if ( 0 != r )
        {
            if ( subtree_counts.min_count <= r )
            {
                median_count = ((n+1)/2) * subtree_counts.max_count;
            }
            else 
            {
                auto count_minus_min = count - subtree_counts.min_count;
                n = count_minus_min / subtree_counts.max_count;
                r = count_minus_min % subtree_counts.max_count;
                if ( r == 0 )                               
                {
                    
                    median_count = ((n+1)/2) * subtree_counts.max_count;     
                }
                else
                {
                    if ( n == 0 )                                       
                    {
                        median_count = r;                              
                    }
                    else
                    {
                        median_count = ((n+2)/2) * subtree_counts.max_count;
                    }
                }
            }
        }

        return median_count;
    }

    template <typename Pred>
    void sort_subtree(Node& subtree, Pred& pred)
    {
        if(!subtree.is_leaf())
        {
            std::sort(subtree.nodes.begin(), subtree.nodes.end(), pred);
            for(auto& n : subtree.nodes)
            {
                sort_subtree(n, pred);
            }
        }
        else
        {
            std::sort(subtree.leaf.begin(), subtree.leaf.end());
        }
    }

    template<typename OutIter>
    void query_recursive(Rect const& region, OutIter& out_it)
    {
        recursive_search(m_root, region, out_it);
    }

    template<typename OutIter>
    void recursive_search(Node const& subtree_node, Rect const& region, OutIter& out_it)
    {
        for(const auto& node : subtree_node.nodes)
        {
            if(node.rank < out_it.get_max_rank())
            {
                if(intersects(region, node.mbr))
                {
                    if(contains(region, node.mbr))
                    {
                        add_leafs(node, region, out_it);
                    }
                    else if(node.is_leaf())
                    {
                        for(const auto& p : node.leaf)
                        {
                            if(within(region, p))
                            {
                                if(!out_it.can_add(p)) { break; }
                                *out_it = p;
                            }
                        }
                    }
                    else
                    {
                        recursive_search(node, region, out_it);
                    }
                }
            }
        }
    }

    template<typename OutIter>
    void add_leafs(Node const& subtree_node, Rect const& region, OutIter& out_it)
    {
        if(subtree_node.is_leaf())
        {
            auto& leaf = subtree_node.leaf;
            for(const auto& p : leaf)
            {
                if(!out_it.can_add(p)) { return; }
                *out_it = p;
            }

            return;
        }

        for(const auto& node : subtree_node.nodes)
        {
            if(node.rank < out_it.get_max_rank())
            {
                add_leafs(node, region, out_it);
            }
        }
    }

    template<typename OutIter>
    void query_iterative(Rect const& region, OutIter& out_it)
    {
        nodesToSearch.push_back(&m_root);

        while(!nodesToSearch.empty())
        {
            auto& subtree = *nodesToSearch.back();
            nodesToSearch.pop_back();

            auto& nodes = subtree.nodes;
            for(auto& node : nodes)
            {
                if(node.rank > out_it.get_max_rank()) { break; }

                const auto& mbr = node.mbr;
                if(intersects(region, mbr))
                {
                    if(contains(region, mbr))
                    {
                        const auto stackPos = nodesToSearch.size();

                        nodesToSearch.push_back(&node);

                        while(stackPos < nodesToSearch.size())
                        {
                            auto& contained_node = *nodesToSearch.back();
                            nodesToSearch.pop_back();

                            if(contained_node.is_leaf())
                            {
                                for(auto& n : contained_node.leaf)
                                {
                                    if(n.rank > out_it.get_max_rank()) { break; }
                                    *out_it = n;
                                }
                            }
                            else
                            {
                                for(auto& n : contained_node.nodes)
                                {
                                    if(node.rank > out_it.get_max_rank()) { break; }
                                    nodesToSearch.push_back(&n);
                                }
                            }
                        }
                    }
                    else if(node.is_leaf())
                    {
                        for(const auto& p : node.leaf)
                        {
                            if(p.rank > out_it.get_max_rank()) { break; }
                            if(within(region, p))
                            {
                                *out_it = p;
                            }
                        }
                    }
                    else
                    {
                        nodesToSearch.push_back(&node);
                    }
                }
            }
        }
    }

private:
    Node m_root;
    Parameters m_parameters;
    TaskStack<Node*> nodesToSearch; 
    size_t m_values_count;
    size_t m_height;
};
