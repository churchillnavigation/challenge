#include "stdafx.h"
#include <iostream>
#include "PointContainer.h"

#include <vector>
#include <list>
#include <algorithm>

#include <iostream>
#include <fstream>
#include <list>
#include <math.h>
#include <time.h>
using namespace std;


float BASELINE_FACTOR;
vector <NodeContainer> nodeContainer;
vector <Point> PointCopy; // Copy of all the points.

// Doing this means we can't search large quantities in smaller regions.
// Saves us from doing push_back on vector.
int partial[1 << (MAX_DEPTH + 2)];
int partialSize;

int solved[1 << (MAX_DEPTH + 2)];
int solvedSize;

Point * zipBuffer[QUERY_COUNT];
Point * pointBuffer[QUERY_COUNT];
int pointBufferSize = 0;

// Returns whether rect1 intersects rect2
inline bool intersects(const Rect & rect1, const Rect & rect2)
{
    return (rect1.lx <= rect2.hx && rect1.hx >= rect2.lx && rect1.ly <= rect2.hy && rect1.hy >= rect2.ly);
}

// Determine if rect1 contains rect2.
inline bool containsRect(const Rect & rect1, const Rect & rect2)
{
    return rect1.lx <= rect2.lx && rect1.hx >= rect2.hx && rect1.ly <= rect2.ly && rect1.hy >= rect2.hy;
}

inline bool isPointInsideRect(const Rect & rect, float x, float y)
{
    return (x >= rect.lx && x <= rect.hx && y >= rect.ly && y <= rect.hy);
}

static inline bool pointRankComparison(const Point & a, const Point & b)
{
    return a.rank < b.rank;
}

static inline bool pointerPointRankComparison(const Point * a, const Point * b)
{
    return a->rank < b->rank;
}

// Must intersect or behavior is undefined.
inline Rect pruneRect(const Rect & rectToTrim, const Rect & source)
{
    return Rect
    {
        max(rectToTrim.lx, source.lx),
        max(rectToTrim.ly, source.ly),
        min(rectToTrim.hx, source.hx),
        min(rectToTrim.hy, source.hy),
    };
}

Rect mainRect;
vector<Point> top20;

// Analytics stuff
int deeperCount = 0;
int deeperSearch;
int lessThanQUERY_COUNT;
int totalDepth;
int evalCount;
int startingsolves;
vector<int> depthCounts;
int missCount;

///////////////////////////////////////////////


struct NodeContainer
{
    vector<NodePartition> partitions;
    Rect baselineRect;
    float width;
    float height;

    double xdiv;
    double ydiv;
    double xmult;
    double ymult;
    int xmax;
    int ymax;
    int depth;
    int powshift;

    bool create(Rect rect, int _depth)
    {
#ifdef __ANALYTICS__
        cout << endl << "Depth: " << _depth << " Rect: " << rect << endl;
#endif

        baselineRect = rect;
        depth = _depth;
        powshift = (int)pow(2, depth);
        xmax = powshift - 1;
        ymax = powshift - 1;
        // potential source of floating point error.
        xdiv = powshift / ((double)rect.hx - (double)rect.lx);
        ydiv = powshift / ((double)rect.hy - (double)rect.ly);
        xmult = 1.0 / xdiv;
        ymult = 1.0 / ydiv;
        partitions.resize(powshift * powshift);
        width = rect.hx - rect.lx;
        height = rect.hy - rect.ly;
        Rect tempRect;

        for (int x = 0; x < powshift; ++x)
        {
            tempRect.lx = x * xmult + (double)baselineRect.lx;
            tempRect.hx = (x + 1) * xmult + (double)baselineRect.lx;
            if (x == powshift - 1)
            {
                tempRect.hx = baselineRect.hx;
            }


            if (tempRect.lx == tempRect.hx && depth != 0)
            {
#ifdef __ANALYTICS__
                cout << " Too Small: " << depth << " " << tempRect.hx << " " << tempRect.lx << " xmult: " << xmult << endl;
#endif
            }

            for (int y = 0; y < powshift; ++y)
            {
                tempRect.ly = y * ymult + (double)baselineRect.ly;
                tempRect.hy = (y + 1) * ymult + (double)baselineRect.ly;
                if (y == powshift - 1)
                {
                    tempRect.hy = baselineRect.hy;
                }


                if (tempRect.ly == tempRect.hy && depth != 0)
                {
#ifdef __ANALYTICS__
                    cout << " Too Small: " << endl;
#endif
                }

                partitions[x * powshift + y].rect = tempRect;
            }
        }

#ifdef __ANALYTICS__
        printf("%f %f %d %d %d\n", xdiv, ydiv, xmax, ymax, depth);
#endif
        return true;
    }

    inline int getPartitionForIndexes(int x, int y)
    {
        return y + x * powshift;
    }

    // Assumes we already have intersection tested.
    inline void GetPartitions(const Rect & rect)
    {
#ifdef __ANALYTICS__
        if (!intersects(rect, baselineRect))
        {
            cout << "Missed Partition Rect: " << rect << " Baseline Rect: " << baselineRect << endl;
            return;
        }
#endif
        Rect pruned = pruneRect(rect, baselineRect);
        int ax, ay, bx, by;
        partialSize = 0;
        solvedSize = 0;
        getIndexForPoint(pruned.lx, pruned.ly, ax, ay);
        getIndexForPoint(pruned.hx, pruned.hy, bx, by);

        // Outer ring of the rect.
        if (ax == bx)
        {
            int index = getPartitionForIndexes(ax, by);
            for (int y = by; y >= ay; --y)
            {
                if (partitions[index].topSize)
                {
                    partial[partialSize++] = index;
                }
                index--;
            }
        }
        else
        {
            for (int x = ax; x <= bx; x += (bx - ax))
            {
                int index = getPartitionForIndexes(x, by);
                for (int y = by; y >= ay; --y)
                {
                    if (partitions[index].topSize)
                    {
                        if (containsRect(pruned, partitions[index].rect))
                        {
                            solved[solvedSize++] = index;
                        }
                        else
                        {
                            partial[partialSize++] = index;
                        }
                    }
                    index--;
                }

            }
        }

        for (int x = ax + 1; x < bx; ++x)
        {
            int index = getPartitionForIndexes(x, by);
            for (int y = by; y >= ay; --y)
            {
                if (partitions[index].topSize)
                {
                    if (y == by || y == ay)
                    {
                        if (containsRect(pruned, partitions[index].rect))
                        {
                            solved[solvedSize++] = index;
                        }
                        else
                        {
                            partial[partialSize++] = index;
                        }
                    }
                    else
                    {
                        solved[solvedSize++] = index;
                    }
                }
                index--;
            }
        }
    }

    // Assumes actual point is inside our rect.
    inline int getIndexForPoint(float x, float y, int & out_x, int & out_y)
    {
        out_x = (int)(((double)x - (double)baselineRect.lx) * xdiv);
        out_y = (int)(((double)y - (double)baselineRect.ly) * ydiv);
        out_x = out_x > xmax ? xmax : out_x;
        out_y = out_y > ymax ? ymax : out_y;

        int index = out_y + out_x * powshift;
#ifdef __ERROR_CHECK__
        if (index < 0 || index >= partitions.size())
        {
            cout << "GetIndexForPoint: " << endl;
            cout << "x: " << x << " y: " << y << " xdiv: " << xdiv << " ydiv: " << ydiv << endl;
            cout << "yoff: " << out_y << " xoff: " << out_x << endl;
        }

        if (!isPointInsideRect(partitions[index].rect, x, y))
        {
            cout << "getIndexForPoint ERROR: " << index << " " << out_x << " " << out_y << " " << xdiv << " " << ydiv << endl;
        }
#endif

        return index;
    }

    inline int getIndexForPoint(float x, float y)
    {
        int x_out;
        int y_out;
        return getIndexForPoint(x, y, x_out, y_out);
    }

    bool Calculate(const Rect & rect, vector<Point *> & points, int count, bool quitIfNoSolves)
    {
        points.clear();

        GetPartitions(rect);
        if (solvedSize == 0 && quitIfNoSolves)
        {
            return false;
        }

#ifdef __ANALYTICS__
        startingsolves += solvedSize;
#endif
        for (int i = 0; i < solvedSize; ++i)
        {
            int index = solved[i];
            NodePartition * partition = &partitions[index];

            if (points.size() >= QUERY_COUNT)
            {
                // We have at least 1 point better than the worst.
                if (partition->minRank <= points.back()->rank)
                {
                    // Merge lists
                    copy(points.begin(), points.end(), zipBuffer);
                    int zipIndex = 0;
                    int tpIndex = 0;

                    for (int i = 0; i < QUERY_COUNT; ++i)
                    {
                        if (tpIndex < partition->topSize && PointCopy[partition->points[tpIndex]].rank < zipBuffer[zipIndex]->rank)
                        {
                            points[i] = &PointCopy[partition->points[tpIndex++]];
                        }
                        else
                        {
                            points[i] = zipBuffer[zipIndex++];
                        }
                    }
                }
            }
            else
            {
                if (points.size())
                {
                    //points.insert(points.end(), partition->top_points.begin(), partition->top_points.end());
                    for (int i = 0; i < partition->topSize; ++i)
                    {
                        points.push_back(&PointCopy[partition->points[i]]);
                    }

                    std::sort(points.begin(), points.end(), pointerPointRankComparison);

                    if (points.size() > QUERY_COUNT)
                    {
                        points.resize(QUERY_COUNT);
                    }
                }
                else
                {
                    //points = partition->top_points;
                    for (int i = 0; i < partition->topSize; ++i)
                    {
                        points.push_back(&PointCopy[partition->points[i]]);
                    }
                }
            }
        }

        for (int i = 0; i < partialSize; ++i)
        {
            int index = partial[i];
            NodePartition * partition = &partitions[index];

            if (points.size() == QUERY_COUNT)
            {
                // Do we have something better in our top points?
                if (partition->minRank <= points.back()->rank)
                {
                    int worstRank = points.back()->rank;

                    //if (partition->topSize >= partition->points.size())
                    //{
                    //    // If we have the same number of points as top points, we don't have to go deeper. Just compare.
                    //    for (int j = 0; j < partition->points.size(); ++j)
                    //    {
                    //        Point * point = &PointCopy[partition->points[j]];
                    //
                    //        if (point->rank > worstRank)
                    //        {
                    //            break;
                    //        }
                    //
                    //        if (isPointInsideRect(rect, point->x, point->y))
                    //        {
                    //            pointBuffer[pointBufferSize++] = point;
                    //            worstRank = points[QUERY_COUNT - pointBufferSize]->rank; // Not 100% true, but gives us a quicker exit.
                    //            //if (found == QUERY_COUNT) Need this when top_points is > query count...
                    //            //{
                    //            //    break;
                    //            //}
                    //        }
                    //    }
                    //
                    //    //////////////////////////////
                    //    // Merge lists
                    //    /////////////////////////////
                    //    if (pointBufferSize)
                    //    {
                    //        copy(points.begin(), points.end(), zipBuffer);
                    //        int zipIndex = 0;
                    //        int tpIndex = 0;
                    //        for (int i = 0; i < QUERY_COUNT; ++i)
                    //        {
                    //            if (tpIndex < pointBufferSize && pointBuffer[tpIndex]->rank < zipBuffer[zipIndex]->rank)
                    //            {
                    //                points[i] = pointBuffer[tpIndex++];
                    //            }
                    //            else
                    //            {
                    //                points[i] = zipBuffer[zipIndex++];
                    //            }
                    //        }
                    //        pointBufferSize = 0;
                    //    }
                    //}
                    //else
                    {
                        // More points than top points.
                        // If we can find a point that won't be inserted, we don't need to travel any deeper to evaluate.
                        // Only use this if we need to node-traverse deeper.
                        //bool needToGoDeeper = true;
                        //int startIndex;
                        //int worstRank = points.back()->rank;
                        //for (startIndex = 0; startIndex < partition->topSize; ++startIndex)
                        //{
                        //    Point * point = &PointCopy[partition->points[startIndex]];
                        //
                        //    if (point->rank > worstRank)
                        //    {
                        //        needToGoDeeper = false;
                        //        break;
                        //    }
                        //
                        //    if (isPointInsideRect(rect, point->x, point->y))
                        //    {
                        //        pointBuffer.push_back(point);
                        //        found++;
                        //        worstRank = points[QUERY_COUNT - found]->rank; // This isn't technically the worst point, but don't want to search through list to find and compare.
                        //    }
                        //}
                        //
                        //if (found == QUERY_COUNT) // Pretty rare, but helps if we can hit it and bail here.
                        //{
                        //    needToGoDeeper = false;
                        //    break;
                        //}
                        ////////////////////////////////
                        //// Merge lists
                        ///////////////////////////////
                        //if (pointBuffer.size())
                        //{
                        //    copy(points.begin(), points.end(), zipBuffer);
                        //    int zipIndex = 0;
                        //    int tpIndex = 0;
                        //    for (int i = 0; i < QUERY_COUNT; ++i)
                        //    {
                        //        if (tpIndex < pointBuffer.size() && pointBuffer[tpIndex]->rank < zipBuffer[zipIndex]->rank)
                        //        {
                        //            points[i] = pointBuffer[tpIndex++];
                        //        }
                        //        else
                        //        {
                        //            points[i] = zipBuffer[zipIndex++];
                        //        }
                        //    }
                        //
                        //    // We didn't use all our newly found points, so we're done.
                        //    if (pointBuffer.back()->rank > points.back()->rank)
                        //    {
                        //        needToGoDeeper = false;
                        //    }
                        //    pointBuffer.clear();
                        //}
                        //
                        ///////////////////////////////
                        //if (needToGoDeeper)
                        {
#ifdef __ANALYTICS__
                            depthCounts[depth] += 1;
                            deeperCount++;
#endif

                            for (int j = 0; j < partition->points.size() && pointBufferSize < QUERY_COUNT; ++j)
                            {
#ifdef __ANALYTICS__
                                deeperSearch++;
#endif
                                Point * point = &PointCopy[partition->points[j]];

                                if (point->rank > worstRank)
                                {
                                    break;
                                }

                                if (isPointInsideRect(rect, point->x, point->y))
                                {
                                    pointBuffer[pointBufferSize++] = point;
                                    // This isn't technically the worst point, but don't want to search through list to find and compare.
                                    worstRank = points[QUERY_COUNT - pointBufferSize]->rank;
                                }
                            }

                            ///////////////////////////
                            if (pointBufferSize)
                            {
                                copy(points.begin(), points.end(), zipBuffer);
                                int zipIndex = 0;
                                int tpIndex = 0;
                                for (int i = 0; i < QUERY_COUNT; ++i)
                                {
                                    if (tpIndex < pointBufferSize && pointBuffer[tpIndex]->rank < zipBuffer[zipIndex]->rank)
                                    {
                                        points[i] = pointBuffer[tpIndex++];
                                    }
                                    else
                                    {
                                        points[i] = zipBuffer[zipIndex++];
                                    }
                                }
                                pointBufferSize = 0;
                            }
                        }
                    }
                }
            }
            else
            {
                int found = 0;
                // We haven't filled our point list yet...
                for (int j = 0; j < partition->points.size() && found < QUERY_COUNT; ++j)
                {
                    Point * point = &PointCopy[partition->points[j]];

                    if (isPointInsideRect(rect, point->x, point->y))
                    {
                        points.push_back(point);
                        found++;
                    }
                }
                if (found)
                {
                    std::sort(points.begin(), points.end(), pointerPointRankComparison);
                    if (points.size() > QUERY_COUNT)
                    {
                        points.resize(QUERY_COUNT);
                    }
                }
            }
        }

        return true;
    }


    void addPoints(vector <Point> & points)
    {
        for (int point_index = 0; point_index < points.size(); ++point_index)
        {
            if (isPointInsideRect(baselineRect, points[point_index].x, points[point_index].y))
            {
                int index = getIndexForPoint(points[point_index].x, points[point_index].y);

#ifdef __ERROR_CHECK__
                if (index < 0 || index >= points.size())
                {
                    cout << "\nFAIL" << endl;
                    cout << baselineRect << endl;
                    cout << index << endl;
                    cout << points[point_index] << endl;
                    break;
                }
#endif
                partitions[index].AddPoint(&points[point_index], point_index);
            }
        }
    }
};



///////////////////////////////////////////////

PointContainer::PointContainer(const Point * points_begin, const Point * points_end)
{
    PointCopy = vector<Point>(points_begin, points_end);

    std::sort(PointCopy.begin(), PointCopy.end(), pointRankComparison);

    vector<int> outliers;

    float min_x = FLT_MAX;
    float max_x = -FLT_MAX;
    float min_y = FLT_MAX;
    float max_y = -FLT_MAX;
    int count = 0;
    for (int i = 0; i < PointCopy.size(); ++i)
    {
        float x = PointCopy[i].x;
        float y = PointCopy[i].y;
        // Need a better way to handle this horrible assumption.
        // Also need a better way to handle the data sets with horizontal/vertical lines.
        if (x < -10000.0f || y < -10000.0f || x > 10000.0f || y > 10000.0f)
        {
            outliers.push_back(i);
        }
    }

#ifdef __OUTPUT_POINTS__
    ofstream file;
    file.open("points.txt");
#endif

    for (int i = 0; i < PointCopy.size(); ++i)
    {
        bool existsInOutliers = false;
        for (int j = 0; j < outliers.size(); ++j)
        {
            if (outliers[j] == i)
            {
                existsInOutliers = true;
                break;
            }
        }

        if (!existsInOutliers)
        {
            float x = PointCopy[i].x;
            float y = PointCopy[i].y;
            min_x = ((x < min_x) ? x : min_x);
            max_x = ((x > max_x) ? x : max_x);
            min_y = ((y < min_y) ? y : min_y);
            max_y = ((y > max_y) ? y : max_y);

            if (i < QUERY_COUNT)
            {
                top20.push_back(PointCopy[i]);
            }
#ifdef __OUTPUT_POINTS__
            file << PointCopy[i].x << " " << PointCopy[i].y << endl;
#endif
        }
    }
#ifdef __OUTPUT_POINTS__
    file.close();
#endif
    mainRect.lx = min_x;
    mainRect.ly = min_y;
    mainRect.hx = max_x;
    mainRect.hy = max_y;

    // Weird case where all points are in a line, just give our rect some dimension.
    // Our algorithm isn't optimized for this case, but at least this way it will still pass.
    if (mainRect.lx == mainRect.hx)
    {
        mainRect.lx -= 1.0f;
        mainRect.hx += 1.0f;
    }
    
    if (mainRect.ly == mainRect.hy)
    {
        mainRect.ly -= 1.0f;
        mainRect.hy += 1.0f;
    }

    nodeContainer.clear();
    int minDepth = MIN_DEPTH;
    int maxDepth = MAX_DEPTH;
    if (PointCopy.size())
    {
        for (int i = 0; i < maxDepth - minDepth; ++i)
        {
            nodeContainer.resize(i + 1);

            if (!nodeContainer[i].create(mainRect, i + minDepth))
            {
                nodeContainer.pop_back();
                // Restart with new partitions. Right now this never occurs since node container always returns true.
                if (minDepth != 0)
                {
                    minDepth--;
                    maxDepth--;
                    i = 0;
                    nodeContainer.clear();
                }
                else
                {
                    break;
                }
            }
            else
            {
                nodeContainer[i].addPoints(PointCopy);
            }
        }
    }
    BASELINE_FACTOR = 1.001f * pow(0.5f, minDepth); // Magic number

#ifdef __ANALYTICS__
    ResetAnalytics();
#endif
}



void PointContainer::ResetAnalytics()
{
    missCount = 0;
    lessThanQUERY_COUNT = 0;
    totalDepth = 0;
    evalCount = 0;
    depthCounts.clear();
    depthCounts.resize(PointContainer::DEPTH);
    top20.clear();
    deeperCount = 0;
    deeperSearch = 0;
    startingsolves = 0;
}

void PointContainer::WriteAnalytics()
{
    printf("MissCount: %d\n", missCount);
    printf("< QUERY_COUNT: %d\n", lessThanQUERY_COUNT);

    printf("Total Depth: %f\n", (float)totalDepth / (float)evalCount);
    for (int i = 0; i < depthCounts.size(); ++i)
    {
        cout << i << ": " << depthCounts[i] << endl;
    }
    cout << "Deeper Count: " << deeperCount << endl;
    cout << "Average Time: " << (double)deeperSearch / deeperCount << endl;

    cout << "Starting solves: " << startingsolves << endl;
}

int32_t PointContainer::Evaluate(const Rect rect, const int32_t count, Point* out_points)
{
    int32_t found = 0;
    outPoints.clear();
    outPoints.reserve(40);

    if (!nodeContainer.size())
    {
        return 0;
    }

    // Maybe we got lucky and it covers the whole thing...
    if (containsRect(rect, nodeContainer[0].baselineRect))
    {
        found = (int32_t)top20.size();
        copy(top20.begin(), top20.begin() + top20.size(), out_points);
    }
    else if (intersects(rect, nodeContainer[0].baselineRect))
    {
        // Guess how deep we should initially go.
        Rect pruned = pruneRect(rect, nodeContainer[0].baselineRect);
        float width = pruned.hx - pruned.lx;
        float height = pruned.hy - pruned.ly;
        float factor = min(width / nodeContainer[0].width, height / nodeContainer[0].height);
        int depth = 0;
        float f = BASELINE_FACTOR;
        while (factor < f && depth < nodeContainer.size() - 1)
        {
            f *= 0.5f;
            depth++;
        }
        while (!nodeContainer[depth].Calculate(pruned, outPoints, count, depth != nodeContainer.size() - 1))
        {
            depth++;
        }

#ifdef __ANALYTICS__
        totalDepth += depth;
        evalCount++;
        if (height == 0.0f)
        {
            cout << "factor: " << factor << " depth: " << depth << " rect: " << rect << endl;
        }
#endif

        if (outPoints.size() > 0)
        {
            found = (int32_t)outPoints.size();
            for (int i = 0; i < outPoints.size(); ++i)
            {
                out_points[i] = *outPoints[i];
            }
        }
    }

#ifdef __ANALYTICS__
    if (found == 0)
    {
        missCount++;
    }

    if (found < QUERY_COUNT)
    {
        lessThanQUERY_COUNT++;
    }
#endif
    return found;
}

PointContainer::~PointContainer()
{
    PointCopy.clear();

#ifdef __ANALYTICS__
    WriteAnalytics();
#endif
}

