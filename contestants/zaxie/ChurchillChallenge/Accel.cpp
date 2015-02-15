#include <algorithm>
#include "Accel.h"
#include "pq.h"


// BUILDING /////////////////////////////////////////////////////////////////
// See Friedman: An Algorithm for Finding Best Matches in Logarithmic Expected Time
// ACM Trans on Mathematical Software, Vol 3, No 3, Sep 1977, p 209-226

KDTree::KDTree(PointV &points, unsigned leafSize) :
    _leafSize(leafSize),
    _points(points),
    _root(build())
	// NB! _nodes member must be declared EARLIER than _root because it must be constructed before being used during build.
{
    _nodes.reserve(_cnodes.size() + 16);
    _nodes.insert(_nodes.end(), _cnodes.begin(), _cnodes.end());
    _cnodes.clear();
    _queryResult.reserve(64);
#ifdef _DEBUG
    if (!_points.empty())
        validate();
#endif
}

// Returns index of the root node.
unsigned KDTree::build()
{
	if (!_points.empty()) {
		_cnodes.reserve(65536);
		return build(0, (unsigned)_points.size());
	}
	return -1;
}

// Returns index of the newly built node.
unsigned KDTree::build(unsigned begin, unsigned end)
{
	// Allocate the node first.
	auto nodei = _cnodes.push_back(Node{});

	if (end - begin <= _leafSize) {
make_leaf:
		// Leaves are sorted by rank and we don't care about median of leaf nodes.
		std::sort(P(begin), P(end), Point::rank_lt{});
		*nodei = Node::makeLeaf(_points[begin].rank, begin, end);
	}
	else {
		// Physically partition the set. In internal nodes, the RIGHT node holds values >= median.
		Split split = selectDim(begin, end);
		int dim = split.dim;
		const float median = split.median;
		Point *pp = std::partition(P(begin), P(end), Point::median_p{ dim, median });
        unsigned mid = I(pp);

        // Partitioning failed; avoid infinite recursion and make a leaf instead.
        if (mid == begin || mid == end)
            goto make_leaf;

        unsigned left, right;
		concurrency::parallel_invoke(
			[=, &left]{ left = build(begin, mid); },
			[=, &right]{ right = build(mid, end); }
		);

		unsigned rank = std::min(_cnodes[left].rank, _cnodes[right].rank);
		*nodei = Node::makeBranch(dim, rank, left, right, median);
	}

	return (unsigned)(nodei - _cnodes.begin());
}

// Prioritize splitting along the longest axis as long as the median is relatively near the center.
KDTree::Split KDTree::selectDim(unsigned begin, unsigned end)
{
	const unsigned skewTolerance = (end - begin) / 16;

	Split splits[2] = {
		findSpread(0, begin, end),
		findSpread(1, begin, end)
	};

	if (splits[0].skew < skewTolerance && splits[0].range > splits[1].range)
		return splits[0];
	if (splits[1].skew < skewTolerance && splits[1].range > splits[0].range)
		return splits[1];
	if (splits[0].skew < splits[1].skew)
		return splits[0];
	return splits[1];
}

// 3-way split (range [m1,m2] is equal to the median)
KDTree::Split KDTree::findSpread(int dim, unsigned begin, unsigned end)
{
	//std::sort(P(begin), P(end), Point::dim_lt{ dim });
	concurrency::parallel_sort(P(begin), P(end), Point::dim_lt{ dim }, 4096);

	// Find leftmost occurrence of median.
	const unsigned mid = begin + (end - begin) / 2;
	const float median = _points[mid][dim];
	const float range = _points[end - 1][dim] - _points[begin][dim];

	unsigned right = mid;
	while (right > begin && _points[right - 1][dim] == median)
		--right;

	_ASSERTE(_points[right][dim] == median);
	_ASSERTE(right == begin || _points[right - 1][dim] < median);
	_ASSERTE(range >= 0);
	return Split{ dim, range, median, mid - right };
}

std::vector<Point> KDTree::overlap(const Rect &query)
{
	// Upper overlap bound is exclusive, so we need to get the extreme points at 1e10
	const Rect rootRange{ -1e10f, -1e10f, 1e10f, 1e10f };
	_query = &query;

	concurrency::combinable<PriorityQueue> cpq;
	collectOverlaps(_root, rootRange, cpq);

	PriorityQueue pq;
	cpq.combine_each([&pq](PriorityQueue &lpq) {
		const unsigned sz = lpq.size();
		for (unsigned i = 0; i < sz; ++i)
			pq.push(lpq.pop());
	});

	const unsigned sz = pq.size();
	for (unsigned i = 0; i < sz; ++i)
	{
		unsigned pointi = pq.pop().value;
		_queryResult.push_back(_points[pointi]);
	}

    return std::move(_queryResult);
}

void KDTree::collectOverlaps(const unsigned nodei, Rect nodeRange, concurrency::combinable<PriorityQueue> &cpq)
{
	PriorityQueue &pq = cpq.local();
	const Node &node = _nodes[nodei];

	if (!_query->overlaps(nodeRange) || !pq.hasPlace(node.rank))
		return;

	if (!node.branch) {
		for (unsigned pointi = node.left; pointi < node.right; ++pointi) {
			const Point &point = _points[pointi];
			if (!pq.hasPlace(point.rank))	// Ascending ranks; no point after this is good
				break;
			if (_query->overlaps(point[0], point[1]))
				pq.push(PqItem{ point.rank, pointi });
		}
	}
	else {
		concurrency::parallel_invoke(
			[=, &cpq]{ collectOverlaps(node.left, nodeRange.splitLeft(node.dim, node.median), cpq); },
			[=, &cpq]{ collectOverlaps(node.right, nodeRange.splitRight(node.dim, node.median), cpq); }
		);
    }
}

// Every point must be covered by some range.
void KDTree::validate()
{
	const Rect rootRange{ -1e10f, -1e10f, 1e10f, 1e10f };
	std::vector<unsigned> leaves;
	std::vector<Rect> bbs;
	leaves.reserve(_nodes.size());
	bbs.reserve(_nodes.size());
	collectLeafNodes(_root, rootRange, leaves, bbs);

	{
		const auto &nodes = _nodes;
		std::sort(leaves.begin(), leaves.end(),
			[&nodes](unsigned n1, unsigned n2) {
			return nodes[n1].left < nodes[n2].left;
		});
	}

	validateIndexSpans(leaves);
	validateBBSpans(bbs);
}

// Ranges spanned by leaves must be contiguous after sorting, i.e., each point must be covered by a range exactly once.
void KDTree::validateIndexSpans(const std::vector<unsigned> &leaves)
{
	if (_nodes[leaves.front()].left != 0 || _nodes[leaves.back()].right != _points.size())
		_ASSERTE(0), abort();

	const unsigned leafCount = (unsigned)leaves.size();
	for (unsigned i = 1; i < leafCount; ++i) {
		const unsigned prev = leaves[i - 1], cur = leaves[i];
		_ASSERTE(_nodes[prev].right == _nodes[cur].left);
	}
}

// No two leaf BBs must overlap.
void KDTree::validateBBSpans(std::vector<Rect> &bbs)
{
	const unsigned leafCount = (unsigned)bbs.size();
	for (unsigned i = 0; i < leafCount - 1; ++i)
		for (unsigned j = i + 1; j < leafCount; ++j)
			_ASSERTE(!bbs[i].overlapsExclusive(bbs[j]));
}

void KDTree::collectLeafNodes(const unsigned nodei, const Rect &rect, std::vector<unsigned> &leaves, std::vector<Rect> &bbs)
{
	// links must consistently point to either leaf or internal node
	const Node &node = _nodes[nodei];

	if (node.branch) {
		validateBranch(node);
		collectLeafNodes(node.left, rect.splitLeft(node.dim, node.median), leaves, bbs);
		collectLeafNodes(node.right, rect.splitRight(node.dim, node.median), leaves, bbs);
	}
	else {
		validateLeaf(node, rect);
		leaves.push_back(nodei);
		bbs.push_back(rect);
	}
}

void KDTree::validateBranch(const Node &node)
{
	_ASSERTE(node.branch);
	_ASSERTE(node.left < _nodes.size() && node.right < _nodes.size());
	_ASSERTE(node.rank == std::min(_nodes[node.left].rank, _nodes[node.right].rank));
}

void KDTree::validateLeaf(const Node &node, const Rect &rect)
{
	_ASSERTE(!node.branch);
	_ASSERTE(node.left < _points.size() && node.right <= _points.size());
	_ASSERTE(node.left < node.right && (node.right - node.left <= _leafSize));
	_ASSERTE(std::is_sorted(P(node.left), P(node.right), Point::rank_lt{}));
	_ASSERTE(P(node.left)->rank == node.rank);

	for (unsigned pointi = node.left; pointi < node.right; ++pointi) {
		const Point &point = _points[pointi];
		_ASSERTE(rect.overlaps(point[0], point[1]));
		// if (point.rank == 885) __debugbreak();
	}
}
