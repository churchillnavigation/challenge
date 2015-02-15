#include "point_search.h"
#include <deque>
#include <vector>
#include <stdint.h>

using namespace std;

//#define __ERROR_CHECK__
//#define __ANALYTICS__
//#define __OUTPUT_POINTS__
#define __PREPCACHE__
#define QUERY_COUNT 20

// Define how deep our NodeContainers start at and end at.
#define MIN_DEPTH 2
#define MAX_DEPTH 9

#define CAP_DEPTH 999 // NodeContainer depth to stop capping at.
#define CAP_SIZE 25 // How many points to add to each node when under the CAP_DEPTH. Must be larger than QUERY_COUNT.

#define XY_THRESHOLD 1024 // Threshold of number of points that determines if we search xyspace/yxspace instead of diving to nodes.
#define SOLVED_THRESHOLD 15 // How many solved indexes to have before we start multi-threading their solve.
#define PARTIAL_THRESHOLD 20 // How many partial indexes to have before we start multi-threading their solve.


// Forward declare;
struct NodeContainer;

class PointContainer
{
public:
    static const int POINTS_TO_FIND = QUERY_COUNT;

    int64_t depthOffsets[MAX_DEPTH + 1];

	PointContainer(const Point * points_begin, const Point * points_end);
	int32_t Evaluate(const Rect rect, const int32_t count, Point* out_points);

    void ResetAnalytics();
    void WriteAnalytics();
    void PrepCache();

	~PointContainer();
    
    HANDLE t2Handle;

    void StartWorkerThreads();
    void StopWorkerThreads();
};

struct NodePartition
{
    int topSize = 0;
    int minRank = INT_MAX;
    Rect rect;
    int * points;
    int PointsSize = 0;
    bool cappedPartition = false;

    // Adds a point to this partition. Assumes addition only occurs in sorted fashion.
    void AddPoints(vector<int> & inPoints, int _minRank = 0)
    {
        if (points)
        {
            delete[] points;
            points = NULL;
            PointsSize = 0;
        }
        
        PointsSize = inPoints.size();
        points = new int[PointsSize];
        
        copy(inPoints.begin(), inPoints.end(), points);

        topSize = min(inPoints.size(), QUERY_COUNT);
        minRank = _minRank;
    }

    NodePartition()
    {
        topSize = 0;
        minRank = INT_MAX;
        points = NULL;
        PointsSize = 0;
        cappedPartition = false;
    }

    NodePartition (const NodePartition & other)
    {
        if (points)
        {
            delete[] points;
            points = NULL;
            PointsSize = 0;
        }

        topSize = other.topSize;
        minRank = other.minRank;
        rect = other.rect;
        PointsSize = other.PointsSize;
        cappedPartition = other.cappedPartition;
        if (PointsSize)
        {
            points = new int[PointsSize];
            copy(other.points, other.points + PointsSize, points);
        }
    }

    ~NodePartition()
    {
        if (points)
        {
            delete[] points;
            points = NULL;
            PointsSize = 0;
        }

    }
};

