#include "stdafx.h"
#include "point_store.h"
#include <algorithm>
#include <iostream>
#include <time.h>
#include <queue>

using namespace std;

PointStore::PointStore() {
	CellRect rootRect;
	rootRect.hx = -1e+10;
	rootRect.hy = 1e+10;
	rootRect.lx = 1e+10;
	rootRect.ly = -1e+10;
	root = Cell(nullptr, rootRect);
	root.Split();
}

PointStore::~PointStore() {
}

Cell& PointStore::Add(Point& p) {
	Cell& cell = Add(p, root);
	return cell;
}

Cell& PointStore::Add(Point& p, Cell& cell) {
	Cell* target = &cell;

	while(true) {
		target = target->FindCell(p);

		if(target->isParent) {
			if(p.rank < target->largestRank) {
				target->FixTop(p);
			}
		} else if(target->points_all.size() >= MAX_SIZE_LEAF) {
			bool split = target->Split();
			if(!split) {
				target->Add(p);
				return *target;
			} else {
				target->FixTop(p);
			}
		} else {
			target->Add(p);
			return *target;
		}
	}
}

int PointStore::CellsForRect(CellRect rect, Point* points, int topN) {
	int cnt = CellsForRect(rect, points, root, topN);
	return cnt;
}

class RankComparision
{
public:
	bool operator() (Point& a, Point& b) 
	{
		return a.rank < b.rank;
	}
};

int PointStore::Count() {
	int cnt = 0;
	Count(root, cnt);
	return cnt;
}

bool SortRankStack(Point& a, Point& b) {
	return a.rank < b.rank;
}

void PointStore::FindRealRoot() {
	floaters.clear();
	realRoot = FindRealRoot(root);
	sort(floaters.begin(), floaters.end(), SortRankStack);
}

Cell* PointStore::FindRealRoot(Cell& cell) {
	if(!cell.isParent){
		for(int i=0;i<cell.points_all.size();i++){
			floaters.push_back(cell.points_all[i]);
		}
		return nullptr;
	}

	if(cell.nw->isParent && cell.ne->isParent && cell.sw->isParent && cell.se->isParent) {
		return &cell;
	}

	Cell* cnw = FindRealRoot(*cell.nw);
	Cell* csw = FindRealRoot(*cell.sw);
	Cell* cne = FindRealRoot(*cell.ne);
	Cell* cse = FindRealRoot(*cell.se);

	if(cnw != nullptr){
		return cnw;
	}

	if(csw != nullptr){
		return csw;
	}
	
	if(cne != nullptr){
		return cne;
	}

	if(cse != nullptr){
		return cse;
	}

	return nullptr;
}

void PointStore::Count(Cell& c, int& cnt) {
	if(c.isParent) {
		Count(*c.nw, cnt);
		Count(*c.ne, cnt);
		Count(*c.sw, cnt);
		Count(*c.se, cnt);
	} else {
		cnt+= (int)c.points_all.size();
	}
}

int PointStore::CellsForRect(CellRect rect, Point* points, Cell& root, int topN) {
	searchBuffer.resize(topN);
	for(int i=0; i<topN;i++) {
		searchBuffer[i].rank = INT_MAX;
	}

	vector<Cell*> stack;
	stack.push_back(root.nw);
	stack.push_back(root.sw);
	stack.push_back(root.ne);
	stack.push_back(root.se);

	int rank = INT_MAX;

	while(!stack.empty()) {
		Cell& cell = *stack.back();
		stack.pop_back();
		CellMatch m = cell.Match(rect);

		if(m == FULL) {
			if(cell.isParent){
				for(int i=0; i< MAX_SIZE_TOP; i++) {
					Point& p = cell.points_best[i];
					if(p.rank >= rank) {
						break;
					}
					pop_heap(searchBuffer.begin(),searchBuffer.end(),SortRankStack);
					searchBuffer[topN-1] = p;
					push_heap (searchBuffer.begin(),searchBuffer.end(),SortRankStack);
					rank = searchBuffer.front().rank;
				}
			} else {
				for(int i=0; i< cell.points_all.size(); i++) {
					Point& p = cell.points_all[i];
					if(p.rank > rank) {
						break;
					}
					pop_heap(searchBuffer.begin(),searchBuffer.end(),SortRankStack);
					searchBuffer[topN-1] = p;
					push_heap (searchBuffer.begin(),searchBuffer.end(),SortRankStack);
					rank = searchBuffer.front().rank;
				}
			}
		} else if(m == PARTIAL) {
			if(cell.isParent) {
				if(cell.points_best[0].rank < rank) 
				{
					stack.push_back(cell.nw);
					stack.push_back(cell.ne);
					stack.push_back(cell.sw);
					stack.push_back(cell.se);
				}
			} else {
				for(int i=0; i< cell.points_all.size(); i++) {
					Point& p = cell.points_all[i];
					if(p.rank > rank) {
						break;
					}
					if(rect.MayContain(p)) {
						pop_heap(searchBuffer.begin(),searchBuffer.end(),SortRankStack);
						searchBuffer[topN-1] = p;
						push_heap (searchBuffer.begin(),searchBuffer.end(),SortRankStack);
						rank = searchBuffer.front().rank;
					} 
				}
			}
		}
	}

	int size = topN;
	while(size > 0 && searchBuffer.front().rank == INT_MAX){
		pop_heap(searchBuffer.begin(),searchBuffer.begin() + size,SortRankStack);
		size--;
	}

	for(int i=size-1; i >=0; i--) {
		points[i] = searchBuffer.front();
		pop_heap(searchBuffer.begin(),searchBuffer.begin() + i + 1,SortRankStack);
	}

	return size;
}

void PointStore::Sort() {
	Sort(root);
}

void PointStore::Sort(Cell& c) {
	if(c.isParent){
		Sort(*c.nw);
		Sort(*c.ne);
		Sort(*c.sw);
		Sort(*c.se);
	} else {
		c.SortByRank();
	}
}

void PointStore::PrintStatistics() {
	cout << "Max level: " << Cell::MaxLevel << endl;
	cout << "Cell count: " << Cell::CellCount << endl;
}