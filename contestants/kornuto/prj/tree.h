#pragma once

#include <algorithm>
#include <vector>
#include <queue>

#include "point_search.h"

#include "gheap.hpp"

const int NUM_QUERY_POINTS = 20;

const int BRANCH_FACTOR = 20;
const int LEAF_CAPACITY = 100;

struct Node
{
	Rect bounds;
	int32_t min_rank;

	int level;

	int count;
};

struct ChildData {
	int32_t rank;
	Rect bounds;
	int level;
};

struct BranchNode : Node {
	//Rect children_bounds[BRANCH_FACTOR];
	ChildData children_data[BRANCH_FACTOR];
	Node* children[BRANCH_FACTOR];

	uint8_t num_best_points;
	Point best_pts[NUM_QUERY_POINTS];
};

struct LeafNode : Node
{
	Point pts[LEAF_CAPACITY];
};

struct QueryNode
{
	int rank;
	Node* node;
	Point* pts;
	int count;

	QueryNode(Node* n, int r) : node(n), rank(r), count(-1) {}
	QueryNode(Point* p, int c, int r) : pts(p), count(c), rank(r) {}

	bool operator<(const QueryNode& b) const {
		return rank < b.rank;
	}
};

static std::vector<Point> buffer;

int32_t mergePoints(std::vector<Point>& r, int count_wanted, int worst_rank, Point* points, int num_points, const Rect* rect);

class PointTree {
public:
	PointTree() = default;

	void build(std::vector<Point>& pts);

	void queryPoints(const Rect& rect, int count, std::vector<Point>& r);
	void queryPoints2(const Rect& rect, int count, std::vector<Point>& r);
	void queryPoints3(const Rect& rect, int count, std::vector<Point>& r);
	void queryPoints4(const Rect& rect, int count, std::vector<Point>& r);

private:
	std::vector<Node*> createLeafNodes(std::vector<Point>& pts) {
		std::sort(pts.begin(), pts.end(), [](Point& i, Point& j) -> bool { return i.x < j.x; });

		int elems = pts.size();
		int elems_per_slice = std::max(1, (int)std::ceil(std::sqrt(elems * LEAF_CAPACITY)));

		int slice_offset = 0;

		std::vector<Node*> leafs;

		while (elems > 0) {
			int n_slice = std::min(elems_per_slice, elems);
			elems -= n_slice;

			std::sort(pts.begin() + slice_offset, pts.begin() + slice_offset + n_slice, [](Point& i, Point& j) -> bool { return i.y < j.y; });

			while (n_slice > 0) {
				int n_leaf = std::min(LEAF_CAPACITY, n_slice);

				LeafNode *leaf = new LeafNode();

				leaf->bounds.lx = std::numeric_limits<float>::max();
				leaf->bounds.hx = -std::numeric_limits<float>::max();

				int min_rank = std::numeric_limits<int>::max();
				int max_rank = std::numeric_limits<int>::min();

				for (int i = 0; i < n_leaf; i++) {
					auto pt = pts[slice_offset + i];

					if (pt.x < leaf->bounds.lx) {
						leaf->bounds.lx = pt.x;
					}

					if (pt.x > leaf->bounds.hx) {
						leaf->bounds.hx = pt.x;
					}

					min_rank = std::min(min_rank, pt.rank);
					max_rank = std::max(max_rank, pt.rank);
				}

				leaf->min_rank = min_rank;

				// Points are sorted by y at this point
				leaf->bounds.ly = pts[slice_offset].y;
				leaf->bounds.hy = pts[slice_offset + n_leaf - 1].y;

				//printf("Node %d - BBOX (%f %f) (%f %f)\n", wow, min_x, min_y, max_x, max_y);
				//printf("Leaf (%f %f) (%f %f) - %d to %d\n", leaf->bounds.lx, leaf->bounds.ly, leaf->bounds.hx, leaf->bounds.hy, minr, maxr);

				// Sort by rank
				std::sort(pts.begin() + slice_offset, pts.begin() + slice_offset + n_leaf, [](Point& i, Point& j) -> bool {return i.rank < j.rank; });

				leaf->level = 0;
				leaf->count = n_leaf;
				memcpy(leaf->pts, &pts[slice_offset], sizeof(Point) * n_leaf);

				leafs.push_back(leaf);

				slice_offset += n_leaf;
				n_slice -= n_leaf;
			}
		}

		return leafs;
	}

	std::vector<Point> getBestPoints(Node* node, int count = NUM_QUERY_POINTS) {
		std::vector<Point> r;
		std::vector<Node*> q;

		// only when r has count elements
		// that way we can skip nodes with min_rank > worst_rank
		int32_t worst_rank = std::numeric_limits<int32_t>::max();

		q.push_back(node);

		while (!q.empty()) {
			Node* node = q.back();
			q.pop_back();

			if (node->level > 0) {
				auto bn = static_cast<BranchNode*>(node);

				for (int i = 0; i < bn->count; i++) {
					auto child = bn->children[i];

					// Children are ordered by rank
					if (child->min_rank > worst_rank)
						break;

					q.push_back(child);
				}
			} else {
				auto leaf = static_cast<LeafNode*>(node);

				worst_rank = mergePoints(r, count, worst_rank, leaf->pts, leaf->count, nullptr);
			}
		}

		return r;
	}

	std::vector<Node*> sortTile(std::vector<Node*>& nodes, int level = 1) {
		auto mid_cmpx = [](Node* i, Node* j) -> bool {
			float mid_i = (i->bounds.hx + i->bounds.lx) / 2;
			float mid_j = (j->bounds.hx + j->bounds.lx) / 2;

			return mid_i < mid_j;
		};

		auto mid_cmpy = [](Node* i, Node* j) -> bool {
			float mid_i = (i->bounds.hy + i->bounds.ly) / 2;
			float mid_j = (j->bounds.hy + j->bounds.ly) / 2;

			return mid_i < mid_j;
		};

		if (nodes.size() > BRANCH_FACTOR) {
			std::sort(nodes.begin(), nodes.end(), mid_cmpx);

			int elems = nodes.size();
			int i = 0;

			std::vector<Node*> r;

			int elems_per_slice = std::max(1, (int)std::ceil(std::sqrt(elems * BRANCH_FACTOR)));

			while (elems > 0) {
				int n = std::min(elems_per_slice, elems);

				std::sort(nodes.begin() + i, nodes.begin() + i + n, mid_cmpy);

				while (n > 0) {
					int leaf_n = std::min(BRANCH_FACTOR, n);

					BranchNode *leaf = new BranchNode();

					leaf->bounds = nodes[i]->bounds;

					for (int jj = 1; jj < leaf_n; jj++) {
						auto bb = nodes[i + jj]->bounds;

						leaf->bounds.lx = std::min(leaf->bounds.lx, bb.lx);
						leaf->bounds.ly = std::min(leaf->bounds.ly, bb.ly);
						leaf->bounds.hx = std::max(leaf->bounds.hx, bb.hx);
						leaf->bounds.hy = std::max(leaf->bounds.hy, bb.hy);
					}

					//printf("Branch - (%f %f) (%f %f) - %d\n", leaf->bounds.lx, leaf->bounds.ly, leaf->bounds.hx, leaf->bounds.hy, elems);

					leaf->level = level + 1;
					leaf->count = leaf_n;

					std::sort(nodes.begin() + i, nodes.begin() + i + leaf_n, [](Node* i, Node* j) -> bool { return i->min_rank < j->min_rank; });

					leaf->min_rank = nodes[i]->min_rank;

					memcpy(leaf->children, &nodes[i], sizeof(Node*) * leaf_n);

					for (int jj = 0; jj < leaf_n; jj++) {
						auto& ch = leaf->children[jj];
						leaf->children_data[jj] = { ch->min_rank, ch->bounds, ch->level };
					}
						//leaf->children_bounds[jj] = leaf->children[jj]->bounds;

					auto bp = getBestPoints(leaf);

 					leaf->num_best_points = bp.size();
					memcpy(leaf->best_pts, &bp[0], bp.size() * sizeof(Point));

					r.push_back(leaf);

					i += leaf_n;
					n -= leaf_n;
					elems -= leaf_n;
				}
			}

			//printf("Level %d - %d nodes\n", level, r.size());

			return r;
		}

		return nodes;
	}

	std::vector<Node*> root;
	std::vector<Node*> q;
	std::priority_queue<QueryNode> q2;
};