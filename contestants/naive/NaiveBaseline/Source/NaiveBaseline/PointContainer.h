#include "point_search.h"
#include <deque>
#include <vector>
#include <stdint.h>

using namespace std;

//#define __ERROR_CHECK__
//#define __ANALYTICS__
//#define __OUTPUT_POINTS__
#define QUERY_COUNT 20

// Each of our node locations caches all points, to keep down on RAM, specify a min/max.
#define MIN_DEPTH 2
#define MAX_DEPTH 10

// Forward declare;
struct NodeContainer;

class PointContainer
{
public:
	static const int MAX_ALLOCATION = 512 * 1024 * 1024;
    static const int POINTS_TO_FIND = QUERY_COUNT;

	static const int DEPTH = 11;

	int64_t depthOffsets[DEPTH + 1];

	PointContainer(const Point * points_begin, const Point * points_end);
	int32_t Evaluate(const Rect rect, const int32_t count, Point* out_points);

    // Buffer for the outpoints.
    vector <Point *> outPoints;

    void ResetAnalytics();
    void WriteAnalytics();

	~PointContainer();
};

struct NodePartition
{
    static const int MAX_POINTS = PointContainer::POINTS_TO_FIND;
    int topSize = 0;
    int minRank;
    vector <uint32_t> points; // Used to be vector <Point *>, but made it 32-bit index to save on space.
    Rect rect;
    // Adds a point to this partition. Assumes addition only occurs in sorted fashion.
    void AddPoint(Point * point, uint32_t index)
    {
        points.push_back(index);

        if (topSize == 0)
        {
            minRank = point->rank;
        }

        if (topSize < MAX_POINTS)
        {
            topSize++;
        }
    }
};

