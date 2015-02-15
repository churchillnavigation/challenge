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

#include <iterator>

// A back inserter iterator that will only insert values to the capacity of container
template<class Container>
class min_constrained_iterator : 
    public std::iterator<std::output_iterator_tag, void, void, void, void>
{
public:
    typedef min_constrained_iterator<Container> min_con_iterator;
    typedef Container container_type;
    typedef typename Container::const_reference const_reference;
    typedef typename Container::value_type value_type;

    explicit min_constrained_iterator(Container& c)
        : container(c)
        , max_rank(std::numeric_limits<int32_t>::max())
    {
    }

    __forceinline min_con_iterator& operator=(value_type const& value)
    {
        insert_impl(value);
        return (*this);
    }

    min_con_iterator& operator*()
    {
        return (*this);
    }

    min_con_iterator& operator++()
    {
        return (*this);
    }

    min_con_iterator operator++(int)
    {
        return (*this);
    }

    template<typename Type>
    bool can_add(Type const& value) const
    {
        if(container._Mylast < container._Myend || value < container.back())
        {
            return true;
        }

        return false;
    }

    int32_t get_max_rank() const
    {
        return max_rank;
    }

    void clear() { container.clear(); }

protected:
    __forceinline void move_max_to_back()
    {
        auto max_val_pos = std::max_element(container.begin(), container.end()); 
        std::iter_swap(max_val_pos, (container.end()-1));
    }

    __forceinline void insert_impl(value_type const& value)
    {
        if(container._Mylast < container._Myend)  // size() < capacity()
        {
            container.push_back(value);
            if(container._Mylast == container._Myend)  // size() < capacity()
            {
                move_max_to_back();
            }
        }
        else if(value < container.back()) 
        {
            container.back() = value;
            move_max_to_back();
            max_rank = container.back().rank;
        }
    }

protected:
    Container &container;
    int32_t max_rank;
};

template<class Container> inline
min_constrained_iterator<Container> min_constrained_inserter(Container& cont)
{
    return (min_constrained_iterator<Container>(cont));
}
