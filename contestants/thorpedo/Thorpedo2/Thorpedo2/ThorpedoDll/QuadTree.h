//Brian Thorpe 2015
#ifndef QUAD_TREE_H
#define QUAD_TREE_H
#include "CommonStructs.h"
#include "QueryPoint.h"
#include "QuadTreeNode.h"
class QuadTree {
private:
	
	//Reference to generated Query Points
	std::vector<const QueryPoint> m_QueryPoints;
	int m_Queries;

	//QuadTree Visitor
	VisitorQueue m_Visitor;

	//Reference to root node
	QuadTreeNode m_Root1;
	QuadTreeNode m_Root2;
	QuadTreeNode m_Root3;
	QuadTreeNode m_Root4;
	QuadTreeNode m_Root5;
	//Tree Bounds
	float m_Scaling;
	float m_X_Min;
	float m_X_Range;
	float m_Y_Min;
	float m_Y_Range;

public:
	QuadTree(float scaling, float xmin, float xmax, float ymin, float ymax,int count);
	void AddPoint(const Point* p);
	void AddRect(const Rect r);
	size_t GetPoints(const Rect rect,  const int32_t count, Point* out_points);
	void WritePoints();
	void WriteRects();
	void PostProcessTree();
	static QuadTree* ConstructFromPoints(const Point* points_begin, const Point* points_end);
	~QuadTree();
};
#endif
