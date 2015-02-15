//Brian Thorpe 2015
#ifndef QUAD_TREE_NODE_H
#define QUAD_TREE_NODE
#include "CommonStructs.h"
#include "QueryPoint.h"
#include "QueryRect.h"
#include "NormRect.h"
#include "Node.h"
#include "NodeVisitor.h"
bool SortPoints(const QueryPoint&  p1, const QueryPoint&  p2);

class QuadTreeNode : public Node {

private:
	int m_Best;
	bool m_IsSplit;
	bool m_IsPruned;
    NormRect m_Bounds;

	uint32_t m_XC;
	uint32_t m_YC;
	//Current Tree Depth at this node
	int m_Depth;

	//QuadTree Nodes
	QuadTreeNode* m_NLL;
	QuadTreeNode* m_NLH;
	QuadTreeNode* m_NHL;
	QuadTreeNode* m_NHH;

	//Sorted QuadtreeNodes
	QuadTreeNode* m_NA;
	QuadTreeNode* m_NB;
	QuadTreeNode* m_NC;
	QuadTreeNode* m_ND;
	//Points stored in this node
	std::vector<const QueryPoint> m_QueryPoints;
	int32_t m_MaxPoints;

public:
	QuadTreeNode(uint32_t xmin, uint32_t ymin, uint32_t xmax, uint32_t ymax, int depth, int max_points);
	void AddPoint(const QueryPoint& p);
	void AddPointSplit(const QueryPoint& p);
	void Split();
	void Visit(const QueryRect& rect, const NormRect& normalized_rect, int32_t count, VisitorQueue& visitor);
	void VisitAll(const QueryRect& rect, const NormRect& normalized_rect, int32_t count, VisitorQueue& visitor);
	void PostProcessTree();

	~QuadTreeNode();
};
#endif