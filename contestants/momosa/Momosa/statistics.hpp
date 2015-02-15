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

#include <math.h>
#include <limits>

namespace statistics {

namespace cnd
{

inline static 
double phi(double d)
{
    const double A1 = 0.31938153;
    const double A2 = -0.356563782;
    const double A3 = 1.781477937;
    const double A4 = -1.821255978;
    const double A5 = 1.330274429;
    const double RSQRT2PI = 0.39894228040143267793994605993438;

    const double K = 1.0 / (1.0 + 0.2316419 * fabs(d));

    double cnd = RSQRT2PI * exp(- 0.5 * d * d) *
          (K * (A1 + K * (A2 + K * (A3 + K * (A4 + K * A5)))));

    if (d > 0) cnd = 1.0 - cnd;

    return cnd;
}
} // cnd

inline static
 double calculate_contained_percentage(double value, double mean, double std_dev)
{
    auto sd_value = abs(value - mean) / std_dev;
    auto phi = cnd::phi(sd_value);

    return abs(phi);
}


struct Point
{
    Point() : x(0.0), y(0.0) {}
    Point(::Point const& p) : x(p.x), y(p.y) {}
    Point(double x_, double y_) : x(x_), y(y_) {}

    Point& operator+=(Point const& rhs) 
    { 
        x += rhs.x; 
        y += rhs.y;
        return *this;
    }

    double x;
    double y;
};

inline Point operator+(Point const& lhs, Point const& rhs) { return Point(lhs.x + rhs.x, lhs.y + rhs.y); }
inline Point operator-(Point const& lhs, Point const& rhs) { return Point(lhs.x - rhs.x, lhs.y - rhs.y); }
inline Point operator*(Point const& lhs, Point const& rhs) { return Point(lhs.x * rhs.x, lhs.y * rhs.y); }
inline Point operator/(Point const& lhs, Point const& rhs) { return Point(lhs.x / rhs.x, lhs.y / rhs.y); }

inline Point operator*(Point const& lhs, double rhs) { return Point(lhs.x * rhs, lhs.y * rhs); }
inline Point operator/(Point const& lhs, double rhs) { return Point(lhs.x / rhs, lhs.y / rhs); }


class calculator
{
public:
     calculator() : count(0) {}

    // Calculate std deviation in one pass.  http://www.cs.berkeley.edu/~mhoemmen/cs194/Tutorials/variance.pdf
     void apply(::Point const& point)
     {
        count++;

        Point p(point);
        auto delta = p - mean;
        mean += delta / count;
        sq_sum += delta * delta * (count-1) / count;
     }

     Point calculate_std_dev()
     {
         if(count == 0) { return Point(); }

         Point variance(sq_sum / count);

         Point std_dev(sqrt(variance.x), sqrt(variance.y));

         return std_dev;
     }

     Point mean;
     Point sq_sum;
     double count;
};

} // statistics