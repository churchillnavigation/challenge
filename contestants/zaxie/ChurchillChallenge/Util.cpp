#include "Accel.h"
#include <random>

// FUNCTIONS ////////////////////////////////////////////////////////////////

PointV CopyInput(const Input::Point *begin, const Input::Point *end)
{
	PointV points;

	if (begin != end) {
		points.reserve(end - begin);
		while (begin != end)
			points.push_back(Point(*begin++));
	}

	return points;
}

void CopyOutput(std::vector<Point> &points, const int count, Input::Point *out)
{
	if (points.size() > count)
		points.erase(points.begin() + count, points.end());
	
	std::transform(points.begin(), points.end(), out,
		[](const Point &p) { return p.asInputPoint(); });
}

std::vector<Point> SlowSearch(const PointV &points, const Rect &rect)
{
	PriorityQueue_AVX2 pq;

	{
		const unsigned n = (unsigned)points.size();
		for (unsigned i = 0; i < n; ++i)
			if (rect.overlaps(points[i][0], points[i][1]))
				pq.push(PqItem{ points[i].rank, i });
	}

	{
		std::vector<Point> result;
		result.reserve(20);

		const unsigned n = pq.size();
		for (unsigned i = 0; i < n; ++i) {
			unsigned p = pq.pop().value;
			result.push_back(points[p]);
		}
		return result;
	}
}

std::vector<Point> SnailSearch(const PointV &points, const Rect &rect)
{
	std::vector<Point> result;
	result.reserve(points.size() / 8);

	{
		const unsigned n = (unsigned)points.size();
		for (unsigned i = 0; i < n; ++i)
			if (rect.overlaps(points[i][0], points[i][1]))
				result.push_back(points[i]);
	}

	std::sort(result.begin(), result.end(), Point::rank_lt{});
	return result;
}

// PQ TEST & BENCHMARK //////////////////////////////////////////////////////

static const unsigned TEST_SIZE = 1 << 23;
static void PqTestRandom();
static void PqTestAscending();
static void PqTestDescending();
static void PqCheckContents(PriorityQueue &pq);
static unsigned PqGetValueForRank(unsigned rank);

void PqTest()
{
	PqTestRandom();
	PqTestAscending();
	PqTestDescending();
}

void PqTestRandom()
{
	std::vector<unsigned> keys;
	keys.reserve(TEST_SIZE);
	for (unsigned i = 0; i < TEST_SIZE; ++i)
		keys.push_back(i);
	
	std::shuffle(keys.begin(), keys.end(), std::default_random_engine());

	PriorityQueue pq;

	for (unsigned rank : keys) {
		unsigned value = PqGetValueForRank(rank);
		pq.push(PqItem{ rank, value });
	}

	PqCheckContents(pq);
}

void PqTestAscending()
{
	PriorityQueue pq;

	for (unsigned i = 0; i < TEST_SIZE; ++i)
		pq.push(PqItem{ i, PqGetValueForRank(i) });

	PqCheckContents(pq);
}

void PqTestDescending()
{
	PriorityQueue pq;

	for (int i = TEST_SIZE; i >= 0; --i)
		pq.push(PqItem{ i, PqGetValueForRank(i) });

	PqCheckContents(pq);
}

// Also checks that there are no duplicates inserted, ever.
void PqCheckContents(PriorityQueue &pq)
{
	const unsigned sz = pq.size();

	// Try reinserting duplicates.
	{
		auto minItem = PqItem{ 0, PqGetValueForRank(0) };
		auto maxItem = PqItem{ 23, PqGetValueForRank(23) };
		auto midItem = PqItem{ 11, PqGetValueForRank(11) };

		for (int round = 0; round < 16; ++round)
		{
			pq.push(midItem);
			pq.push(maxItem);
			pq.push(minItem);
		}
	}

	// Check data.
	for (unsigned i = 0; i < sz; ++i) {
		PqItem item = pq.pop();
		if (item.rank != i || item.value != PqGetValueForRank(item.rank))
		{
			_ASSERTE(item.rank == i);
			_ASSERTE(item.value == PqGetValueForRank(item.rank));
			abort();
		}
	}
}

unsigned PqGetValueForRank(unsigned rank)
{
	return ~((rank + 271828) * 3141592);
}

