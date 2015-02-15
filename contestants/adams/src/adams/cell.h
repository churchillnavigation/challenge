#include "stdafx.h"
#include "point_search.h"
#include <vector>

using namespace std;

const int MAX_SIZE_TOP = 20;
const int MAX_SIZE_LEAF = 10 * 1000; // 0.041
// const int MAX_SIZE_LEAF = 100 * 1000;  // 0.07
// const int MAX_SIZE_LEAF = 1 * 1000;   // 0.08
// const int MAX_SIZE_LEAF = 20 * 1000; // 0.043
//const int MAX_SIZE_LEAF = 5 * 1000; // 0.045
// const int MAX_SIZE_LEAF = 2 * 1000; // 0.059
// const int MAX_SIZE_LEAF = 50 * 1000; // 0.05
// const int MAX_SIZE_LEAF = 15 * 1000; // 0.048

struct CellRect {
	float lx;
	float ly;
	float hx;
	float hy;

	bool MayContain(Point& p){ 
		return p.x >= hx && p.x <= lx && p.y >= ly && p.y <= hy;
	}
};

enum CellMatch {
	NONE, PARTIAL, FULL
};

class Cell {
public:
	Cell();
	~Cell();
	Cell(Cell* parent, CellRect rect);
	void Add(Point& p);
	Cell* FindCell(Point& p);
	void FixTop(Point& p);
	bool Split();
	bool MayContain(Point& p);
	CellMatch Match(CellRect rect);
	void SortByRank();

	float midX;
	float midY;
	Cell* nw;
	Cell* ne;
	Cell* se;
	Cell* sw;
	Cell* parent;
	int level;
	int largestRank;
	bool isParent;
	bool isEmpty;
	CellRect rect;
	Point points_best[MAX_SIZE_TOP];
	vector<Point> points_all;

	static int CellCount;
	static int MaxLevel;
	static void Dedupe(vector<Point>& points);
};