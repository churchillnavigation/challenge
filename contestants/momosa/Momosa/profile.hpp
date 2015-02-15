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

#include <chrono>
#include <windows.h>

namespace std { 
namespace chrono {

struct high_res_clock
{
    typedef long long rep;
    typedef std::nano period;
    typedef std::chrono::duration<rep, period> duration;
    typedef std::chrono::time_point<high_res_clock> time_point;
    static const bool is_steady = true;

    static time_point now()
    {
        const long long frequency = []() -> long long 
        {
            LARGE_INTEGER freq;
            QueryPerformanceFrequency(&freq);
            return freq.QuadPart;
        }();

        LARGE_INTEGER count;
        QueryPerformanceCounter(&count);
        return time_point(duration(count.QuadPart * static_cast<rep>(period::den) / frequency));
    }
};

}} // std::chrono