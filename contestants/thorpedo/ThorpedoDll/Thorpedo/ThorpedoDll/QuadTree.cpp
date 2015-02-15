//Brian Thorpe 2015
#include "stdafx.h"
#include "QuadTree.h"
QuadTree::QuadTree(float scaling, float x_min, float y_min, float x_max, float y_max, int count) : m_Queries(0), m_Scaling(scaling), m_X_Min(x_min),
	m_Y_Min(y_min), m_X_Range(x_max-x_min), m_Y_Range(y_max-y_min),
	m_Root(0, 0, MAX_TREE_SIZE, MAX_TREE_SIZE, MAX_TREE_DEPTH){}

///AddPoint
///Adds a new Points to the Quadtree
void QuadTree::AddPoint(const Point* p)
{
	const QueryPoint qp(p, m_X_Min, m_X_Range, m_Y_Min, m_Y_Range);
	m_Root.AddPoint(qp);	
	
}

//GetPoints
//Finds a number of points upto count which are inside the query rectangle
size_t QuadTree::GetPoints(const Rect rect,  const int32_t count, Point* out_points)
{
	const NormRect normRect = NormRect::NormalizedRectFromRect(rect, m_X_Min, m_X_Range, m_Y_Min, m_Y_Range);
	const QueryRect origRect(rect);

	m_Root.Visit(origRect, normRect, count, m_Visitor);
	
	//Take the minimum of the size or the total count; Visitor should not be larger than size;
	size_t total_points = min(count, m_Visitor.size());

	size_t i = total_points;

	//Set the points backwards so the highest point is last
	while (!m_Visitor.empty())
	{
		i--;
		out_points[i].id = m_Visitor.top().m_Id;
		out_points[i].rank = m_Visitor.top().m_Rank;
		out_points[i].x = m_Visitor.top().m_X;
		out_points[i].y = m_Visitor.top().m_Y;
		m_Visitor.pop();
	}	
	return total_points;
}

//PostProcessTree
//Remove empty nodes, and sort the tree so the searches start in branches with
//the highest ranked points
void QuadTree::PostProcessTree()
{
	m_Root.PostProcessTree();
}

//Construct From Points
//Construct the QuadTree from a set of points
QuadTree* QuadTree::ConstructFromPoints(const Point* points_begin, const Point* points_end)
{
	float x_min = FLT_MAX;
	float x_max = -FLT_MAX;
	float y_min = FLT_MAX;
	float y_max = -FLT_MAX;

	const Point* start = points_begin;
	const Point* end = points_end;

	int valid_points = 0;

	//find min and max bounds excluding extreme points
	start = points_begin;
	while(start != end)
	{
		//Discard 4 extreme points
		if (abs(start->x) < 1e9 && abs(start->y) < 1e9)
		{
			if(start->x < x_min)
				x_min = start->x;
			else if(start->x > x_max)
				x_max = start->x; 

			if(start->y < y_min)
				y_min = start->y;
			else if(start->y > y_max)
				y_max = start->y; 
		
			valid_points++;
		}
		start++;
	}

	//Create a new quadtree and store the points
	QuadTree* quad_tree = new QuadTree(1.0f, x_min, y_min, x_max, y_max, valid_points);

	start = points_begin;
	while(start != end)
	{
		//Discard 4 extreme points
		if (abs(start->x) < 1e9 && abs(start->y) < 1e9)
		{
			quad_tree->AddPoint(start);
		}
		start++;
	}

	quad_tree->PostProcessTree();
	return quad_tree;
}

QuadTree::~QuadTree()
{
}