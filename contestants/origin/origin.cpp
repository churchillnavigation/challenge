//Everything copyright Graham Clenaghan, 2015

#include <vector>
#include <queue>
#include <algorithm>
#include "point_search.h"

class TreeNode
{
public:
	TreeNode(Point point, const bool sn);
	TreeNode();
	~TreeNode();
	Point p;
	TreeNode *ne;
	TreeNode *sw;
	const bool ns;

	void insert(Point point);
};

struct SearchContext
{
	SearchContext(const Point* points_begin, const Point* points_end);
	int32_t search(const Rect rect, const int32_t count, Point* out_points);

	TreeNode kdtree;
};

inline bool sortpoint(const Point a, const Point b)
{
	return a.rank < b.rank;
}

struct sortnode
{
	bool operator()(const TreeNode *a, const TreeNode *b) const
	{
		return a->p.rank > b->p.rank;
	}
};

//SearchContext definitions-------------------------------------------------------------------------------------
SearchContext::SearchContext(const Point* points_begin, const Point* points_end)
{
	std::vector<Point> points(points_begin, points_end);
	for (auto it = points.begin(); it != points.end(); ++it)
	{
		if (it->x > 5000 || it->x < -5000 || it->y > 5000 || it->y < -5000)
		{ //There are points that never seem to be in any rectangle, might as as well toss 'em.
			points.erase(it);
		}
	}
	std::sort(points.begin(), points.end(), sortpoint);

	for (auto it : points)
	{
		kdtree.insert(it);
	}
}

//TreeNode definitions-------------------------------------------------------------------------------------------
TreeNode::TreeNode(Point point, const bool sn) : ne(nullptr) , sw(nullptr) , ns(sn)
{
	p = point;
}
TreeNode::TreeNode() : ne(nullptr) , sw(nullptr) , ns(true)
{
	p.rank = -1;
}
TreeNode::~TreeNode()
{
	delete ne;
	delete sw;
}

void TreeNode::insert(Point point)
{
	if (p.rank == -1)
	{
		p = point;
		return;
	}

	if (ns)
	{
		if (p.y > point.y)
		{
			if (sw)
			{
				sw->insert(point);
			} else {
				sw = new TreeNode(point, false);
			}
		} else {
			if (ne)
			{
				ne->insert(point);
			} else {
				ne = new TreeNode(point, false);
			}
		}
	} else {
		if (p.x > point.x)
		{
			if (sw)
			{
				sw->insert(point);
			} else {
				sw = new TreeNode(point, true);
			}
		} else {
			if (ne)
			{
				ne->insert(point);
			} else {
				ne = new TreeNode(point, true);
			}
		}
	}
}

int32_t SearchContext::search(const Rect rect, const int32_t count, Point *out_points)
{
	std::priority_queue<TreeNode *, std::vector<TreeNode *>, sortnode> nodes; //list of nodes currently being searched
	TreeNode *currnode = nullptr;
	int32_t c = 0;
	nodes.push(&kdtree);
	
	while(c < count && !nodes.empty())
	{
		currnode = nodes.top();
		nodes.pop();
		if (currnode->p.rank == -1)
		{
			continue;
		} else {
			switch ((((rect.lx <= currnode->p.x) ? 1 : 0) | ((rect.hx >= currnode->p.x) ? 2 : 0) | ((rect.ly <= currnode->p.y) ? 4 : 0) | ((rect.hy >= currnode->p.y) ? 8 : 0) | (currnode->ns ? 16 : 0)))
			{
			case 5:
			case 9:
			case 13:
			case 21:
			case 22:
			case 23:
				if (currnode->sw)
				{
					nodes.push(currnode->sw);
				}
				break;
			case 6:
			case 10:
			case 14:
			case 25:
			case 26:
			case 27:
				if (currnode->ne)
				{
					nodes.push(currnode->ne);
				}
				break;
			case 7:
			case 11:
			case 29:
			case 30:
				if (currnode->ne)
				{
					nodes.push(currnode->ne);
				}
				if (currnode->sw)
				{
					nodes.push(currnode->sw);
				}
				break;
			case 15:
			case 31:
				*out_points = currnode->p;
				out_points++;
				c++;
				if (currnode->ne)
				{
					nodes.push(currnode->ne);
				}
				if (currnode->sw)
				{
					nodes.push(currnode->sw);
				}
				break;
			}
		}
	}
	
	return c;
}

//External calls--------------------------------------------------------------------------------------------------
extern "C" __declspec(dllexport) SearchContext* __stdcall create(const Point* points_begin, const Point* points_end)
{
	return new SearchContext(points_begin, points_end);
}

extern "C" __declspec(dllexport) SearchContext* __stdcall destroy(SearchContext* sc)
{
	delete sc;
	return 0;
}

extern "C" __declspec(dllexport) int32_t __stdcall search(SearchContext* sc, const Rect rect, const int32_t count, Point* out_points)
{
	return sc->search(rect, count, out_points);
}
