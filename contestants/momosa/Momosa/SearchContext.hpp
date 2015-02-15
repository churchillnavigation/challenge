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

#include "SearchContextImpl.hpp"

class SearchContext
{
public:
    SearchContext(Point const* points_begin, Point const* points_end) 
        : m_impl(points_begin, points_end)
    {
    }

    ~SearchContext() {}

    int32_t search(Rect const& rect, int32_t const count, Point* out_points)
    {
        auto results = m_impl.search(rect, count, out_points);
        return results;
    }

private:
    SearchContextRTree m_impl;
};