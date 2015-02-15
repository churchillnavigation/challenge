// Churchill Navigation programming challenge
// Sascha Kratky, skratky@gmail.com
//
// requires Visual Studio C++ 2013
// code licensed under MIT, see license.txt

#include "point_search.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <future>
#include <iterator>
#include <limits>
#include <numeric>
#include <thread>
#include <vector>

struct SearchContext {
	uint32_t             magic;
	// point data in a data-oriented layout
	std::vector<int32_t> rank;
	std::vector<float>   x;
	std::vector<float>   y;
	std::vector<int8_t>  id;
	// skip lists for each point
	std::vector<uint8_t> x_next_smaller;
	std::vector<uint8_t> x_next_larger;
	std::vector<uint8_t> y_next_smaller;
	std::vector<uint8_t> y_next_larger;
	// container for temporary results
	std::vector<int32_t> results;
	// worker threads
	int32_t              worker_count;
	std::atomic<int32_t> count;
	std::deque<std::future<std::array<std::vector<int32_t>::iterator,2>>> workers;
};

namespace {

template<typename Iterator, typename OutputIterator, typename Compare>
std::array<std::vector<int32_t>::iterator,2> compute_skip(Iterator begin, Iterator end, OutputIterator skip, Compare compare) {
	OutputIterator skip_last = skip;
	for (Iterator iter = begin; iter != end; ++iter) {
		auto x = *iter;
		std::size_t dist = 0;
		typedef typename std::iterator_traits<OutputIterator>::value_type skip_value_type;
		for (Iterator next = iter; ++dist <= std::numeric_limits<skip_value_type>::max() && ++next != end && compare(x, *next); ) {
		}
		*skip_last++ = static_cast<skip_value_type>(dist - 1);
	}
	return { std::vector<int32_t>::iterator(), std::vector<int32_t>::iterator() };
}

}

extern "C" __declspec(dllexport) SearchContext* __stdcall create(const Point* points_begin, const Point* points_end)
{
	int32_t size = static_cast<int32_t>(std::distance(points_begin, points_end));
	SearchContext* sc = new SearchContext;
	sc->magic = 0xFEEDFACE;
	sc->worker_count = std::max(static_cast<int32_t>(std::thread::hardware_concurrency()) - 1, 1);
	sc->count = 0;
	if (size <= 0) return sc;
	sc->rank.resize(size);
	sc->x.resize(size);
	sc->y.resize(size);
	sc->id.resize(size);
	sc->results.reserve(256);
	// save point list to search context sorted by rank
	std::iota(std::begin(sc->rank), std::end(sc->rank), 0);
	std::stable_sort(std::begin(sc->rank), std::end(sc->rank), [=](int32_t idx1, int32_t idx2) {
		return points_begin[idx1].rank < points_begin[idx2].rank;
	});
	for (int32_t idx = 0; idx < size; ++idx) {
		const Point* pt = std::next(points_begin, sc->rank[idx]);
		sc->rank[idx] = pt->rank;
		sc->x[idx]    = pt->x;
		sc->y[idx]    = pt->y;
		sc->id[idx]   = pt->id;
	}
	// compute skip lists for each point
	// computations are independent, warm up our worker list in the process
	sc->x_next_larger.resize(size);
	sc->workers.push_back(
		std::async(std::launch::async,
			compute_skip<decltype(std::begin(sc->x)),decltype(std::begin(sc->x_next_larger)),std::greater_equal<float>>,
			std::begin(sc->x), std::end(sc->x), std::begin(sc->x_next_larger), std::greater_equal<float>()));
	sc->x_next_smaller.resize(size);
	sc->workers.push_back(
		std::async(std::launch::async,
			compute_skip<decltype(std::begin(sc->x)),decltype(std::begin(sc->x_next_smaller)),std::less_equal<float>>,
			std::begin(sc->x), std::end(sc->x), std::begin(sc->x_next_smaller), std::less_equal<float>()));
	sc->y_next_larger.resize(size);
	sc->workers.push_back(
		std::async(std::launch::async,
			compute_skip<decltype(std::begin(sc->y)),decltype(std::begin(sc->y_next_larger)),std::greater_equal<float>>,
			std::begin(sc->y), std::end(sc->y), std::begin(sc->y_next_larger), std::greater_equal<float>()));
	sc->y_next_smaller.resize(size);
	sc->workers.push_back(
		std::async(std::launch::async,
			compute_skip<decltype(std::begin(sc->y)),decltype(std::begin(sc->y_next_smaller)),std::less_equal<float>>,
			std::begin(sc->y), std::end(sc->y), std::begin(sc->y_next_smaller), std::less_equal<float>()));
	while (!sc->workers.empty()) {
		sc->workers.front().wait();
		sc->workers.pop_front();
	}
	return sc;
}

namespace {

template<typename Iterator>
Iterator search_y_first(SearchContext* sc, const Rect& rect, int32_t start_idx, int32_t end_idx, Iterator out_begin)
{
	int32_t count = 0;
	for (int32_t idx = start_idx; count < sc->count && idx < end_idx; ++idx) {
		float y = sc->y[idx];
		if (y < rect.ly) {
			idx += sc->y_next_larger[idx];
			continue;
		}
		if (y > rect.hy) {
			idx += sc->y_next_smaller[idx];
			continue;
		}
		float x = sc->x[idx];
		if (x < rect.lx) {
			idx += sc->x_next_larger[idx];
			continue;
		}
		if (x > rect.hx) {
			idx += sc->x_next_smaller[idx];
			continue;
		}
		++count;
		*out_begin++ = idx;
	}
	return out_begin;
}

template<typename Iterator>
Iterator search_x_first(SearchContext* sc, const Rect& rect, int32_t start_idx, int32_t end_idx, Iterator out_begin)
{
	int32_t count = 0;
	for (int32_t idx = start_idx; count < sc->count && idx < end_idx; ++idx) {
		float x = sc->x[idx];
		if (x < rect.lx) {
			idx += sc->x_next_larger[idx];
			continue;
		}
		if (x > rect.hx) {
			idx += sc->x_next_smaller[idx];
			continue;
		}
		float y = sc->y[idx];
		if (y < rect.ly) {
			idx += sc->y_next_larger[idx];
			continue;
		}
		if (y > rect.hy) {
			idx += sc->y_next_smaller[idx];
			continue;
		}
		++count;
		*out_begin++ = idx;
	}
	return out_begin;
}

template<typename Iterator>
std::array<Iterator,2> search_rect(SearchContext* sc, const Rect& rect, int32_t start_idx, int32_t end_idx, Iterator out_begin)
{
	assert(0 <= start_idx && start_idx <= static_cast<int32_t>(sc->rank.size()));
	assert(0 <= end_idx && end_idx <= static_cast<int32_t>(sc->rank.size()));
	// heuristic: process point coordinates corresponding to narrower side of rectangle first
	if (rect.hy - rect.ly < rect.hx - rect.lx) {
		return { out_begin, search_y_first(sc, rect, start_idx, end_idx, out_begin) };
	}
	else {
		return { out_begin, search_x_first(sc, rect, start_idx, end_idx, out_begin) };
	}
}

template<typename ForwardIterator, typename OutputIterator>
OutputIterator copy_to_output(SearchContext* sc, ForwardIterator begin, ForwardIterator end, OutputIterator out_begin, OutputIterator out_end)
{
	OutputIterator out_last = out_begin;
	for (auto iter = begin; iter != end && out_last != out_end; ++out_last, ++iter) {
		int32_t idx    = *iter;
		assert(0 <= idx && idx < sc->rank.size());
		out_last->id   = sc->id[idx];
		out_last->rank = sc->rank[idx];
		out_last->x    = sc->x[idx];
		out_last->y    = sc->y[idx];
		assert(out_last == out_begin || out_last->rank >= std::prev(out_last, 1)->rank);
	}
	return out_last;
}

}

extern "C" __declspec(dllexport) int32_t __stdcall search(SearchContext* sc, const Rect rect, const int32_t count, Point* out_points)
{
	if (count <= 0) return 0;
	if (sc == nullptr || sc->magic != 0xFEEDFACE) return 0;
	int32_t len = static_cast<int32_t>(sc->rank.size());
	if (len == 0) return 0;
	// number of outstanding points to search
	sc->count = count;
	// set up worker thread for each segment of point list
	// each worker thread writes output to a portion of the results list
	// align each segment on a 64 byte boundary to prevent false sharing
	int32_t segment_align = 16;
	int32_t result_segment_len = ((count + segment_align - 1) / segment_align) * segment_align;
	sc->results.resize(result_segment_len * sc->worker_count);
	int32_t segment_len = (len + sc->worker_count - 1) / sc->worker_count;
	segment_len = ((segment_len + segment_align - 1) / segment_align) * segment_align;
	auto out_last = out_points;
	auto out_end = std::next(out_points, count);
	for (int32_t worker_idx = 0; sc->count > 0 && worker_idx < sc->worker_count; ++worker_idx) {
		int32_t start_idx = segment_len * worker_idx;
		int32_t end_idx   = std::min(start_idx + segment_len, len);
		auto result_begin = std::next(std::begin(sc->results), result_segment_len * worker_idx);
		sc->workers.push_back(
			std::async(std::launch::async,
				search_rect<decltype(std::begin(sc->results))>, sc, rect, start_idx, end_idx, result_begin));
		// collect results from oldest worker thread
		auto& worker = sc->workers.front();
		if (worker.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
			auto iter_pair = worker.get();
			assert(std::distance(iter_pair[0], iter_pair[1]) <= count);
			// decrease outstanding points to reduce load for remaining workers
			sc->count -= static_cast<int32_t>(std::distance(iter_pair[0], iter_pair[1]));
			out_last = copy_to_output(sc, iter_pair[0], iter_pair[1], out_last, out_end);
			sc->workers.pop_front();
		}
	}
	// collect results from active worker threads
	while (!sc->workers.empty()) {
		auto iter_pair = sc->workers.front().get();
		assert(std::distance(iter_pair[0], iter_pair[1]) <= count);
		// decrease outstanding points to reduce load for remaining workers
		sc->count -= static_cast<int32_t>(std::distance(iter_pair[0], iter_pair[1]));
		out_last = copy_to_output(sc, iter_pair[0], iter_pair[1], out_last, out_end);
		sc->workers.pop_front();
	}
	return static_cast<int32_t>(std::distance(out_points, out_last));
}

extern "C" __declspec(dllexport) SearchContext* __stdcall destroy(SearchContext* sc)
{
	if (sc != nullptr && sc->magic == 0xFEEDFACE) {
		delete sc;
		return nullptr;
	}
	return sc;
}
