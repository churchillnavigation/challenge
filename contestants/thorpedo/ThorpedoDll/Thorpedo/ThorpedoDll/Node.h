//Brian Thorpe 2015
#ifndef NODE_H
#define NODE_H
#include "CommonStructs.h"
#include "QueryPoint.h"
#include "QueryRect.h"
#include "NodeVisitor.h"
class Node {

protected:

public:
	virtual void Visit(const QueryRect& rect, const NormRect& normalized_rect, int32_t count, VisitorQueue& visitor) = 0;
};
#endif