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

#if defined(MOMOSA_DLL_EXPORTS)
#   define MOMOSA_DLL_API __declspec(dllexport)
#else
#   define MOMOSA_DLL_API __declspec(dllimport)
#endif

#include "point_search.h"

extern "C" 
{
    MOMOSA_DLL_API SearchContext* create(const Point* points_begin, const Point* points_end);
    MOMOSA_DLL_API int32_t search(SearchContext* sc, const Rect rect, const int32_t count, Point* out_points);
    MOMOSA_DLL_API SearchContext* destroy(SearchContext* sc);
}
