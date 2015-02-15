#include "stdafx.h"
#include "adams.h"
#include <iostream>

using namespace std;

struct SearchContext {
	PointStore store;
};

void print(int cnt, float a, float b, float c, float d){
	float area = abs(a-c) * abs(b-d);
	cout << "#" << cnt << ":\t" << a << "\t" << b << "\t" << c << "\t" << d << "\t" << area << endl;
}

bool cmp(int& a, int& b){
	return a < b;
}

SearchContext* __stdcall  create(const Point* points_begin, const Point* points_end){
	SearchContext* sc = new SearchContext();
	Point* ptr = (Point*) points_begin;

	long cnt = (long)(points_end - points_begin);

	for(int i=0; i < cnt; i++) {
		Point p = ptr[i];
		sc->store.Add(p);
	}

	sc->store.Sort();
	sc->store.FindRealRoot();

	return sc;
}

int32_t __stdcall search(SearchContext* sc, const Rect rect, const int32_t count, Point* out_points){
	CellRect cellRect;
	cellRect.hx = rect.lx;
	cellRect.hy = rect.hy;
	cellRect.lx = rect.hx;
	cellRect.ly = rect.ly;

	int cnt = sc->store.CellsForRect(cellRect, out_points, count);

	return cnt;
}

SearchContext* __stdcall destroy(SearchContext* sc){
	delete sc;
	return nullptr;
}

