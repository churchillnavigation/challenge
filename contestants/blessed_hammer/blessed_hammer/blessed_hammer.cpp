// blessed_hammer.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include <iterator>
#include <vector>
#include <algorithm>
#include <thread>



bool SortByRankPointsPredicate(const Point &a, const Point &b)
{
	return a.rank < b.rank;
}


float ElementXPredicate(const Point &a, const Point &b)
{
	return a.x < b.x;
}



float ElementYPredicate(const Point &a, const Point &b)
{
	return a.y < b.y;
}



extern "C" __declspec(dllexport) SearchContext* __stdcall create(const Point* points_begin, const Point* points_end)
{
	if (points_begin != nullptr)
	{
		SearchContext* sc = new SearchContext();
		sc->orderedPoints.assign(points_begin, points_end);

		std::sort(sc->orderedPoints.begin(), sc->orderedPoints.end(), SortByRankPointsPredicate);
		
		sc->pointArea.lx = std::min_element(sc->orderedPoints.begin(), sc->orderedPoints.end(), ElementXPredicate)->x;
		sc->pointArea.hx = std::max_element(sc->orderedPoints.begin(), sc->orderedPoints.end(), ElementXPredicate)->x;
		sc->pointArea.ly = std::min_element(sc->orderedPoints.begin(), sc->orderedPoints.end(), ElementYPredicate)->y;
		sc->pointArea.hy = std::max_element(sc->orderedPoints.begin(), sc->orderedPoints.end(), ElementYPredicate)->y;
		
		sc->tree = BuildTree(sc->orderedPoints, sc->pointArea);


		return sc;
	}
	return nullptr;
}



extern "C" __declspec(dllexport) int32_t __stdcall search(SearchContext* sc, const Rect rect, const int32_t count, Point* out_points)
{
	return SearchTree(sc, rect, count, out_points);
}


extern "C" __declspec(dllexport) SearchContext* __stdcall destroy(SearchContext* sc)
{
	if (sc != nullptr)
	{
		sc->orderedPoints.clear();
		delete sc;
	}


	return nullptr;
}