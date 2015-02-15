#include "stdafx.h"
#include "Block.h"

bool BlockNodeLevelCompare::operator()(BlockNode *& nodeA, BlockNode *& nodeB) const
{
	return nodeA->MinimumRank > nodeB->MinimumRank;
}
