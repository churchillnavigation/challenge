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
#include "DebugTimer.h"
#include <crtdefs.h>
#include <process.h>

using namespace std;


float BASELINE_FACTOR;
vector <NodeContainer> nodeContainer;
// Copy of all the points.
Point * PointCopy;
int PointCopySize = 0;

// Holds points sorted by x.
int * xyPoints;
int xyPointsSize = 0;
// Holds points sorted by y.
int * yxPoints;
int yxPointsSize = 0;

// Doing this means we can't search large quantities in smaller regions.
// Saves us from doing push_back on vector. Downside is we can't hold all points.
int partial[1 << (MAX_DEPTH + 2)];
int partialSize;

int solved[1 << (MAX_DEPTH + 2)];
int solvedSize;

// Buffer for the points.
int t1_buffer1[QUERY_COUNT];
int t1_buffer2[QUERY_COUNT];
int t1_pointBuffer[QUERY_COUNT];
int t1_outPointsBufferSize = 0;
int * t1_outPoints = t1_buffer1;
int * t1_zipBuffer = t1_buffer2;

// Stuff for running on the other thread.
int t2_buffer1[QUERY_COUNT];
int t2_buffer2[QUERY_COUNT];
int t2_pointBuffer[QUERY_COUNT];
int t2_outPointsBufferSize = 0;
int * t2_outPoints = t2_buffer1;
int * t2_zipBuffer = t2_buffer2;
int t2_startSolve;
int t2_endSolve;
NodeContainer * t2_NodeContainer;
Rect t2_rect;
volatile bool run_t2_partial = false;
volatile bool run_t2_solved = false;
int t2_solveindex;

// Returns whether rect1 intersects rect2
inline bool intersects(const Rect & rect1, const Rect & rect2)
{
    return (rect1.lx <= rect2.hx && rect1.hx >= rect2.lx && rect1.ly <= rect2.hy && rect1.hy >= rect2.ly);
}

// Determine if rect1 contains rect2.
inline bool containsRect(const Rect & rect1, const Rect & rect2)
{
    if (rect1.lx > rect2.lx)
        return false;
    if (rect1.hx < rect2.hx)
        return false;
    if (rect1.ly > rect2.ly)
        return false;
    if (rect1.hy < rect2.hy)
        return false;

    return true;
}

inline bool isPointInsideRect(const Rect & rect, float & x, float & y)
{
    if (rect.lx > x)
    {
        return false;
    }
    if (rect.hx < x)
    {
        return false;
    }
    if (rect.ly > y)
    {
        return false;
    }
    if (rect.hy < y)
    {
        return false;
    }
    return true;
}

static inline bool pointRankComparison(const Point & a, const Point & b)
{
    return a.rank < b.rank;
}

static inline bool pointerPointRankComparison(const Point * a, const Point * b)
{
    return a->rank < b->rank;
}

static inline bool pointCopyRankComparison(const int & a, const int & b)
{
    return PointCopy[a].rank < PointCopy[b].rank;
}

static inline bool pointXYRankComparison(const int & a, const int & b)
{
    if (PointCopy[a].x == PointCopy[b].x)
    {
        return PointCopy[a].y < PointCopy[b].y;
    }
    else
    {
        return PointCopy[a].x < PointCopy[b].x;
    }
}

static inline bool pointYXRankComparison(const int & a, const int & b)
{
    if (PointCopy[a].y == PointCopy[b].y)
    {
        return PointCopy[a].x < PointCopy[b].x;
    }
    else
    {
        return PointCopy[a].y < PointCopy[b].y;
    }
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

inline void zipMerge(int * & buffer1, int * & zipBuffer, int & buffer1size, int * & buffer2, int & buffer2Size)
{
    // Merge lists
    int zipIndex = 0;
    int tpIndex = 0;

    for (int i = 0; i < QUERY_COUNT; ++i)
    {
        if (tpIndex < buffer2Size && zipIndex < buffer1size)
        {
            zipBuffer[i] = PointCopy[buffer2[tpIndex]].rank < PointCopy[buffer1[zipIndex]].rank ? buffer2[tpIndex++] : buffer1[zipIndex++];
        }
        else
        {
            if (tpIndex < buffer2Size)
            {
                zipBuffer[i] = buffer2[tpIndex++];
            }
            else if (zipIndex < buffer1size)
            {
                zipBuffer[i] = buffer1[zipIndex++];
            }
        }
    }
    int * temp = zipBuffer;
    zipBuffer = buffer1;
    buffer1 = temp;
    buffer1size = min(QUERY_COUNT, buffer1size + buffer2Size);
}

inline void zipMergeNonUnique(int * & buffer1, int * & zipBuffer, int & buffer1size, int * & buffer2, int & buffer2Size)
{
    // Merge lists
    int zipIndex = 0;
    int tpIndex = 0;
    int i = 0;
    while (i < QUERY_COUNT)
    {
        if (tpIndex < buffer2Size && zipIndex < buffer1size)
        {
            zipBuffer[i] = PointCopy[buffer2[tpIndex]].rank < PointCopy[buffer1[zipIndex]].rank ? buffer2[tpIndex++] : buffer1[zipIndex++];
            if (zipBuffer[i] != zipBuffer[i - 1])
            {
                i++;
            }
        }
        else
        {
            if (tpIndex < buffer2Size)
            {
                zipBuffer[i] = buffer2[tpIndex++];
                if (zipBuffer[i] != zipBuffer[i - 1])
                {
                    i++;
                }
            }
            else if (zipIndex < buffer1size)
            {
                zipBuffer[i] = buffer1[zipIndex++];
                if (zipBuffer[i] != zipBuffer[i - 1])
                {
                    i++;
                }
            }
            else
            {
                break;
            }
        }
    }
    int * temp = zipBuffer;
    zipBuffer = buffer1;
    buffer1 = temp;
    buffer1size = min(QUERY_COUNT, i);
}

Rect mainRect;
vector<Point> top20;

// Analytics stuff
#ifdef __ANALYTICS__
int deeperCount = 0;
int deeperSearch;
int lessThanQUERY_COUNT;
int totalDepth;
int evalCount;
int startingsolves;
vector<int> depthCounts;
int missCount;
double solveTime;
double partialTime;
double partitionTime;
double timeB;
double timeC;
int maxpartial;
int maxsolved;
int solvemisscount;
#endif

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
    int divxmax;
    int divymax;
    int depth;
    int powshift;
    int mindepth;
    int mindepthfactor;

    NodeContainer * deeperContainer = NULL;
    bool isDeepest;

    bool createFromOther(Rect rect, int _depth, NodeContainer & other)
    {
#ifdef __ANALYTICS__
        cout << endl << "Depth: " << _depth << " Rect: " << rect << endl;
#endif
        isDeepest = false;
        baselineRect = rect;
        depth = _depth;
        powshift = (int)pow(2, depth);
        xmax = powshift - 1;
        ymax = powshift - 1;
        partitions.resize(powshift * powshift);
        width = rect.hx - rect.lx;
        height = rect.hy - rect.ly;

        divxmax = other.divxmax;
        divymax = other.divymax;
        xdiv = other.xdiv;
        ydiv = other.ydiv;
        xmult = other.xmult;
        ymult = other.ymult;
        mindepth = other.mindepth;
        mindepthfactor = mindepth - depth;
        deeperContainer = &other;


        vector<vector<int>> tempPartition;
        tempPartition.resize(partitions.size());

        int my_ax, my_ay;
        for (int x = 0; x <= other.xmax; x++)
        {
            my_ax = x / 2;
            for (int y = 0; y <= other.ymax; ++y)
            {
                my_ay = y / 2;
                int myIndex = my_ax * powshift + my_ay;
                int lowerIndex = x * other.powshift + y;

                vector<int> tempVector;
                for (int point = 0; point < other.partitions[lowerIndex].PointsSize; ++point)
                {
                    tempVector.push_back(other.partitions[lowerIndex].points[point]);
                }
                tempPartition[myIndex].insert(tempPartition[myIndex].end(), tempVector.begin(), tempVector.end());
            }
        }


        for (int x = 0; x <= xmax; ++x)
        {
            for (int y = 0; y <= ymax; ++y)
            {
                int index = x * powshift + y;
                std::sort(tempPartition[index].begin(), tempPartition[index].end(), pointCopyRankComparison);
                if (tempPartition[index].size())
                {
                    // Could be just > CAP_SIZE, but we're not checking the capped status of nodes beneath us,
                    // so we have to assume if == to cap, it must be capped.
                    if (tempPartition[index].size() >= CAP_SIZE && depth <= CAP_DEPTH)
                    {
                        tempPartition[index].resize(CAP_SIZE);
                        partitions[index].cappedPartition = true;
                    }

                    partitions[index].AddPoints(tempPartition[index], PointCopy[tempPartition[index].front()].rank);
                }

                int pointlow = 2 * x * other.powshift + 2 * y;
                int pointhigh = pointlow + 1 + other.powshift;

                partitions[index].rect.lx = other.partitions[pointlow].rect.lx;
                partitions[index].rect.ly = other.partitions[pointlow].rect.ly;
                partitions[index].rect.hx = other.partitions[pointhigh].rect.hx;
                partitions[index].rect.hy = other.partitions[pointhigh].rect.hy;
            }
        }


#ifdef __ANALYTICS__
        printf("%f %f %d %d %d\n", xdiv, ydiv, xmax, ymax, depth);
#endif
        return true;
    }

    bool create(Rect rect, int _depth)
    {
#ifdef __ANALYTICS__
        cout << endl << "Depth: " << _depth << " Rect: " << rect << endl;
#endif

        isDeepest = true;
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
        mindepth = depth;

        divxmax = xmax;
        divymax = ymax;
        mindepthfactor = 0;
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
    inline bool GetPartitions(const Rect & rect, bool quitIfNoSolves)
    {
#ifdef __ANALYTICS__
        if (!intersects(rect, baselineRect))
        {
            cout << "Missed Partition Rect: " << rect << " Baseline Rect: " << baselineRect << endl;
            return;
        }
#endif
        int ax, ay, bx, by;
        partialSize = 0;
        solvedSize = 0;
        getIndexForPoint(rect.lx, rect.ly, ax, ay);
        getIndexForPoint(rect.hx, rect.hy, bx, by);

        if (quitIfNoSolves)
        {
            // Not always 100% true that there are no solves, but this catches more cases than the boundaries.
            if (ax == bx || ay == by)
            {
                return false;
            }
        }
        // Outer ring of the rect.
        if (ax == bx)
        {
            int index = getPartitionForIndexes(ax, ay);
            for (int y = ay; y <= by; ++y)
            {
                if (partitions[index].topSize)
                {
                    partial[partialSize++] = index;
                }
                index++;
            }
        }
        else
        {
            for (int x = ax; x <= bx; x += (bx - ax))
            {
                int index = getPartitionForIndexes(x, ay) - 1;
                for (int y = ay; y <= by; y++)
                {
                    if (partitions[++index].topSize)
                    {
                        if (containsRect(rect, partitions[index].rect))
                        {
                            solved[solvedSize++] = index;
                        }
                        else
                        {
                            partial[partialSize++] = index;
                        }
                    }
                }
            }
        }

        for (int x = ax + 1; x < bx; ++x)
        {
            int index = getPartitionForIndexes(x, ay) - 1;
            for (int y = ay; y <= by; ++y)
            {
                if (partitions[++index].topSize)
                {
                    if (y != by && y != ay)
                    {
                        solved[solvedSize++] = index;
                    }
                    else
                    {
                        if (containsRect(rect, partitions[index].rect))
                        {
                            solved[solvedSize++] = index;
                        }
                        else
                        {
                            partial[partialSize++] = index;
                        }
                    }
                }
            }
        }
#ifdef __ANALYTICS__
        partitionTime += DebugTimer::GetCounter();
#endif
        return solvedSize > 0 || !quitIfNoSolves;
    }

    inline void processSolvedPartition(int partition_index, int * & outPoints, int & outPointsBufferSize, int * & zipBuffer)
    {
        NodePartition * nodepartition = &partitions[partition_index]; 
        if (outPointsBufferSize >= QUERY_COUNT)
        {
            // We have at least 1 point better than the worst.
            if (nodepartition->minRank <= PointCopy[outPoints[outPointsBufferSize - 1]].rank)
            {
                // Merge lists
                int zipIndex = 0;
                int tpIndex = 0;

                for (int i = 0; i < QUERY_COUNT; ++i)
                {
                    if (tpIndex < nodepartition->topSize && PointCopy[nodepartition->points[tpIndex]].rank < PointCopy[outPoints[zipIndex]].rank)
                    {
                        zipBuffer[i] = nodepartition->points[tpIndex++];
                    }
                    else
                    {
                        zipBuffer[i] = outPoints[zipIndex++];
                    }
                }
                int * temp = zipBuffer;
                zipBuffer = outPoints;
                outPoints = temp;
            }
        }
        else
        {
            if (outPointsBufferSize)
            {
                // Merge lists
                int zipIndex = 0;
                int tpIndex = 0;

                for (int i = 0; i < QUERY_COUNT; ++i)
                {
                    if (tpIndex < nodepartition->topSize && zipIndex < outPointsBufferSize)
                    {
                        zipBuffer[i] = PointCopy[nodepartition->points[tpIndex]].rank < PointCopy[outPoints[zipIndex]].rank ? nodepartition->points[tpIndex++] : outPoints[zipIndex++];
                    }
                    else
                    {
                        if (tpIndex < nodepartition->topSize)
                        {
                            zipBuffer[i] = nodepartition->points[tpIndex++];
                        }
                        else if (zipIndex < outPointsBufferSize)
                        {
                            zipBuffer[i] = outPoints[zipIndex++];
                        }
                    }
                }
                int * temp = zipBuffer;
                zipBuffer = outPoints;
                outPoints = temp;
                outPointsBufferSize = min(QUERY_COUNT, outPointsBufferSize + nodepartition->topSize);
            }
            else
            {
                copy(nodepartition->points, nodepartition->points + nodepartition->topSize, outPoints);
                outPointsBufferSize = nodepartition->topSize;
            }
        }
    }

    inline void processPartialPartition(const Rect & rect, int partition_index, int * & outPoints, int & outPointsBufferSize, int * & zipBuffer, int * pointBuffer)
    {
        int pointBufferSize = 0;
        int j = 0;
        NodePartition * nodepartition = &partitions[partition_index];
        int startingOutPoints = outPointsBufferSize;
        if (outPointsBufferSize == QUERY_COUNT)
        {
            int worstRank = PointCopy[outPoints[outPointsBufferSize - 1]].rank;
            // Do we have something better in our top points?
            if (nodepartition->minRank <= worstRank)
            {
#ifdef __ANALYTICS__
                depthCounts[depth] += 1;
                deeperCount++;
#endif

                for (j = 0; j < nodepartition->PointsSize && pointBufferSize < QUERY_COUNT; ++j)
                {
#ifdef __ANALYTICS__
                    deeperSearch++;
#endif
                    Point * point = &PointCopy[nodepartition->points[j]];

                    if (point->rank > worstRank)
                    {
                        break;
                    }

                    if (isPointInsideRect(rect, point->x, point->y))
                    {
                        pointBuffer[pointBufferSize++] = nodepartition->points[j];
                        // This isn't technically the worst point, but don't want to search through list to find and compare.
                        worstRank = PointCopy[outPoints[QUERY_COUNT - pointBufferSize]].rank;
                    }
                }

                // Zero out the point buffer if we have to dig deeper. "Undo" our changes.
                pointBufferSize = (nodepartition->cappedPartition && j == nodepartition->PointsSize) ? 0 : pointBufferSize;

                ///////////////////////////
                if (pointBufferSize)
                {
                    int zipIndex = 0;
                    int tpIndex = 0;
                    for (int i = 0; i < QUERY_COUNT; ++i)
                    {
                        if (tpIndex < pointBufferSize && PointCopy[pointBuffer[tpIndex]].rank < PointCopy[outPoints[zipIndex]].rank)
                        {
                            zipBuffer[i] = pointBuffer[tpIndex++];
                        }
                        else
                        {
                            zipBuffer[i] = outPoints[zipIndex++];
                        }
                    }
                    int * temp = zipBuffer;
                    zipBuffer = outPoints;
                    outPoints = temp;

                    pointBufferSize = 0;
                }
            }
        }
        else
        {
            if (outPointsBufferSize)
            {
                // We haven't filled our point list yet...
                for (j = 0; j < nodepartition->PointsSize && pointBufferSize < QUERY_COUNT; ++j)
                {
                    Point * point = &PointCopy[nodepartition->points[j]];

                    if (isPointInsideRect(rect, point->x, point->y))
                    {
                        pointBuffer[pointBufferSize++] = nodepartition->points[j];
                    }
                }

                // Zero out the point buffer if we have to dig deeper. "Undo" our changes.
                pointBufferSize = (nodepartition->cappedPartition && j == nodepartition->PointsSize) ? 0 : pointBufferSize;

                if (pointBufferSize)
                {
                    // Merge lists
                    int zipIndex = 0;
                    int tpIndex = 0;

                    for (int i = 0; i < QUERY_COUNT; ++i)
                    {
                        if (tpIndex < pointBufferSize && zipIndex < outPointsBufferSize)
                        {
                            zipBuffer[i] = PointCopy[pointBuffer[tpIndex]].rank < PointCopy[outPoints[zipIndex]].rank ? pointBuffer[tpIndex++] : outPoints[zipIndex++];
                        }
                        else
                        {
                            if (tpIndex < pointBufferSize)
                            {
                                zipBuffer[i] = pointBuffer[tpIndex++];
                            }
                            else if (zipIndex < outPointsBufferSize)
                            {
                                zipBuffer[i] = outPoints[zipIndex++];
                            }
                        }
                    }
                    int * temp = zipBuffer;
                    zipBuffer = outPoints;
                    outPoints = temp;
                    outPointsBufferSize = min(QUERY_COUNT, outPointsBufferSize + pointBufferSize);
                    pointBufferSize = 0;
                }
            }
            else
            {
                // We haven't filled our point list yet...
                for (j = 0; j < nodepartition->PointsSize && outPointsBufferSize < QUERY_COUNT; ++j)
                {
                    Point * point = &PointCopy[nodepartition->points[j]];

                    if (isPointInsideRect(rect, point->x, point->y))
                    {
                        outPoints[outPointsBufferSize++] = nodepartition->points[j];
                    }
                }
            }
        }
        
        // Ouch, hit max depth without finding enough points. Need to go deeper.
        if (nodepartition->cappedPartition && j == nodepartition->PointsSize)
        {
            outPointsBufferSize = startingOutPoints; // Need this for "We haven't filled our point list yet..."
            // The other paths zero out pointBuffersize.
            processUnsolvedPartial(rect, partition_index, outPoints, outPointsBufferSize, zipBuffer, pointBuffer);
        }
    }


    inline void processUnsolvedPartial(const Rect & rect, int partition_index, int * & outPoints, int & outPointsBufferSize, int * & zipBuffer, int * pointBuffer)
    {
        if (deeperContainer != NULL)
        {
            int x = partition_index / powshift;
            int y = partition_index - x * powshift;

            int point1 = 2 * x * deeperContainer->powshift + 2 * y;
            int point2 = point1 + 1;
            int point3 = point1 + deeperContainer->powshift;
            int point4 = point3 + 1;
            NodePartition * nextPartitions = &deeperContainer->partitions[0];

            // First quad.
            if (containsRect(rect, nextPartitions[point1].rect))
            {
                deeperContainer->processSolvedPartition(point1, outPoints, outPointsBufferSize, zipBuffer);
            }
            else if (intersects(rect, nextPartitions[point1].rect))
            {
                deeperContainer->processPartialPartition(rect, point1, outPoints, outPointsBufferSize, zipBuffer, pointBuffer);
            }

            // Second quad.
            if (containsRect(rect, nextPartitions[point2].rect))
            {
                deeperContainer->processSolvedPartition(point2, outPoints, outPointsBufferSize, zipBuffer);
            }
            else if (intersects(rect, nextPartitions[point2].rect))
            {
                deeperContainer->processPartialPartition(rect, point2, outPoints, outPointsBufferSize, zipBuffer, pointBuffer);
            }

            // Third quad.
            if (containsRect(rect, nextPartitions[point3].rect))
            {
                deeperContainer->processSolvedPartition(point3, outPoints, outPointsBufferSize, zipBuffer);
            }
            else if (intersects(rect, nextPartitions[point3].rect))
            {
                deeperContainer->processPartialPartition(rect, point3, outPoints, outPointsBufferSize, zipBuffer, pointBuffer);
            }

            // Fourth quad.
            if (containsRect(rect, nextPartitions[point4].rect))
            {
                deeperContainer->processSolvedPartition(point4, outPoints, outPointsBufferSize, zipBuffer);
            }
            else if (intersects(rect, nextPartitions[point4].rect))
            {
                deeperContainer->processPartialPartition(rect, point4, outPoints, outPointsBufferSize, zipBuffer, pointBuffer);
            }
        }
    }

    // Assumes actual point is inside our rect.
    inline int getIndexForPoint(float x, float y, int & out_x, int & out_y)
    {
        out_x = (int)(((double)x - (double)baselineRect.lx) * xdiv);
        out_y = (int)(((double)y - (double)baselineRect.ly) * ydiv);
        out_x = out_x > divxmax ? divxmax : out_x;
        out_y = out_y > divymax ? divymax : out_y;

        // We now have the coordinate of the deepest container.
        // Translocate to our own coordinates.
        out_x = out_x >> mindepthfactor;
        out_y = out_y >> mindepthfactor;

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

    bool Calculate(const Rect & rect, int count, bool quitIfNoSolves)
    {
        t1_outPointsBufferSize = 0;
        t2_outPointsBufferSize = 0;
        if (!GetPartitions(rect, quitIfNoSolves))
        {
#ifdef __ANALYTICS__
            solvemisscount++;
#endif
            return false;
        }

#ifdef __ANALYTICS__
        startingsolves += solvedSize;
        maxsolved = max(maxsolved, solvedSize);
        maxpartial = max(maxpartial, partialSize);

        DebugTimer::StartCounter();
#endif
        // Solved nodes. Multithread if there are enough to find.
        if (solvedSize > SOLVED_THRESHOLD)
        {
            t2_startSolve = solvedSize / 2;
            t2_endSolve = solvedSize;
            t2_NodeContainer = this;
            run_t2_solved = true;

            for (int i = 0; i < t2_startSolve; ++i)
            {
                processSolvedPartition(solved[i], t1_outPoints, t1_outPointsBufferSize, t1_zipBuffer);
            }

            while (run_t2_solved)
            {
                Sleep(0);
            }
            zipMerge(t1_outPoints, t1_zipBuffer, t1_outPointsBufferSize, t2_outPoints, t2_outPointsBufferSize);
        }
        else
        {
            for (int i = 0; i < solvedSize; ++i)
            {
                processSolvedPartition(solved[i], t1_outPoints, t1_outPointsBufferSize, t1_zipBuffer);
            }
        }
#ifdef __ANALYTICS__
        solveTime += DebugTimer::GetCounter();

        
        DebugTimer::StartCounter();
#endif

        // Partial nodes. Multithread if there are enough.
        if (partialSize > PARTIAL_THRESHOLD)
        {
            t2_startSolve = partialSize / 2;
            t2_endSolve = partialSize;
            t2_NodeContainer = this;
            t2_outPointsBufferSize = t1_outPointsBufferSize;
            copy(t1_outPoints, t1_outPoints + t1_outPointsBufferSize, t2_outPoints);
            t2_rect = rect;

            run_t2_partial = true;

            for (int i = 0; i < t2_startSolve; ++i)
            {
                processPartialPartition(rect, partial[i], t1_outPoints, t1_outPointsBufferSize, t1_zipBuffer, t1_pointBuffer);
            }

            while (run_t2_partial)
            {
                Sleep(0);
            }
            zipMergeNonUnique(t1_outPoints, t1_zipBuffer, t1_outPointsBufferSize, t2_outPoints, t2_outPointsBufferSize);
        }
        else
        {
            for (int i = 0; i < partialSize; ++i)
            {
                processPartialPartition(rect, partial[i], t1_outPoints, t1_outPointsBufferSize, t1_zipBuffer, t1_pointBuffer);
            }
        }
#ifdef __ANALYTICS__
        partialTime += DebugTimer::GetCounter();
#endif
        return true;
    }


    void AddPoints(Point * points, int PointCopySize, bool cull = false)
    {
        vector<vector <int>> InitialPoints;
        InitialPoints.resize(partitions.size());

        for (int point_index = 0; point_index < PointCopySize; ++point_index)
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
                InitialPoints[index].push_back(point_index);
            }
        }

        for (int i = 0; i < InitialPoints.size(); ++i)
        {
            if (InitialPoints[i].size())
            {
                partitions[i].AddPoints(InitialPoints[i], PointCopy[InitialPoints[i][0]].rank);
            }
        }
    }
};



///////////////////////////////////////////////

PointContainer::PointContainer(const Point * points_begin, const Point * points_end)
{
    PointCopySize = (points_end - points_begin);
    if (PointCopySize)
    {
        PointCopy = new Point[PointCopySize];
        copy(points_begin, points_end, PointCopy);
        std::sort(PointCopy, PointCopy + PointCopySize, pointRankComparison);

        xyPoints = new int[PointCopySize];
        yxPoints = new int[PointCopySize];
    }
    vector<int> outliers;

    float min_x = FLT_MAX;
    float max_x = -FLT_MAX;
    float min_y = FLT_MAX;
    float max_y = -FLT_MAX;
    int count = 0;
    for (int i = 0; i < PointCopySize; ++i)
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

    for (int i = 0; i < PointCopySize; ++i)
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
            xyPoints[xyPointsSize++] = i;
            yxPoints[yxPointsSize++] = i;

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

    std::sort(xyPoints, xyPoints + xyPointsSize, pointXYRankComparison);
    std::sort(yxPoints, yxPoints + yxPointsSize, pointYXRankComparison);

    // Weird case where all points are in a line, just give our rect some dimension.
    // Our algorithm sucks for this case, but at least this way it will still pass.
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
    if (PointCopySize)
    {
        nodeContainer.resize((MAX_DEPTH - MIN_DEPTH) + 1);
        nodeContainer.back().create(mainRect, MAX_DEPTH);
        nodeContainer.back().AddPoints(PointCopy, PointCopySize);
        for (int i = MAX_DEPTH - MIN_DEPTH - 1; i >= 0; --i)
        {
            nodeContainer[i].createFromOther(mainRect, MIN_DEPTH + i, nodeContainer[i + 1]);
        }
    }
    BASELINE_FACTOR = 1.414f * pow(0.5f, MIN_DEPTH); // Magic number

    StartWorkerThreads();
#ifdef __PREPCACHE__
    PrepCache();
#endif
#ifdef __ANALYTICS__
    ResetAnalytics();
#endif
}

int getXYPoints(const Rect & rect, int & lowIndex, int & highIndex)
{
    int lowPoint = 0;
    int highPoint = xyPointsSize - 1;

    // Find low point. Binary search.
    while (highPoint != lowPoint)
    {
        int midPoint = (highPoint + lowPoint) / 2;
        if (rect.lx <= PointCopy[xyPoints[midPoint]].x)
        {
            highPoint = midPoint;
        }
        else
        {
            lowPoint = midPoint;
        }
        if (lowPoint == highPoint - 1)
        {
            if (rect.lx > PointCopy[xyPoints[lowPoint]].x)
            {
                lowPoint = highPoint;
            }
            else
            {
                break;
            }
        }
    }
    lowIndex = lowPoint;

    lowPoint = 0;
    highPoint = xyPointsSize - 1;
    // Find high point. Binary search.
    while (highPoint != lowPoint)
    {
        int midPoint = (highPoint + lowPoint) / 2;
        if (rect.hx >= PointCopy[xyPoints[midPoint]].x)
        {
            lowPoint = midPoint;
        }
        else
        {
            highPoint = midPoint;
        }
        if (lowPoint == highPoint - 1)
        {
            if (rect.hx < PointCopy[xyPoints[highPoint]].x)
            {
                highPoint = lowPoint;
            }
            else
            {
                break;
            }
        }
    }
    highIndex = highPoint;
    return highPoint - lowIndex;
}

void solveXYRect(const Rect & rect, int solvedLowPoint, int solvedHighPoint)
{
    t1_outPointsBufferSize = 0;
    int * pointBuffer = &t1_pointBuffer[0];
    int pointBufferSize = 0;
    int worstRank = INT_MAX;
    
    solvedLowPoint--;
    while (++solvedLowPoint <= solvedHighPoint)
    {
        if (PointCopy[xyPoints[solvedLowPoint]].y >= rect.ly && PointCopy[xyPoints[solvedLowPoint]].y <= rect.hy)
        {
            if (PointCopy[xyPoints[solvedLowPoint]].rank < worstRank)
            {
                pointBuffer[pointBufferSize++] = xyPoints[solvedLowPoint];

                // Sort and merge.
                if (pointBufferSize >= QUERY_COUNT)
                {
                    std::sort(pointBuffer, pointBuffer + QUERY_COUNT, pointCopyRankComparison);
                    zipMerge(t1_outPoints, t1_zipBuffer, t1_outPointsBufferSize, pointBuffer, pointBufferSize);
                    worstRank = t1_outPoints[t1_outPointsBufferSize - 1];
                    pointBufferSize = 0;
                }
            }
        }
    }
    
    // Sort and merge.
    if (pointBufferSize)
    {
        std::sort(pointBuffer, pointBuffer + pointBufferSize, pointCopyRankComparison);
        zipMerge(t1_outPoints, t1_zipBuffer, t1_outPointsBufferSize, pointBuffer, pointBufferSize);
    }

}


int getYXPoints(const Rect & rect, int & lowIndex, int & highIndex)
{
    int lowPoint = 0;
    int highPoint = yxPointsSize - 1;

    // Find low point. Binary search.
    while (highPoint != lowPoint)
    {
        int midPoint = (highPoint + lowPoint) / 2;
        if (rect.ly <= PointCopy[yxPoints[midPoint]].y)
        {
            highPoint = midPoint;
        }
        else
        {
            lowPoint = midPoint;
        }
        if (lowPoint == highPoint - 1)
        {
            if (rect.ly > PointCopy[yxPoints[lowPoint]].y)
            {
                lowPoint = highPoint;
            }
            else
            {
                break;
            }
        }
    }
    lowIndex = lowPoint;

    lowPoint = 0;
    highPoint = yxPointsSize - 1;
    // Find high point. Binary search.
    while (highPoint != lowPoint)
    {
        int midPoint = (highPoint + lowPoint) / 2;
        if (rect.hy >= PointCopy[yxPoints[midPoint]].y)
        {
            lowPoint = midPoint;
        }
        else
        {
            highPoint = midPoint;
        }
        if (lowPoint == highPoint - 1)
        {
            if (rect.hy < PointCopy[yxPoints[highPoint]].y)
            {
                highPoint = lowPoint;
            }
            else
            {
                break;
            }
        }
    }
    highIndex = highPoint;
    return highPoint - lowIndex;
}

void solveYXRect(const Rect & rect, int solvedLowPoint, int solvedHighPoint)
{
    t1_outPointsBufferSize = 0;
    int * pointBuffer = &t1_pointBuffer[0];
    int pointBufferSize = 0;
    int worstRank = INT_MAX;

    solvedLowPoint--;
    while (++solvedLowPoint <= solvedHighPoint)
    {
        if (PointCopy[yxPoints[solvedLowPoint]].x >= rect.lx && PointCopy[yxPoints[solvedLowPoint]].x <= rect.hx)
        {
            if (PointCopy[yxPoints[solvedLowPoint]].rank < worstRank)
            {
                pointBuffer[pointBufferSize++] = yxPoints[solvedLowPoint];

                // Sort and merge.
                if (pointBufferSize >= QUERY_COUNT)
                {
                    std::sort(pointBuffer, pointBuffer + QUERY_COUNT, pointCopyRankComparison);
                    zipMerge(t1_outPoints, t1_zipBuffer, t1_outPointsBufferSize, pointBuffer, pointBufferSize);
                    worstRank = t1_outPoints[t1_outPointsBufferSize - 1];
                    pointBufferSize = 0;
                }
            }
        }
    }

    // Sort and merge.
    if (pointBufferSize)
    {
        std::sort(pointBuffer, pointBuffer + pointBufferSize, pointCopyRankComparison);
        zipMerge(t1_outPoints, t1_zipBuffer, t1_outPointsBufferSize, pointBuffer, pointBufferSize);
    }
}


void PointContainer::ResetAnalytics()
{
#ifdef __ANALYTICS__
    missCount = 0;
    lessThanQUERY_COUNT = 0;
    totalDepth = 0;
    evalCount = 0;
    depthCounts.clear();
    depthCounts.resize(MAX_DEPTH + 1);
    top20.clear();
    deeperCount = 0;
    deeperSearch = 0;
    startingsolves = 0;
    solveTime = 0;
    partialTime = 0;
    partitionTime = 0;
    timeB = 0;
    timeC = 0;
    maxpartial = 0;
    maxsolved = 0;
    solvemisscount = 0;
#endif
}

// Superstition.
void PointContainer::PrepCache()
{
    vector<Point> queryBuffer;
    queryBuffer.resize(QUERY_COUNT);
    volatile int test = 0;
    time_t currtime;
    time_t temp;
    time(&currtime);

    if (nodeContainer.size())
    {
        for (float x = mainRect.lx; x < mainRect.hx; x += (mainRect.hx - mainRect.lx) / 3000.0f)
        {
            for (float y = mainRect.ly; y < mainRect.hy && time(&temp) - currtime < 20; y += (mainRect.hy - mainRect.ly) / 2000.0f)
            {
                Rect rect = { mainRect.lx, x, mainRect.ly, y };
                Evaluate(rect, QUERY_COUNT, &queryBuffer[0]);
                test += queryBuffer.size();
            }
        }

        for (float y = mainRect.ly; y < mainRect.hy; y += (mainRect.hy - mainRect.ly) / 3000.0f)
        {
            for (float x = mainRect.lx; x < mainRect.hx && time(&temp) - currtime < 20; x += (mainRect.hx - mainRect.lx) / 2000.0f)
            {
                Rect rect = { mainRect.lx, x, mainRect.ly, y };
                Evaluate(rect, QUERY_COUNT, &queryBuffer[0]);
                test += queryBuffer.size();
            }
        }
    }
}

void PointContainer::WriteAnalytics()
{
#ifdef __ANALYTICS__
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
    cout << "Solve Time: " << solveTime << endl;
    cout << "Partial Time:  " << partialTime << endl;
    cout << "PartitionTime: " << partitionTime << endl;
    cout << "timeB: " << timeB << endl;
    cout << "timeC: " << timeC << endl;
    cout << "maxpartial: " << maxpartial << endl;
    cout << "maxsolves: " << maxsolved << endl;
    cout << "solvemisscount: " << solvemisscount << endl;
#endif
}

int32_t PointContainer::Evaluate(const Rect rect, const int32_t count, Point* out_points)
{
    int32_t found = 0;
    t1_outPointsBufferSize = 0;

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

        bool solved = false;

        bool lowX = width < nodeContainer[nodeContainer.size() - 1].xmult;
        bool lowY = height < nodeContainer[nodeContainer.size() - 1].ymult;
        
        if (lowX && lowY)
        {
            // Deep dive to the lowest container.
            nodeContainer[nodeContainer.size() - 1].Calculate(pruned, count, false);
            solved = true;
        }
        if (lowX && !solved)
        {
            int solvedLowPoint, solvedHighPoint;
            getXYPoints(rect, solvedLowPoint, solvedHighPoint);
            if (solvedHighPoint - solvedLowPoint < XY_THRESHOLD)
            {
                solveXYRect(pruned, solvedLowPoint, solvedHighPoint);
                solved = true;
            }
        }
        
        if (lowY && !solved)
        {
            int solvedLowPoint, solvedHighPoint;
            getYXPoints(rect, solvedLowPoint, solvedHighPoint);
            if (solvedHighPoint - solvedLowPoint < XY_THRESHOLD)
            {
                solveYXRect(pruned, solvedLowPoint, solvedHighPoint);
                solved = true;
            }
        }

        if (!solved)
        {
            float factor = min(width / nodeContainer[0].width, height / nodeContainer[0].height);
            int depth = 0;
            while (factor < BASELINE_FACTOR && depth < nodeContainer.size() - 1)
            {
                factor *= 2.0f;
                depth++;
            }

            while (!nodeContainer[depth].Calculate(pruned, count, depth != nodeContainer.size() - 1))
            {
                depth++;
            }
        }
#ifdef __ANALYTICS__
        totalDepth += depth;
        evalCount++;
        if (height == 0.0f)
        {
            timeB += DebugTimer::GetCounter(); 
            cout << "factor: " << factor << " depth: " << depth << " rect: " << rect << endl;
        }

        DebugTimer::StartCounter();
#endif

        found = (int32_t)t1_outPointsBufferSize;
        for (int i = 0; i < t1_outPointsBufferSize; ++i)
        {
            out_points[i] = PointCopy[t1_outPoints[i]];
        }
#ifdef __ANALYTICS__
        timeC += DebugTimer::GetCounter();
#endif
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

////////////////////////
// Threading
////////////////////////
volatile bool exit_t2 = false;

unsigned int __stdcall t2Thread(void* data)
{
    while (!exit_t2)
    {
        if (run_t2_partial)
        {
            for (int i = t2_startSolve; i < t2_endSolve; ++i)
            {
                t2_NodeContainer->processPartialPartition(t2_rect, partial[i], t2_outPoints, t2_outPointsBufferSize, t2_zipBuffer, &t2_pointBuffer[0]);
            }
            run_t2_partial = false;
        }
        if (run_t2_solved)
        {
            for (int i = t2_startSolve; i < t2_endSolve; ++i)
            {
                t2_NodeContainer->processSolvedPartition(solved[i], t2_outPoints, t2_outPointsBufferSize, t2_zipBuffer);
            }
            run_t2_solved = false;
        }
        Sleep(0);
    }
    return 0;
}


void PointContainer::StartWorkerThreads()
{
    exit_t2 = false;
    
    t2Handle = (HANDLE)_beginthreadex(0, 0, &t2Thread, 0, 0, 0);
}

void PointContainer::StopWorkerThreads()
{
    exit_t2 = true;
    WaitForSingleObject(t2Handle, INFINITE);
    CloseHandle(t2Handle);
}

// Destructor
PointContainer::~PointContainer()
{
    if (PointCopy)
    {
        delete[] PointCopy;
        PointCopySize = 0;
    }

    if (xyPoints)
    {
        delete[] xyPoints;
        xyPointsSize = 0;
    }

    if (yxPoints)
    {
        delete[] yxPoints;
        yxPointsSize = 0;
    }

    StopWorkerThreads();

#ifdef __ANALYTICS__
    if (nodeContainer.size())
    {
        WriteAnalytics();
    }
#endif
}

