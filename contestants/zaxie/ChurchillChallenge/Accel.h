#ifndef ACCEL_H
#define ACCEL_H

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <crtdbg.h>
#include <stdint.h>
#include <vector>
#include <ppl.h>
#include <concurrent_vector.h>
#include "pq.h"

using PriorityQueue = PriorityQueue_STL;

#pragma pack(push, 1)
namespace Input
{
	struct Point
	{
		int8_t id;
		int32_t rank;
		float x, y;
	};

	struct Rect
	{
		float lx;
		float ly;
		float hx;
		float hy;

		bool overlaps(float x, float y) const
		{
			return x >= lx && x <= hx && y >= ly && y <= hy;
		}
		bool overlaps(const Rect &other) const
		{
			if (lx > other.hx || hx < other.lx) return false;
			if (ly > other.hy || hy < other.ly) return false;
			return true;
		}
		bool overlapsExclusive(const Rect &other) const
		{
			if (lx >= other.hx || hx <= other.lx) return false;
			if (ly >= other.hy || hy <= other.ly) return false;
			return true;
		}
		Rect splitLeft(int dim, float median) const
		{
			_ASSERTE(dim == 0 || dim == 1);
			if (dim == 0) return Rect{ lx, ly, median, hy };
			else return Rect{ lx, ly, hx, median };
		}
		Rect splitRight(int dim, float median) const
		{
			_ASSERTE(dim == 0 || dim == 1);
			if (dim == 0) return Rect{ median, ly, hx, hy };
			else return Rect{ lx, median, hx, hy };
		}
	};

	static_assert(sizeof(Rect) == 16, "Rect not packed");
}
#pragma pack(pop)

using Input::Rect;	// No alignment problems

struct Point
{
	float data[2];
	uint32_t id : 8;
	uint32_t rank : 24;

	Point()
	{
	}
	explicit Point(const Input::Point &ip)
	{
		_ASSERTE(ip.rank >= 0 && ip.rank < (1 << 24));
		data[0] = ip.x;
		data[1] = ip.y;
		id = ip.id;
		rank = ip.rank;
	}
	float operator[](int dim) const
	{
		_ASSERTE(dim == 0 || dim == 1);
		return data[dim];
	}
	Input::Point asInputPoint() const
	{
		return Input::Point{ id, rank, data[0], data[1] };
	}
	bool operator==(const Point &rhs) const
	{
		_ASSERTE(rank != rhs.rank || (data[0] == rhs[0] && data[1] == rhs[1]));
		return rank == rhs.rank;
	}

	struct rank_lt
	{
		bool operator()(const Point &p1, const Point &p2) const
		{
			return p1.rank < p2.rank;
		}
	};

	struct dim_lt
	{
		int dim;
		bool operator()(const Point &p1, const Point &p2) const
		{
			return p1[dim] < p2[dim];
		}
	};

	struct median_p
	{
		int dim;
		float median;
		bool operator()(const Point &p) const
		{
			return p[dim] < median;
		}
	};
};

static_assert(sizeof(Point) == 12, "Point not packed");

typedef std::vector<Point> PointV;

class KDTree
{
public:
	struct Node
	{
		uint32_t	rank : 30;
		uint32_t	dim : 1;
		uint32_t	branch : 1;
		uint32_t	left;
		uint32_t	right;
		float		median;			// median is the minimum value in right subtree

		// median & dimension aren't important in leaf nodes
		static Node makeLeaf(int rank, unsigned left, unsigned right)
		{
			_ASSERTE(rank >= 0 && rank < ((1 << 30) - 1));
			return Node{ rank, 0, 0, left, right, 0 };
		}
		static Node makeBranch(int dim, unsigned rank, unsigned left, unsigned right, float median)
		{
			_ASSERTE(rank >= 0 && rank < ((1 << 30) - 1));
			return Node{ rank, dim, 1, left, right, median };
		}
	};

	static_assert(sizeof(Node) == 16, "node not packed");
	
	KDTree(PointV &points, unsigned leafSize);
	std::vector<Point> overlap(const Rect &rect);

	const Node &node(unsigned nodei) const
	{
		return _nodes[nodei];
	}

private:

	struct Split
	{
		int dim;
		float range, median;	// actual values
		unsigned skew;			// distance of lowest median index from the center
	};

	const unsigned _leafSize;
	PointV &_points;
    concurrency::concurrent_vector<Node> _cnodes;   // use concurrent vector during construction
    std::vector<Node> _nodes;                       // but ordinary for lookups [no sync penalty]
	const unsigned _root;
	const Rect *_query;
    std::vector<Point> _queryResult;

	Point *P(unsigned i)
	{
#ifdef _DEBUG
		if (i == _points.size())
			return (&_points[i - 1]) + 1;
#endif
		return &_points[i];
	}
	unsigned I(Point *p)
	{
		return (unsigned)(p - &_points[0]);
	}

	unsigned build();
	unsigned build(unsigned begin, unsigned end);
	Split selectDim(unsigned begin, unsigned end);
	Split findSpread(int dim, unsigned begin, unsigned end);
	void collectOverlaps(const unsigned nodei, Rect nodeRange, concurrency::combinable<PriorityQueue> &cpq);

	void validate();
	void validateIndexSpans(const std::vector<unsigned> &leaves);
	void validateBBSpans(std::vector<Rect> &bbs);
	void collectLeafNodes(const unsigned nodei, const Rect &rect, std::vector<unsigned> &leaves, std::vector<Rect> &bbs);
	void validateBranch(const Node &node);
	void validateLeaf(const Node &node, const Rect &rect);
};

PointV CopyInput(const Input::Point *begin, const Input::Point *end);
void CopyOutput(std::vector<Point> &points, const int count, Input::Point *out);
std::vector<Point> SlowSearch(const PointV &points, const Rect &rect);
std::vector<Point> SnailSearch(const PointV &points, const Rect &rect);
void PqTest();

#endif
