#pragma once
#include "geom.h"
#include <functional>
namespace search2d
{
typedef std::function< bool (size_t) > searchOutput; //returns true to continue search
void linear_search8(const geom2d::Point * __restrict points, size_t len, const geom2d::Rectangle&r, searchOutput output);
}