#include "point_search.h"

enum Collision
{
	None,
	Partial,
	AContainsB,
	BContainsA,
};

Collision GetCollision(Rect & a, Rect & b)
{
	if (a.lx <= b.lx)
	{

	}
}