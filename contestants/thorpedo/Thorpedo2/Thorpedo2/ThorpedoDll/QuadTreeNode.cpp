//Brian Thorpe 2015
#include "stdafx.h"
#include "QuadTreeNode.h"
bool SortPoints(const QueryPoint&  p1, const QueryPoint&  p2) 
{ 
	return (p1.m_Rank < p2.m_Rank); 
}	

QuadTreeNode::QuadTreeNode(uint32_t xmin, uint32_t ymin, uint32_t xmax, uint32_t ymax, int depth, int32_t max_points) : m_Best(1<<30), m_IsSplit(false),m_IsPruned(false),
	m_Bounds(xmin,ymin,xmax,ymax), m_XC((xmin + xmax) / 2), m_YC((ymin + ymax) / 2), m_Depth(depth),
	m_NLL(nullptr), m_NLH(nullptr), m_NHL(nullptr), m_NHH(nullptr),
	m_NA(nullptr), m_NB(nullptr), m_NC(nullptr), m_ND(nullptr),
	m_MaxPoints(max_points)
{
	//Do not allocate any memory in advance to avoid consuming more than is required
	m_QueryPoints.reserve(0);
}

void QuadTreeNode::AddPoint(const QueryPoint& p)
{
	//If this point is the new best for this node store it
	if(p.m_Rank < m_Best)
	{
		m_Best = p.m_Rank;
	}

	if (!m_IsSplit)
	{
		m_QueryPoints.push_back(p);
		size_t size = m_QueryPoints.size();
		if (size == MAX_TREE_POINTS && m_Depth > 0)
		{
			Split();
		}
	}
	else
	{
		AddPointSplit(p);
	}
}

void QuadTreeNode::AddPointSplit(const QueryPoint& p)
{
	//Add a point to the subdivided node
	if (NormRect::RectContainsPoint(m_Bounds.m_LX, m_Bounds.m_LY, m_XC, m_YC, p.m_NX, p.m_NY))
	{
		if(m_NLL == nullptr)
		{
			m_NLL = new QuadTreeNode(m_Bounds.m_LX, m_Bounds.m_LY, m_XC, m_YC, m_Depth - 1, m_MaxPoints);
		}
		m_NLL->AddPoint(p);
	}
	else if (NormRect::RectContainsPoint(m_Bounds.m_LX, m_YC, m_XC, m_Bounds.m_HY, p.m_NX, p.m_NY))
	{
		if(m_NLH == nullptr)
		{
			m_NLH = new QuadTreeNode(m_Bounds.m_LX, m_YC, m_XC, m_Bounds.m_HY, m_Depth-1, m_MaxPoints);
		}
		m_NLH->AddPoint(p);
	}
	else if (NormRect::RectContainsPoint(m_XC, m_Bounds.m_LY, m_Bounds.m_HX, m_YC, p.m_NX, p.m_NY))
	{
		if(m_NHL == nullptr)
		{
			m_NHL = new QuadTreeNode(m_XC, m_Bounds.m_LY, m_Bounds.m_HX, m_YC, m_Depth - 1, m_MaxPoints);
		}
		m_NHL->AddPoint(p);
	}
	else if (NormRect::RectContainsPoint(m_XC, m_YC, m_Bounds.m_HX, m_Bounds.m_HY, p.m_NX, p.m_NY))
	{
		if(m_NHH == nullptr)
		{
			m_NHH = new QuadTreeNode(m_XC, m_YC, m_Bounds.m_HX, m_Bounds.m_HY, m_Depth - 1, m_MaxPoints);
		}
		m_NHH->AddPoint(p);
	}
}

void QuadTreeNode::Split()
{
	//Set the node status to split
	m_IsSplit = true;

	while(!m_QueryPoints.empty())
	{
		const QueryPoint q = m_QueryPoints.back();
		AddPointSplit(q);
		m_QueryPoints.pop_back();
	}
	//Forces vector to free memory, since this node is now a branch and not a leaf
	std::vector<const QueryPoint>().swap(m_QueryPoints);
}

void QuadTreeNode::Visit(const QueryRect& rect, const NormRect& normalized_rect, int32_t count, VisitorQueue& visitor)
{
	size_t size = visitor.size();
	//if the visitor is full check to see if any points ranked below here are useful
	if (size >= count)
	{
		int current_rank =  visitor.top().m_Rank;
		if(current_rank < m_Best)
		{
			return;
		}
	}

	if (normalized_rect.IntersectsRect(m_Bounds.m_LX, m_Bounds.m_LY, m_Bounds.m_HX, m_Bounds.m_HY))
	{
		//If this node is completly contained visit every child node
		if(normalized_rect.ContainsRect(m_Bounds.m_LX, m_Bounds.m_LY, m_Bounds.m_HX, m_Bounds.m_HY))
		{
			VisitAll(rect, normalized_rect, count, visitor);
		}
		else
		{
			//Search children if this node is split
			if (m_IsSplit)
			{
				if(m_NA != nullptr)
					m_NA->Visit(rect, normalized_rect, count, visitor);
				if(m_NB != nullptr)
					m_NB->Visit(rect, normalized_rect, count, visitor);
				if(m_NC != nullptr)
					m_NC->Visit(rect, normalized_rect, count, visitor);
				if(m_ND != nullptr)
					m_ND->Visit(rect, normalized_rect, count, visitor);
			}
			else
			{
				int32_t highestRank = (1<<30);
				bool is_full = false;
				//If we have a full list we can set the highest rank we are searching for
				if(size >= count)
				{
					is_full = true;
					highestRank =  visitor.top().m_Rank;
				}
				for (std::vector<const QueryPoint>::iterator it = m_QueryPoints.begin(); it != m_QueryPoints.end(); ++it)
				{
					//Are we past the lowest ranked point that is useable?
					if (it->m_Rank < highestRank)
					{
						if(rect.ContainsPoint(it->m_X, it->m_Y))
						{
							visitor.push((*it));
							size++;
							//Check to see if visitor just became full if so get the highest ranked point
							if(is_full)
							{
								visitor.pop();
								highestRank =  visitor.top().m_Rank;								
							}
							else if (size == count)
							{
								highestRank =  visitor.top().m_Rank;
								is_full = true;
							}
						}
					}
					else //There are no more lower ranked points in this list
					{
						break;
					}
				}
			}
		}
	}
}

void QuadTreeNode::VisitAll(const QueryRect& rect, const NormRect& normalized_rect, int32_t count, VisitorQueue& visitor)
{
	size_t size = visitor.size();
	if (size >= count)
	{
		int current_rank =  visitor.top().m_Rank;
		if(current_rank < m_Best)
		{
			return;
		}
	}
	if (m_IsSplit)
	{
		if(m_NA != nullptr)
			m_NA->VisitAll(rect, normalized_rect, count, visitor);
		if(m_NB != nullptr)
			m_NB->VisitAll(rect, normalized_rect, count, visitor);
		if(m_NC != nullptr)
			m_NC->VisitAll(rect, normalized_rect, count, visitor);
		if(m_ND != nullptr)
			m_ND->VisitAll(rect, normalized_rect, count, visitor);
	}
	else
	{
		int32_t highestRank = (1<<30);
		bool is_full = false;
		//If we have a full list we can set the highest rank we are searching for
		if(size >= count)
		{
			is_full = true;
			highestRank =  visitor.top().m_Rank;
		}
		for (std::vector<const QueryPoint>::iterator it = m_QueryPoints.begin(); it != m_QueryPoints.end(); ++it)
		{
			//Are we past the lowest ranked point that is useable?
			if (it->m_Rank < highestRank)
			{
				if(rect.ContainsPoint(it->m_X, it->m_Y))
				{
					visitor.push((*it));
					size++;
					//Check to see if visitor just became full if so get the highest ranked point
					if(is_full)
					{
						visitor.pop();
						highestRank =  visitor.top().m_Rank;								
					}
					else if (size == count)
					{
						highestRank =  visitor.top().m_Rank;
						is_full = true;
					}
				}
			}
			else //There are no more lower ranked points in this list
			{
				break;
			}
		}
	}
}

void QuadTreeNode::PostProcessTree()
{
	if (m_IsSplit == false) //Sort if the tree has values
	{
		std::sort(m_QueryPoints.begin(), m_QueryPoints.end(), SortPoints);		
	}
	else //If the tree is split prune children and sort the nodes based on the node with the highest ranked points
	{
		if(m_NLL != nullptr)
			m_NLL->PostProcessTree();
		if(m_NLH != nullptr)
			m_NLH->PostProcessTree();
		if(m_NHL != nullptr)
			m_NHL->PostProcessTree();
		if(m_NHH != nullptr)
			m_NHH->PostProcessTree();
		
		//Manual Sorting of 4 child nodes
		int ll = 0;
		int lh = 0;
		int hl = 0;
		int hh = 0;

		int ranka = -1;
		int rankb = -1;
		int rankc = -1;
		int rankd = -1;

		m_NA = m_NLL;
		m_NB = m_NLH;
		m_NC = m_NHL;
		m_ND = m_NHH;
		if(m_NA != nullptr)
			ranka = m_NA->m_Best;
		if(m_NB != nullptr)
			rankb = m_NB->m_Best;
		if(m_NC != nullptr)
			rankc = m_NC->m_Best;
		if(m_ND != nullptr)
			rankd = m_ND->m_Best;
		//SWAP IF A Null with B, C or D
		if(m_NA == nullptr)
		{
			if(m_NB != nullptr)
			{
				m_NA = m_NB;
				m_NB = nullptr;
			}
			else if(m_NC != nullptr)
			{
				m_NA = m_NC;
				m_NC = nullptr;
			}
			else if(m_ND != nullptr)
			{
				m_NA = m_ND;
				m_ND = nullptr;
			}
		}
		//SWAP IF B Null with C or D
		if(m_NB == nullptr)
		{
			if(m_NC != nullptr)
			{
				m_NB = m_NC;
				m_NC = nullptr;
			}
			else if(m_ND != nullptr)
			{
				m_NB = m_ND;
				m_ND = nullptr;
			}
		}
		//SWAP IF C Null with D
		if(m_NC == nullptr)
		{
			if(m_ND != nullptr)
			{
				m_NC = m_ND;
				m_ND = nullptr;
			}
		}
		//If A isn't null check swapping with B, C, D
		if (m_NA != nullptr)
		{
			if(m_NB != nullptr)
			{
				if (m_NA->m_Best > m_NB->m_Best)
				{
					QuadTreeNode* temp = m_NB;
					m_NB = m_NA;
					m_NA = temp;
					
				}
			}
			if(m_NC != nullptr)
			{
				if (m_NA->m_Best > m_NC->m_Best)
				{
					QuadTreeNode* temp = m_NC;
					m_NC = m_NA;
					m_NA = temp;
				}
			}
			if(m_ND != nullptr)
			{
				if (m_NA->m_Best > m_ND->m_Best)
				{
					QuadTreeNode* temp = m_ND;
					m_ND = m_NA;
					m_NA = temp;
				}
			}
		}
		
		//If B isn't null check swapping with  C, D
		if (m_NB != nullptr)
		{
			if(m_NC != nullptr)
			{
				if (m_NB->m_Best > m_NC->m_Best)
				{
					QuadTreeNode* temp = m_NC;
					m_NC = m_NB;
					m_NB = temp;
				}
			}
			if(m_ND != nullptr)
			{
				if (m_NB->m_Best > m_ND->m_Best)
				{
					QuadTreeNode* temp = m_ND;
					m_ND = m_NB;
					m_NB = temp;
				}
			}
		}
		
		//If C isn't null check swapping with  C, D
		if (m_NC != nullptr)
		{
			if(m_ND != nullptr)
			{
				if (m_NC->m_Best > m_ND->m_Best)
				{
					QuadTreeNode* temp = m_ND;
					m_ND = m_NC;
					m_NC = temp;
				}
			}
		}
	}
}
QuadTreeNode::~QuadTreeNode()
{	
	if (m_IsSplit)
	{
		if(m_NLL != nullptr)
			delete m_NLL;
		if(m_NLH != nullptr)
			delete m_NLH;
		if(m_NHL != nullptr)
			delete m_NHL;
		if(m_NHH != nullptr)
			delete m_NHH;
		m_NLL = nullptr;
		m_NLH = nullptr;
		m_NHL = nullptr;
		m_NHH = nullptr;
		m_NA = nullptr;
		m_NB = nullptr;
		m_NC = nullptr;
		m_ND = nullptr;
	}
}