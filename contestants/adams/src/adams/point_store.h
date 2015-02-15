#include "stdafx.h"
#include "cell.h"
#include <vector>
#include <deque>

using namespace std;

class PointStore {
public:
	PointStore();
	~PointStore();
	Cell& Add(Point& p);
	void PrintStatistics();
	int CellsForRect(CellRect rect, Point* points, int topN);
	void PointStore::FindRealRoot();
	int Count();
	void Sort();

protected:
	int CellsForRect(CellRect rect, Point* points, Cell& cell, int topN);
	Cell& Add(Point& p, Cell& cell);
	void Sort(Cell& c);
	void Count(Cell& c, int& cnt);
	Cell* PointStore::FindRealRoot(Cell& cell);
	Cell root;
	Cell* realRoot;
	vector<Point> searchBuffer;
	vector<Point> floaters;
};

