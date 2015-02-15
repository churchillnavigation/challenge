#include "tree.h"

int32_t mergePoints(std::vector<Point>& r, int count_wanted, int worst_rank, Point* points, int num_points, const Rect* rect)
{
	auto it = r.begin();
	//int j = 0;

	//printf("MERGING %d points - min %d\n", num_points, worst_rank);

	for (int i = 0; i < num_points; i++) {
		auto pt = points[i];

		if (pt.rank > worst_rank) {
			break;
		}

		if (rect != nullptr && (pt.x < rect->lx || pt.x > rect->hx || pt.y < rect->ly || pt.y > rect->hy)) {
			continue;
		}

		//it = std::lower_bound(it, r.end(), pt, [](const Point& i, const Point& j) -> bool { return i.rank < j.rank; });
		while (it != r.end() && it->rank < pt.rank) {
			it++;
		}

		it = r.insert(it, pt);
		//r.push_back(pt);

		if (r.size() > count_wanted) {
			r.pop_back();
			worst_rank = r.back().rank;
		}
	}

	//std::partial_sort(it, r.begin() + count_wanted, r.end());

	//if (r.size() >= count_wanted) {
		// With memmove above, the vector should never be bigger than count_wanted
	//	r.resize(count_wanted);

	//	worst_rank = r.back().rank;
	//}

	return worst_rank;
}

void PointTree::build(std::vector<Point>& pts) {
	int level = 0;

	root = createLeafNodes(pts);

	while (root.size() > BRANCH_FACTOR) {
		root = sortTile(root, level++);
	}

	//printf("%d %d %d\n", sizeof(Point), sizeof(BranchNode), sizeof(LeafNode));
}

void PointTree::queryPoints(const Rect& rect, int count, std::vector<Point>& r) {
	r.clear();

	// only when r has 'count' elements
	// that way we can skip nodes with min_rank > worst_rank
	int32_t worst_rank = std::numeric_limits<int32_t>::max();

	for (Node* node : root) {
		if (Rect::Inside(node->bounds, rect)) {
			auto bn = static_cast<BranchNode*>(node);

			worst_rank = mergePoints(r, count, worst_rank, bn->best_pts, bn->num_best_points, nullptr);
		} else if (Rect::Intersects(node->bounds, rect)) {
			q.push_back(node);
		}
	}

	while (!q.empty()) {
		Node* node = q.back();
		q.pop_back();

		if (node->level > 0) {
			auto bn = static_cast<BranchNode*>(node);
			//printf("BranchLeaf (%.3f %.3f) (%.3f %.3f) - %d %d %d - %d\n", bn->bounds.lx, bn->bounds.ly, bn->bounds.hx, bn->bounds.hy, q.size(), bn->level, bn->count, worst_rank);
			//printf("Intersect with (%f %f) (%f %f)\n", rect.lx, rect.ly, rect.hx, rect.hy);

			int q_end = q.size();

			for (int i = 0; i < bn->count; i++) {
				// Children are ordered by rank
				if (bn->children_data[i].rank > worst_rank) {
					break;
				}

				if (!Rect::Intersects(bn->children_data[i].bounds, rect)) {
					continue;
				}

				auto child = bn->children[i];
				bool competely_inside = Rect::Inside(bn->children_data[i].bounds, rect);

				if (bn->children_data[i].level == 0) {
					auto leaf = static_cast<LeafNode*>(child);

					//_mm_prefetch((const char *) leaf->pts, _MM_HINT_T0);

					worst_rank = mergePoints(r, count, worst_rank, leaf->pts, leaf->count, competely_inside ? nullptr : &rect);
				}
				else if (competely_inside) {
					auto bn = static_cast<BranchNode*>(child);

					worst_rank = mergePoints(r, count, worst_rank, bn->best_pts, bn->num_best_points, nullptr);
				} else {
					q.push_back(child);
				}
			}

			// Few ms fater since children as pushed on backwards (largest rank to lowest)
			std::reverse(q.begin() + q_end, q.end());
		}
	}
}

void PointTree::queryPoints2(const Rect& rect, int count, std::vector<Point>& r)
{
	r.clear();

	//std::priority_queue<QueryNode> q;

	gheap<5, 1> Heap;
	std::vector<QueryNode> q;

	for (Node* node : root) {
		if (Rect::Inside(node->bounds, rect)) {
			//printf("root optimization doe\n");
			auto cbn = static_cast<BranchNode*>(node);
			//q.push(QueryNode(cbn->best_pts, cbn->num_best_points, cbn->min_rank));
			//q.make_heap
			q.push_back(QueryNode(cbn->best_pts, cbn->num_best_points, cbn->min_rank));
			//Heap.make_heap(q.begin(), q.end());

			continue;
		}

		if (Rect::Intersects(node->bounds, rect)) {
			//q.push(QueryNode(node, node->min_rank));
			q.push_back(QueryNode(node, node->min_rank));
			//Heap.make_heap(q.begin(), q.end());
		}
	}

	std::make_heap(q.begin(), q.end());

	//printf("We innit dog\n");

	while (q.size() > 0) {
		auto qn = q.front();

		//q.pop();
		Heap.pop_heap(q.begin(), q.end());
		//std::pop_heap(q.begin(), q.end()); 
		q.pop_back();

		//printf(" %d", q.size());

		if (qn.count == -1) {
			auto bn = static_cast<BranchNode*>(qn.node);

			//Heap.pop_heap(q.begin(), q.end());
			//q.pop_back();

			for (int i = 0; i < bn->count; i++) {
				auto child = bn->children[i];

				bool completely_inside = Rect::Inside(child->bounds, rect);

				if (completely_inside && child->level > 0) {
					auto cbn = static_cast<BranchNode*>(child);

					q.push_back(QueryNode(cbn->best_pts, cbn->num_best_points, cbn->min_rank));
					Heap.push_heap(q.begin(), q.end());
					//std::push_heap(q.begin(), q.end());
					continue;
				}

				if (!completely_inside && !Rect::Intersects(child->bounds, rect))
					continue;

				if (child->level == 0)
				{
					auto leaf = static_cast<LeafNode*>(child);
					q.push_back(QueryNode(leaf->pts, leaf->count, leaf->min_rank));
				} else {
					q.push_back(QueryNode(child, child->min_rank));
				}

				Heap.push_heap(q.begin(), q.end());
				//std::push_heap(q.begin(), q.end());
			}
			//Heap.make_heap(q.begin(), q.end());
			//std::make_heap(q.begin(), q.end());
			//printf("Brancherino\n");
		} else {
			int32_t worst_rank = std::numeric_limits<int32_t>::max();

			//Heap.restore_heap_after_item_increase(

			if (q.size() > 0)
				worst_rank = q.front().rank;

			//printf("Min ranks doe %d going until %d\n", qn.rank, worst_rank);

			for (int i = 0; i < qn.count; i++) {
				auto& p = qn.pts[i];

				if ((p.x < rect.lx || p.x > rect.hx || p.y < rect.ly || p.y > rect.hy)) {
					//printf("not in bounds :(\n");
					continue;
				}

				if (p.rank > worst_rank) {
					q.push_back(QueryNode(qn.pts + i, qn.count - i, p.rank));
					Heap.push_heap(q.begin(), q.end());
					//std::push_heap(q.begin(), q.end());
					break;
				}

				//printf("Add pt %d\n", p.rank);
				r.push_back(p);

				if (r.size() == count) {
					//printf("Left %d nodes\n", q.size());
					return;
				}
			}
		}
	}

	//printf("\n");
}

void PointTree::queryPoints3(const Rect& rect, int count, std::vector<Point>& r)
{
	r.clear();

	std::vector<QueryNode> q;

	for (Node* node : root) {
		if (Rect::Inside(node->bounds, rect)) {
			//printf("root optimization doe\n");
			auto cbn = static_cast<BranchNode*>(node);
			//q.push(QueryNode(cbn->best_pts, cbn->num_best_points, cbn->min_rank));
			//q.make_heap

			auto kef = QueryNode(cbn->best_pts, cbn->num_best_points, cbn->min_rank);
			auto it = std::lower_bound(q.begin(), q.end(), kef);
			q.insert(it, kef);
			//Heap.make_heap(q.begin(), q.end());

			continue;
		}

		if (Rect::Intersects(node->bounds, rect)) {
			//q.push(QueryNode(node, node->min_rank));
			auto kef = QueryNode(node, node->min_rank);
			auto it = std::lower_bound(q.begin(), q.end(), kef);
			q.insert(it, kef);
			//Heap.make_heap(q.begin(), q.end());
		}
	}

	std::make_heap(q.begin(), q.end());

	//printf("We innit dog\n");

	//auto it = q.begin();
	auto it = 0;
	//while (q.size() > 0) {
	while (it < q.size()) {
		auto qn = q[it++];

		//q.erase(q.begin());

		//q.pop();
		//Heap.pop_heap(q.begin(), q.end());
		//std::pop_heap(q.begin(), q.end()); 
		//q.pop_back();

		//printf(" %d", q.size());

		if (qn.count == -1) {
			auto bn = static_cast<BranchNode*>(qn.node);

			//Heap.pop_heap(q.begin(), q.end());
			//q.pop_back();

			auto iit = q.begin() + it;

			auto old = std::distance(iit, q.end());

			for (int i = 0; i < bn->count; i++) {
				auto child = bn->children[i];

				bool completely_inside = Rect::Inside(child->bounds, rect);

				if (completely_inside && child->level > 0) {
					auto cbn = static_cast<BranchNode*>(child);

					auto kef = QueryNode(cbn->best_pts, cbn->num_best_points, cbn->min_rank);

					//iit = std::lower_bound(iit, q.end(), kef);
					//iit = q.insert(iit, kef);
					//q.push_back(kef);
					q.emplace_back(cbn->best_pts, cbn->num_best_points, cbn->min_rank);
					//Heap.push_heap(q.begin(), q.end());
					//std::push_heap(q.begin(), q.end());
					continue;
				}

				if (!completely_inside && !Rect::Intersects(child->bounds, rect))
					continue;

				if (child->level == 0)
				{
					auto leaf = static_cast<LeafNode*>(child);
					//auto kef = QueryNode(leaf->pts, leaf->count, leaf->min_rank);
					//auto iit = std::lower_bound(q.begin() + it, q.end(), kef);
					//iit = std::lower_bound(iit, q.end(), kef);
					//iit = q.insert(iit, kef);
					//q.push_back(kef);
					q.emplace_back(leaf->pts, leaf->count, leaf->min_rank);
				}
				else {
					//auto kef = QueryNode(child, child->min_rank);
					//auto iit = std::lower_bound(q.begin() + it, q.end(), kef);
					//iit = std::lower_bound(iit, q.end(), kef);
					//iit = q.insert(iit, kef);
					//q.push_back(kef);
					q.emplace_back(child, child->min_rank);
				}

				//Heap.push_heap(q.begin(), q.end());
				//std::push_heap(q.begin(), q.end());
			}

			printf("added %d nodes\n", std::distance(q.begin() + it, q.end()) - old);

			//std::sort(q.begin() + it, q.end());
			//Heap.make_heap(q.begin(), q.end());
			//std::make_heap(q.begin(), q.end());
			//printf("Brancherino\n");
		}
		else {
			int32_t worst_rank = std::numeric_limits<int32_t>::max();

			//Heap.restore_heap_after_item_increase(

			if (q.size() > 0)
				worst_rank = q[it].rank;

			//printf("Min ranks doe %d going until %d\n", qn.rank, worst_rank);

			for (int i = 0; i < qn.count; i++) {
				auto& p = qn.pts[i];

				if ((p.x < rect.lx || p.x > rect.hx || p.y < rect.ly || p.y > rect.hy)) {
					//printf("not in bounds :(\n");
					continue;
				}

				if (p.rank > worst_rank) {
					auto kef = QueryNode(qn.pts + i, qn.count - i, p.rank);
					auto iit = std::lower_bound(q.begin() + it, q.end(), kef);

					//if (iit == q.begin() + it)
					//	printf("WOWOWOWO ");

					q.insert(iit, kef);

					//Heap.push_heap(q.begin(), q.end());
					//std::push_heap(q.begin(), q.end());
					break;
				}

				//printf("Add pt %d\n", p.rank);
				r.push_back(p);

				if (r.size() == count) {
					//printf("Left %d nodes\n", q.size());
					return;
				}
			}
		}
	}

	//printf("\n");
}

void PointTree::queryPoints4(const Rect& rect, int count, std::vector<Point>& r)
{
	r.clear();

	std::vector<QueryNode> q;

	for (Node* node : root) {
		if (Rect::Inside(node->bounds, rect)) {
			auto cbn = static_cast<BranchNode*>(node);

			q.emplace_back(cbn->best_pts, cbn->num_best_points, cbn->min_rank);
			continue;
		}

		if (Rect::Intersects(node->bounds, rect)) {
			q.emplace_back(node, node->min_rank);
		}
	}

	//printf("We innit dog\n");

	//auto it = q.begin();
	//auto it = 0;
	while (q.size() > 0) {
//	while (it < q.size()) {
		//auto qn = q[it++];
		auto qn = q.front();

		q.front() = std::move(q.back());
		q.pop_back();

		if (qn.count == -1) {
			auto bn = static_cast<BranchNode*>(qn.node);

			//Heap.pop_heap(q.begin(), q.end());
			//q.pop_back();

			for (int i = 0; i < bn->count; i++) {
				auto child = bn->children[i];

				bool completely_inside = Rect::Inside(child->bounds, rect);

				if (completely_inside && child->level > 0) {
					auto cbn = static_cast<BranchNode*>(child);
					q.emplace_back(cbn->best_pts, cbn->num_best_points, cbn->min_rank);
					continue;
				}

				if (!completely_inside && !Rect::Intersects(child->bounds, rect))
					continue;

				if (child->level == 0)
				{
					auto leaf = static_cast<LeafNode*>(child);

					q.emplace_back(leaf->pts, leaf->count, leaf->min_rank);
				} else {
					q.emplace_back(child, child->min_rank);
				}
			}
		} else {
			int32_t worst_rank = std::numeric_limits<int32_t>::max();

			if (q.size() > 0) {
				//worst_rank = q.front().rank;
			
			//	std::partial_sort(q.begin(), q.begin() + std::min(2ull, q.size()), q.end());
				std::nth_element(q.begin(), q.begin(), q.end());
				worst_rank = q.front().rank;
			}

			//printf("Min ranks doe %d going until %d\n", qn.rank, worst_rank);

			for (int i = 0; i < qn.count; i++) {
				auto& p = qn.pts[i];

				if ((p.x < rect.lx || p.x > rect.hx || p.y < rect.ly || p.y > rect.hy)) {
					//printf("not in bounds :(\n");
					continue;
				}

				if (p.rank > worst_rank) {
					//auto nn = QueryNode(qn.pts + i, qn.count - i, p.rank);
					//auto it = std::lower_bound(q.begin(), q.end(), nn);
					//q.insert(it, nn);
					q.emplace_back(qn.pts + i, qn.count - i, p.rank);

					break;
				}

				//printf("Add pt %d\n", p.rank);
				r.push_back(p);

				if (r.size() == count) {
					//printf("Left %d nodes\n", q.size());
					return;
				}
			}
		}

		//std::partial_sort(q.begin(), q.begin() + std::min(2ull, q.size()), q.end());
		std::nth_element(q.begin(), q.begin(), q.end());
	}

	//printf("\n");
}
