#ifndef __TREE_H
#define __TREE_H

#include "point_search.h"
#include <vector>

struct Query
{
	Rect rect;
	int32_t capacity;
	int32_t count;
	Point *points;
	int min_rank;
};

class Tree
{
public:
	Tree(Rect span);

	virtual ~Tree();

	void add_point(const Point &pt);
	
	void query(Query &query);
	
	void print_stats();

private:
	friend class Tree2;

	struct Stats
	{
		int nodes;
		int levels;
		int points;
	};

	virtual void ensure_children();
	
	void gather_stats(Stats &stats, int level);
	
	void process_node(Query &q, std::vector<Tree*> &bfs);

	Tree* children[4];

	Rect span;	
	std::vector<Point> points;
	int32_t min_rank;
#ifdef USE_POINT_OVERLAP
	PointOverlap overlap;
#endif
};

#endif
