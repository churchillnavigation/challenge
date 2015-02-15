#include <type_traits>
#include <cassert>
#include <algorithm>
#include <memory>
#include <vector>
#include <set>
#include <queue>
#include "../point_search.h"

struct AugmentedPoint {
	int32_t bestRankLower, bestRankUpper;
	Point point;
	AugmentedPoint& operator=(const Point& p) { point = p; return *this; }
};

struct SearchContext {
	enum Axis : unsigned char { X_AXIS, Y_AXIS };
	typedef const AugmentedPoint CAP;
	static bool pointCompare(const Point& a, const Point& b) {
		return a.rank < b.rank; // worst rank on top of heap
	}
	SearchContext(const Point *begin, const Point *end) {
		treeSize = end - begin;;
		tree = new AugmentedPoint[treeSize];
		std::copy(begin, end, tree);
		buildTree(tree, treeSize, X_AXIS);
	}
	~SearchContext() {
		delete[] tree;
	}
	int32_t buildTree(AugmentedPoint *start, size_t count, Axis axis) {
		if (count == 0)
			return std::numeric_limits<int32_t>::max();
		// place pivot
		size_t mid = count / 2;
		AugmentedPoint *pivot = start + mid;
		auto comp = (axis == X_AXIS) ?
			[](CAP& a, CAP& b) { return a.point.x < b.point.x; } :
			[](CAP& a, CAP& b) { return a.point.y < b.point.y; };
		std::nth_element(start, pivot, start + count, comp);
		// build subtrees
		Axis otherAxis = (axis == X_AXIS) ? Y_AXIS : X_AXIS;
		int32_t hrLower = buildTree(start, mid, otherAxis);
		int32_t hrUpper = buildTree(start + mid + 1, count - mid - 1, otherAxis);
		// record and return smallest rank seen
		pivot->bestRankLower = hrLower;
		pivot->bestRankUpper = hrUpper;
		return std::min<int32_t>({pivot->point.rank, hrLower, hrUpper});
	}
	int doSearch(const Rect& rect, Point* out, size_t capacity) {
		size_t outcnt = 0;
		iterativeSearch(tree, treeSize, X_AXIS,
			rect, out, outcnt, capacity);
		std::sort_heap(out, out + outcnt, pointCompare);
		return outcnt;
	}
	void iterativeSearch(AugmentedPoint *start0, size_t count0, Axis axis0,
			const Rect& rect, Point *out, size_t& outcnt, size_t outcap) {
		if (count0 == 0 || outcap == 0)
			return;
		// create heap of candidates
		struct Candidate {
			CAP *start;
			size_t count;
			int32_t best;
			Axis axis;
		};
		struct CandidateCompare {
			bool operator()(const Candidate& a, const Candidate& b) const {
				return a.best > b.best; // best rank on top of heap
			};
		};
		std::priority_queue<Candidate,
			std::vector<Candidate>,
			CandidateCompare> candidates;
		candidates.push(Candidate{start0, count0,
				std::numeric_limits<int32_t>::max(), axis0});
		auto shouldAdd = [&](int32_t rank){
			return outcnt != outcap || rank < out->rank;
		};
		while (candidates.size()) {
			Candidate cand = candidates.top();
			candidates.pop();
			// bail if no improvement possible
			if (!shouldAdd(cand.best))
				break;
			// find and add pivot
			size_t mid = cand.count / 2;
			CAP& pivot = cand.start[mid];
			const Point& ppt = pivot.point;
			if (shouldAdd(ppt.rank) &&
					ppt.x >= rect.lx &&
					ppt.x <= rect.hx &&
					ppt.y >= rect.ly &&
					ppt.y <= rect.hy) {
				if (outcnt == outcap) {
					std::pop_heap(out, out + outcnt, pointCompare);
					out[outcap - 1] = ppt;
				} else {
					out[outcnt++] = ppt;
				}
				std::push_heap(out, out + outcnt, pointCompare);
			}
			// add lower and upper
			Axis otherAxis = cand.axis == X_AXIS ? Y_AXIS : X_AXIS;
			bool lowerValid = shouldAdd(pivot.bestRankLower) &&
				(cand.axis == X_AXIS ? rect.lx <= ppt.x : rect.ly <= ppt.y);
			if (lowerValid && mid > 0)
				candidates.push(Candidate{cand.start, mid,
						pivot.bestRankLower, otherAxis});
			bool upperValid = shouldAdd(pivot.bestRankUpper) &&
				(cand.axis == X_AXIS ? rect.hx >= ppt.x : rect.hy >= ppt.y);
			if (upperValid && cand.count - mid - 1 > 0)
				candidates.push(Candidate{cand.start + mid + 1, cand.count - mid - 1,
						pivot.bestRankUpper, otherAxis});
		}
	}
	AugmentedPoint *tree;
	size_t treeSize;
};

extern "C" {

#define EXPORT  __attribute__((stdcall,dllexport))

EXPORT
SearchContext *create(const Point *begin, const Point *end) {
	return new SearchContext(begin, end);
}

EXPORT
int32_t search(SearchContext *ctx, const Rect rect, const int32_t outsiz, Point *out) {
	return ctx->doSearch(rect, out, outsiz);
}

EXPORT
SearchContext *destroy(SearchContext *ctx) {
	delete ctx;
	return nullptr;
}

};

static_assert(std::is_convertible<decltype(create), T_create>::value,
		"create function wrong type");
static_assert(std::is_convertible<decltype(search), T_search>::value,
		"search function wrong type");
static_assert(std::is_convertible<decltype(destroy), T_destroy>::value,
		"destroy function wrong type");
