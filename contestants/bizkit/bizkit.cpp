#include <stdint.h>
#include <vector>
#include <list>
#include <algorithm>

#define DLLIMPORT __declspec(dllexport)

#define NUMBER_OF_POINTS 10000000
#define NUMBER_OF_CLUSTERS 6400
#define X_CUTS 125000
#define XY_CUTS 1563
#define TRIVIAL_SEARCH_LIMIT_PER_POINT 360

float lx,hx,ly,hy;

/* The following structs are packed with no padding. */
#pragma pack(push, 1)

/* Defines a point in 2D space with some additional attributes like id and rank. */

struct Point
{
	int8_t id;
	int32_t rank;
	float x;
	float y;
};

/* Defines a rectangle, where a point (x,y) is inside, if x is in [lx, hx] and y is in [ly, hy]. */
struct Rect
{
	float lx;
	float ly;
	float hx;
	float hy;
};
#pragma pack(pop)

struct Cluster
{
  uint32_t size;
  Point * points;	
  float xMin;
  float xMax;
  float yMin;
  float yMax;
};

struct SearchContext 
{
  uint32_t nrOfPoints;      
  uint32_t nrOfClusters;      
  Point * points;  // all the points ordered according to rank
  Cluster * clusters;  // clusters are basically disjoint rectangles with points     
} context;

// finding xMin, xMax, yMin, yMax for a cluster
void findBorders(Cluster * cl)
{
  if ( (*cl).size == 0	)
    return;
  (*cl).xMin = (*cl).xMax = (*cl).points[0].x;
  (*cl).yMin = (*cl).yMax = (*cl).points[0].y;
  for (int32_t i = 1; i < (*cl).size; i++)
  {
    if ( (*cl).points[i].x < (*cl).xMin )
      (*cl).xMin = (*cl).points[i].x;
    else if ( (*cl).points[i].x > (*cl).xMax )
      (*cl).xMax = (*cl).points[i].x;
    if ( (*cl).points[i].y < (*cl).yMin )
      (*cl).yMin = (*cl).points[i].y;
    else if ( (*cl).points[i].y > (*cl).yMax )
      (*cl).yMax = (*cl).points[i].y;
  }
  return;
}

bool compareX (Point * p1, Point * p2) { return ( (*p1).x < (*p2).x ); }

bool compareY (Point * p1, Point * p2) { return ( (*p1).y < (*p2).y ); }

bool compareRank (Point * p1, Point * p2) { return ( (*p1).rank < (*p2).rank ); }


extern "C" DLLIMPORT SearchContext* __stdcall create(const Point* points_begin, const Point* points_end)
{
  context.nrOfPoints = points_end - points_begin;

  Point * points = new Point[context.nrOfPoints];    

// ordering points according to rank 
// (!minor cheating! I use that rank goes from 0 to 9999999)
  for (uint32_t i = 0; i<context.nrOfPoints; i++)
    points[ (*(points_begin+i)).rank ] = *(points_begin+i);
  context.points = points;
    
  int32_t clNr = 0;

  std::vector<Point *> sorting;

  for (uint32_t i = 0; i<context.nrOfPoints; i++)
    sorting.push_back(context.points+i);

// sorting points according to X-value
  std::sort( sorting.begin(), sorting.end(), compareX );  
    
  std::vector<Point *>::iterator it = sorting.begin();
  std::vector<Point *>::iterator end = sorting.end();
  
  Cluster * clusters = new Cluster[NUMBER_OF_CLUSTERS];

  while( it < end )
  {
// partitioning the points into vertical strips 
// and sorting the points inside each strip according to Y-value
  	int32_t xCutSize = ( it + X_CUTS <= end ) ? X_CUTS : end-it;		    	
  	std::sort( it, it+xCutSize, compareY );

      
    while( xCutSize > 0 )
  	{
// partitioning the points inside each strip into rectangles 
// and sorting them according to rank --> clusters
	  int32_t xyCutSize = ( XY_CUTS <= xCutSize ) ? XY_CUTS : xCutSize;

      Point * clusterPoints = new Point[xyCutSize];
	  
  	  std::sort( it, it+xyCutSize, compareRank );
  	    	  
	  for (int32_t ptNr = 0; ptNr < xyCutSize; ptNr++)
        clusterPoints[ptNr] = **(it+ptNr);

	  clusters[clNr].points = clusterPoints; 

      clusters[clNr].size = xyCutSize;

	  findBorders( clusters + clNr );    
  	    	          
      it += xyCutSize;
	  xCutSize -= xyCutSize;
  	  clNr++;
    }
  }
  context.nrOfClusters = clNr;
  context.clusters = clusters;

//  sorting.clear();
  
  return &context;              
};

extern "C" DLLIMPORT int32_t __stdcall search(SearchContext* sc, const Rect rect, const int32_t count, Point* out_points)
{
  if ( (*sc).nrOfPoints == 0 )
    return 0;

  lx = rect.lx;
  hx = rect.hx;
  ly = rect.ly;
  hy = rect.hy;

// TRYING TRIVIAL SEARCH FIRST
// we start going through the lowest-ranked points
// but we stop if we dont find points in the rectangle quickly enough 
  int32_t cnt = 0;

  Point * point = (*sc).points;
  Point * endpoint = point; 
  for (int32_t round = 0; round < count; round++)
  {
  	endpoint += TRIVIAL_SEARCH_LIMIT_PER_POINT;
  
    for ( ; point != endpoint; point++)
    {      
      if ( (*point).x < lx ) 
	    continue;
	  if ( (*point).x > hx )
	    continue;
	  if ( (*point).y < ly )
        continue;
	  if ( (*point).y > hy ) 
        continue;
    
      out_points[cnt] = *point;
    
      cnt++;
    
      if (cnt == count)
        return cnt;
    }
    if (cnt <= round)
      break;
  }

// CLUSTER SEARCH INSTEAD...
  
  cnt = 0;

  std::list<Cluster *> clstrs;  // list of "active" clusters

// finding all the clusters that (might) intersect rect  
  for (int32_t i=0; i < (*sc).nrOfClusters; i++)
  {
  	if ( (*sc).clusters[i].xMin > hx )
  	  continue;
  	if ( (*sc).clusters[i].xMax < lx )
  	  continue;
  	if ( (*sc).clusters[i].yMin > hy )
  	  continue;
  	if ( (*sc).clusters[i].yMax < ly )
  	  continue;
  	
	clstrs.push_back( (*sc).clusters + i );
  }
  
  cnt = 0;  //number of points found in rect
  int32_t n = 0;  //we will take the n-th ranked point in each cluster; n=0,1,2,...
  std::list<Cluster *>::iterator it;
  
  while ( !clstrs.empty() )
  {
  	it = clstrs.begin();
    while ( it != clstrs.end() )
    {
// if a certain cluster has less than n points, we remove it from the list
      if (n == (**it).size)
      {
	    it = clstrs.erase(it);
		continue;
      }
// if the next point of a cluster has a higher rank than #20, then we remove the cluster from the list
      if ( (cnt == count) && (**it).points[n].rank >= out_points[count-1].rank )
      {
	    it = clstrs.erase(it);
		continue;
      }

// chech if the point is in rect
      if ( ( (**it).points[n].x < lx ) ||  ( (**it).points[n].x > hx ) || 
	    ( (**it).points[n].y < ly ) || ( (**it).points[n].y > hy ) )
	  {
	    it++;
		continue;
      }
	  
// determining the position of the new point
	  int32_t pos = cnt;

	  while( (pos > 0) && ( (**it).points[n].rank < out_points[pos-1].rank ) )
	    pos--;

	  if ( pos < count)
	  {  
	    int32_t j = (cnt < count) ? cnt : count - 1;
	    
        for ( ; j>pos ; j--)
          out_points[j] = out_points[j-1];  
	    
	    out_points[pos] = (**it).points[n];
	    
	    if ( cnt < count )
		  cnt++;  
      }
      
      it++;
    }
    n++;
	
  }
  
  return cnt;      
};

extern "C" DLLIMPORT SearchContext* __stdcall destroy(SearchContext* sc)
{
  delete[] context.points;
  for (int32_t i = 0; i < context.nrOfClusters; i++)
    delete[] context.clusters[i].points; 
  delete[] context.clusters;
  return nullptr;             
};

