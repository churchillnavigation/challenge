#include "Tree.h"
#include "Profiler.h"
#include <assert.h>
#include <cmath>
#include <algorithm>
#include <iostream>
#include <cstring>
#include <queue>
#include <limits>
#include <omp.h>

using namespace std;

#ifndef PER_NODE
#  ifdef TREE_REBALANCE
#    define PER_NODE (4*8) // FIXME check performance repercussions of this
#  else
#    define PER_NODE 32
#  endif
#endif

inline bool inside(Rect r, Point p)
{
	return (r.lx <= p.x) & (p.x <= r.hx)
	     & (r.ly <= p.y) & (p.y <= r.hy);
}

inline bool overlaps(Rect a, Rect b)
{
	// http://stackoverflow.com/questions/306316/determine-if-two-rectangles-overlap-each-other
	
	return (a.lx < b.hx) && (a.hx > b.lx)
	    && (a.ly < b.hy) && (a.hy > b.ly);
}

//static void __attribute__((noinline)) insert_point(Query &q, const Point &p)
inline void insert_point(Query &q, const Point &p)
{
	int insert = -1;

	for (int i = q.count; i >= 0; i--)
	{
		if (q.points[i].rank > p.rank)
			insert = i;
	}
	
	if (insert == -1 && q.count == q.capacity)
		return;
	else if (insert == -1)
	{
		q.points[q.count++] = p;
	}
	else
	{
		q.count = min(q.capacity, q.count + 1);
		for (int i = q.count - 1; i > insert; i--)
		{
			q.points[i] = q.points[i - 1];
		}
		q.points[insert] = p;
	}
	
	q.min_rank = q.points[q.count - 1].rank;
}

Tree::Tree(Rect span_):
	span(span_),
	min_rank(numeric_limits<int32_t>::max()),
	local_min_rank(numeric_limits<int32_t>::max())
{
	children[0] = 0;
	children[1] = 0;
	children[2] = 0;
	children[3] = 0;

	points.reserve(PER_NODE);
}

Tree::~Tree()
{
	delete children[0];
	delete children[1];
	delete children[2];
	delete children[3];
}

void Tree::ensure_children()
{
	if (children[0])
		return;
	
	float mx = 0.5 * (span.lx + span.hx);
	float my = 0.5 * (span.ly + span.hy);
	
	float mx_ = nextafter(mx, span.hx);
	float my_ = nextafter(my, span.hy);
	
	Rect s1{span.lx, span.ly, mx,      my};
	Rect s2{mx_,     span.ly, span.hx, my};
	Rect s3{span.lx, my_,     mx,      span.hy};
	Rect s4{mx_,     my_,     span.hx, span.hy};
	
	children[0] = new Tree(s1);
	children[1] = new Tree(s2);
	children[2] = new Tree(s3);
	children[3] = new Tree(s4);
}

void Tree::add_point(const Point &pt)
{
	min_rank = min(min_rank, pt.rank);

	// Since we are initing root node with small_span(), some points might lie outside
	// of node span. We add these points without question.
	if (!inside(span, pt))
	{
		local_min_rank = min(local_min_rank, pt.rank);
		points.push_back(pt);
		return;
	}
	
	if (points.size() < PER_NODE)
	{
		local_min_rank = min(local_min_rank, pt.rank);
		points.push_back(pt);
	}
	else
	{
		ensure_children();

		float mx = 0.5 * (span.lx + span.hx);
		float my = 0.5 * (span.ly + span.hy);
		
		if (pt.x <= mx && pt.y <= my)
			children[0]->add_point(pt);
		else if (pt.x > mx && pt.y <= my)
			children[1]->add_point(pt);
		else if (pt.x <= mx && pt.y > my)
			children[2]->add_point(pt);
		else if (pt.x > mx && pt.y > my)
			children[3]->add_point(pt);
	}
}

void Tree::process_node(Query &q, vector<Tree*> &bfs)
{
	// TODO: Port faster overlapping test from Tree2?
	if (overlaps(q.rect, span))
	{
		PROFILE(ctx->NodesVisited++)		
		PROFILE(ctx->tmp0++)
		PROFILE(ctx->tmp1 = 0)

		for (const Point &p: points)
		{
			if (q.count == q.capacity && p.rank > q.min_rank)
			{
				PROFILE(ctx->PointsBypassed += points.size() - ctx->tmp1)
				break;
			}
			PROFILE(ctx->tmp1++)
		
			PROFILE(ctx->PointsChecked++)

			if (inside(q.rect, p))
			{
				insert_point(q, p);
				PROFILE(ctx->PointsInside++)
			}
		}
		
		if (children[0])
		{
			for (int i = 0; i < 4; i++)
			{
				if (q.count < q.capacity || q.min_rank > children[i]->min_rank)
				{
					bfs.push_back(children[i]);
				}
			}
		}
	}
	else
	{
		PROFILE(ctx->OverlapFailed++)
	}
}

void Tree::query(Query& q)
{
	PROFILE(ctx->Queries++)
	PROFILE(ctx->tmp0 = 0)

	vector<Tree*> bfs;
	bfs.reserve(64);
	bfs.push_back(this);
	
	unsigned it = 0;

	while(it < bfs.size())
	{
		PROFILE(ctx->MaxQueue =  max<int>(ctx->MaxQueue, bfs.size()))

		Tree *t = bfs[it++];
		t->process_node(q, bfs);
	}
	
	PROFILE(if (q.count == 0) ctx->EmptyQueries ++)
	PROFILE(if (q.count == q.capacity) ctx->FullQueries ++)
	PROFILE(if (ctx->tmp0 <= 15) ctx->FastQueries ++)
	PROFILE(if (ctx->tmp0 >= 50) ctx->SlowQueries ++)
	PROFILE(if (bfs.size() >= 64) ctx->LongQueries ++)
}

void Tree::print_stats()
{
	Stats stats;
	memset(&stats, 0, sizeof stats);
	gather_stats(stats, 1);
	
	cout << "Stats for " << this << endl;
	cout << "# levels    : " << stats.levels << endl;
	cout << "# nodes     : " << stats.nodes << endl;
	cout << "Memory usage: " << (stats.nodes * sizeof(Tree) + stats.points * sizeof(Point))/1024/1024 << " MB" << endl;
}

void Tree::gather_stats(Stats &stats, int level)
{
	stats.nodes ++;
	stats.points += points.size();
	
	if (level > stats.levels)
		stats.levels = level;	
	
	if (children[0])
	{
		children[0]->gather_stats(stats, level + 1);
		children[1]->gather_stats(stats, level + 1);
		children[2]->gather_stats(stats, level + 1);
		children[3]->gather_stats(stats, level + 1);
	}
}

void Tree::rebalance()
{
}

void Tree::recalculate()
{
	min_rank = local_min_rank;

	if (!children[0])
		return;
	
	for (int i = 0; i < 4; i++)
		min_rank = min(min_rank, children[i]->min_rank);
}
