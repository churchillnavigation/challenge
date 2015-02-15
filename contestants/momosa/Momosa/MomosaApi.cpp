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

#include "MomosaApi.hpp"
#include "SearchContext.hpp"


SearchContext* create(Point const* points_begin, Point const* points_end)
{
    SearchContext* context = new SearchContext(points_begin, points_end);
    return context;
}

int32_t search(SearchContext* sc, Rect const rect, int32_t const count, Point* out_points)
{
    return sc->search(rect, count, out_points);
}

SearchContext* destroy(SearchContext* sc)
{
    delete sc;
    return nullptr;
}