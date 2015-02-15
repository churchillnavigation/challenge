#include "stdafx.h"
#include <algorithm>
#include <iostream>
#include "cell.h"

using namespace std;

bool SortRank(Point& a, Point& b) {
	return a.rank < b.rank;
}

bool SortX(Point& a, Point& b) {
	if( a.x == b.x ){ 
		if( a.y == b.y ) {
			return a.rank < b.rank;
		}

		return a.y > b.y;
	}

	return a.x > b.x;
}

int Cell::CellCount = 0;
int Cell::MaxLevel = 0;

Cell::Cell() {
	nw = ne = se = sw = parent = nullptr;
	CellCount++;
	level = -1;
	isParent = false;
	isEmpty = true;
}

Cell::Cell(Cell* parent_, CellRect rect_) : parent(parent_), rect(rect_) {
	nw = ne = se = sw = nullptr;
	CellCount++;
	isParent = false;
	isEmpty = true;
	points_all.reserve(MAX_SIZE_LEAF);

	if(parent) {
		level = parent->level + 1;
		if(level > MaxLevel) {
			MaxLevel = level;
		}
	} else {
		level = 0;
	}
}

int total = 0;
Cell::~Cell() {
	delete nw;
	delete ne;
	delete sw;
	delete se;
}

void Cell::Add(Point& p){
	points_all.push_back(p);
	isEmpty = false;
}

CellMatch Cell::Match(CellRect target) {
	if(isEmpty) {
		return NONE;
	}

	if(target.hx > rect.lx) {
		return NONE;
	}

	if(target.lx < rect.hx) {
		return NONE;
	}

	if(target.ly > rect.hy) {
		return NONE;
	}

	if(target.hy < rect.ly) {
		return NONE;
	}

	if(
		rect.hx >= target.hx &&
		rect.lx <= target.lx &&
		rect.hy <= target.hy &&
		rect.ly >= target.ly
		){
			return FULL;
	}

	return PARTIAL;
}

Cell* Cell::FindCell(Point& p) {
	bool ltMidX = p.x <= midX;
	bool ltMidY = p.y <= midY;

	if(ltMidX){
		if(ltMidY) {
			return sw;
		}
		return nw;
	} else if(ltMidY) {
		return se;
	}

	return ne;
}

void Cell::Dedupe(vector<Point>& points) {
	size_t size = points.size();

	if(size == 0) {
		return;
	}

	sort(points.begin(), points.end(), SortX);

	int w = 1;
	int dupeCount = 0;

	for(int i=1; i < size; i++) {
		Point& prev = points[i-1];
		Point& curr = points[i];

		if(curr.x == prev.x && curr.y == prev.y) {
			dupeCount++;
		} else {
			dupeCount = 0;
		}

		if(dupeCount < MAX_SIZE_TOP) {
			points[w++] = points[i];
		}
	}

	points.resize(w);
}

void Cell::SortByRank() {
	sort(points_all.begin(), points_all.end(), SortRank);
}

bool Cell::Split() {
	Cell::Dedupe(points_all);
	size_t size = points_all.size();

	if(parent != nullptr && size < MAX_SIZE_LEAF) {
		return false;
	}

	float midX = rect.hx/2 + rect.lx/2;
	float midY = rect.hy/2 + rect.ly/2;
	this->midX = midX;
	this->midY = midY;

	CellRect nwRect = CellRect();
	nwRect.hx = rect.hx;
	nwRect.hy = rect.hy;
	nwRect.lx = midX;
	nwRect.ly = midY;

	CellRect neRect = CellRect();
	neRect.hx = midX;
	neRect.hy = rect.hy;
	neRect.lx = rect.lx;
	neRect.ly = midY;

	CellRect swRect = CellRect();
	swRect.hx = rect.hx;
	swRect.hy = midY;
	swRect.lx = midX;
	swRect.ly = rect.ly;

	CellRect seRect = CellRect();
	seRect.hx = midX;
	seRect.hy = midY;
	seRect.lx = rect.lx;
	seRect.ly = rect.ly;

	nw = new Cell(this, nwRect);
	ne = new Cell(this, neRect);
	sw = new Cell(this, swRect);
	se = new Cell(this, seRect);

	for(int i=0; i < points_all.size(); i++) {
		Point& p = points_all[i];
		Cell* target = FindCell(p);
		target->Add(p);
	}

	SortByRank();

	for(int i=0; i < MAX_SIZE_TOP && i < points_all.size(); i++) {
		points_best[i] = points_all[i];
	}

	largestRank = points_best[MAX_SIZE_TOP-1].rank;

	vector<Point>().swap(points_all);

	isParent = true;

	return true;
}

void Cell::FixTop(Point& p) {
	if(largestRank < p.rank) {
		return;
	}

	for(int i=MAX_SIZE_TOP-2; i >= 0; i--) {
		Point curr = points_best[i];

		if(curr.rank > p.rank) {
			points_best[i+1] = curr;
			points_best[i] = p;
		} else {
			break;
		}
	}

	largestRank = points_best[MAX_SIZE_TOP-1].rank;
}