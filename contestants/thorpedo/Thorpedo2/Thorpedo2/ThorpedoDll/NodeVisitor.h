//Brian Thorpe 2015
#ifndef NODE_VISITOR_H
#define NODE_VISITOR_H
#include "QueryPoint.h"
class PointComparison
{
	bool reverse;
public:
	PointComparison(const bool& revparam=false) {reverse=revparam;}
	bool operator() (const QueryPoint& lhs, const QueryPoint& rhs) const
	{
		if (reverse) return (lhs.m_Rank > rhs.m_Rank);
		else return (lhs.m_Rank < rhs.m_Rank);
	}
};

typedef std::priority_queue<const QueryPoint, std::vector<const QueryPoint>, PointComparison> VisitorQueue;

#endif