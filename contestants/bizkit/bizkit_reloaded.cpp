#include <stdint.h>
#include <algorithm>


typedef uint32_t u4;
typedef int32_t s4;


#define DLLIMPORT __declspec(dllexport)

#define NUMBER_OF_POINTS 10000000
#define RANK_ENDOFLIST 10000001  //must be larger than maximum rank
#define RANK_CELLEMPTY 10000002  //must be larger than RANK_ENDOFLIST
#define COUNT 20
#define STORE 300
#define TS_AREA_LIMIT 0.05
#define DYNAMIC_LIMIT_1 10
#define DYNAMIC_LIMIT_2 12
#define DYNAMIC_LIMIT_3 14
#define DYNAMIC_LIMIT_4 19



#define BOUND 5e9

#define DEPTH_L 14
#define DEPTH_l 2
#define DIVIDE_L 16384
#define DIVIDE_l 4

#define DEPTH_M 11
#define DEPTH_m 5
#define DIVIDE_M 2048
#define DIVIDE_m 32

#define DEPTH 8
#define DEPTH_S DEPTH
#define DEPTH_s DEPTH
#define DIVIDE 256  //2^DEPTH


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

/* Defines a rectangle, where a point (x,y) is mrg, if x is in [lx, hx] and y is in [ly, hy]. */
struct Rect
{
	float lx;
	float ly;
	float hx;
	float hy;
};
#pragma pack(pop)

class SearchContext 
{
  u4 nrOfPoints;

  float * xyValues;
  float * xy;
  int8_t * ids;
  
  bool dynamic_mode;
  u4 dynamic_limit_1, dynamic_limit_2, dynamic_limit_3, dynamic_limit_4;  
  
  u4** topRanks;
  u4** xStripsL;
  u4** xStripsM;
  u4** yStripsL;
  u4** yStripsM;
  
  u4 dyadicCovers[2*DIVIDE*DIVIDE];
  
  float xMin, xMax, yMin, yMax;
  float xStep, yStep, xStepInv, yStepInv, xLength, yLength, xLengthInv, yLengthInv;

  float lx,hx,ly,hy;
  float x_center, y_center, x_radius, y_radius;
  float x_side, y_side;
  float ratio;
  u4 strip_mode;
  u4 lx_L, hx_L, ly_L, hy_L;
  u4 lx_S, hx_S, ly_S, hy_S;
  
  
  u4 interval_x1, interval_x2, interval_y1, interval_y2;
  //u4 problemCount = 0;
  //std::string searchMode;

  s4 cnt;
  s4 cnt_ref;
  u4 checkStart_rank;
  u4* returnRanks;
  u4 returnRanks_ref[COUNT];
  u4** tl, ** tl_end, ** tl_minRank, ** il, **bl;
  u4 minRank;  
  
  u4* mrgLists[DIVIDE*DIVIDE];
  u4 *mrg1, *mrg2, *mrg3, *mrg4;
  u4* borderLists[DIVIDE*DIVIDE];
  u4 mrg_nr, border_nr, mrg_totalSize, border_totalSize; 
    
  u4* target;

      
  bool isContained(u4 rank) {
    xy = xyValues + (rank << 1);
//  return *xy >= lx && *xy <= hx && *(++xy) >= ly  && *xy <= hy; };
    return fabs( *xy - x_center ) <= x_radius && fabs (*(++xy) - y_center ) <= y_radius; }; 

    
  
  s4 get_x_index(u4 div, float x);  
  s4 get_y_index(u4 div, float y);  
 
  void updateReturnRanks(u4* r);
  void updateReturnRanks_nocheck(u4* r);
  
  u4** createCellLists (u4 xDiv, u4 yDiv);
  
  void adjust_list(u4** list);
  void adjust_lists();
  
  void getLists(u4 x_start, u4 x_end, u4 y_start, u4 y_end, u4 yDiv, u4** lists);
  inline void checkBorder() {tl_end=borderLists+border_nr; for(bl=borderLists; bl!=tl_end; bl++) updateReturnRanks(*bl);};


  void mergingSearch_init();
  bool mergingSearch_update();  
  bool mergingSearch();  
  bool mergingSearch_1();
  bool mergingSearch_2();
  bool mergingSearch_3();
  bool mergingSearch_4();
  void mergingSearch_dynamic();
  void mergingSearch_nocheck();
  void trivialSearch();
  void referenceSearch();
  bool stripSearch(u4 type, u4 bound);
  
  bool checkReturn();
  void returnPoints(Point* out_points) {
    u4* returnRanks_end = returnRanks + cnt;
    Point* out_point = out_points;
    for (target = returnRanks; target != returnRanks_end; target++)
    {
      u4 rank = *target;
      (*out_point).rank = rank;
      (*out_point).id = ids[rank];

//      xy = xyValues + 2*rank;
//      (*out_point).x = *xy;
//      (*out_point).y = *(++xy);

      out_point++;
    }
  }



public:
  SearchContext(const Point* points_begin, const Point* points_end);
  ~SearchContext() { if (nrOfPoints > 0) { delete[] xyValues; delete[] ids;
    delete[] topRanks; delete[] xStripsL; delete[] xStripsM; 
    delete[] yStripsL; delete[] yStripsM; delete[] returnRanks;} }
  s4 search(const int32_t count, Point* out_points);
  s4 search_compare();
  bool search_tryDyadic();
  bool setRect(const Rect rect); 
  void writeToFile(); 
  
//  std::string search_log;
//  u4 rect_count = 0;

};


bool SearchContext::setRect(const Rect rect)
{
  if ( nrOfPoints == 0 ) return false;

  lx = rect.lx < xMin ? xMin : rect.lx;
  hx = rect.hx > xMax ? xMax : rect.hx;
  ly = rect.ly < yMin ? yMin : rect.ly;
  hy = rect.hy > yMax ? yMax : rect.hy;

  if ( lx > xMax || hx < xMin || ly > yMax || hy < yMin ) 
    return false;
  
//  if ( hx <= lx || hy <= ly ) return false;

  x_center = 0.5 * (rect.lx + rect.hx);
  x_radius = rect.hx - x_center;
  y_center = 0.5 * (rect.ly + rect.hy);
  y_radius = rect.hy - y_center;

  x_side = (hx - lx) * xLengthInv;
  y_side = (hy - ly) * yLengthInv;

  ratio = x_side / y_side;

//  strip_mode = 0;
  if (ratio > 1)
  {
//  	if (ratio > 8.)
  	  if (ratio > 512.) strip_mode = 3;
  	  else strip_mode = 4;

  }
  else 	
  { 	
//  	if (ratio*8 < 1.)
  	  if (ratio*512 < 1.) strip_mode = 1;
  	  else strip_mode = 2;
  }
    
  lx_L = (u4) ( (lx-xMin) * xLengthInv * DIVIDE_L);
  hx_L = (u4) ( (hx-xMin) * xLengthInv * DIVIDE_L);
  ly_L = (u4) ( (ly-yMin) * yLengthInv * DIVIDE_L);
  hy_L = (u4) ( (hy-yMin) * yLengthInv * DIVIDE_L);
  
  if ( hx_L >= DIVIDE_L ) hx_L = DIVIDE_L - 1;
  if ( hy_L >= DIVIDE_L ) hy_L = DIVIDE_L - 1;
  
  lx_S = lx_L >> (DEPTH_L - DEPTH_S);
  hx_S = hx_L >> (DEPTH_L - DEPTH_S);
  ly_S = ly_L >> (DEPTH_L - DEPTH_S);
  hy_S = hy_L >> (DEPTH_L - DEPTH_S);
    
  return true;
}

void SearchContext::adjust_list(u4** list)
{
  u4* lower = *list;
  u4* upper = lower + lower[-1];
  u4* midpoint;
  while (lower != upper)
  {
    midpoint = lower + ( (upper-lower) >> 1 );	
  	if ( checkStart_rank < *midpoint ) 
  	  upper = midpoint;
  	else
	  lower = midpoint+1;  
  }  
  *list = lower;
//  std::cout << (*lower) << " (previous: " << lower[-1] << ")\n"; 
}

void SearchContext::adjust_lists()
{
  tl_end = mrgLists + mrg_nr;
  for (tl = mrgLists; tl != tl_end; tl++)
    adjust_list(tl);
		
  tl_end = borderLists + border_nr;
  for (tl = borderLists; tl != tl_end; tl++)
    adjust_list(tl);
}
	
	


s4 SearchContext::get_x_index(u4 div, float x)
{
  s4 x_i = (s4)( div*xLengthInv*(x-xMin) );
  if (x_i < 0) x_i = 0;
  if (x_i >= div) x_i = div-1;
  return x_i;
}

s4 SearchContext::get_y_index(u4 div, float y)
{
  s4 y_i = (s4)( div*yLengthInv*(y-yMin) );
  if (y_i < 0) y_i = 0;
  if (y_i >= div) y_i = div-1;
  return y_i;
}

bool findDyadicCover( u4 l, u4 h, u4* interval1, u4* interval2)
{
  if ( h <= l+1 )
  {
    *interval1 = DIVIDE + l;
	*interval2 = 0;	  	
    return true;
  }
  
  u4 cover_l = 0;
  u4 cover_h = DIVIDE;
  u4 cover_length = DIVIDE;
  u4 cover_middle;
  
  while (true)
  {
  	cover_length = cover_length >> 1;
    cover_middle = cover_l+cover_length;
  	if ( h <= cover_middle )
  	  cover_h = cover_middle;
  	else if ( l>= cover_middle )
  	  cover_l = cover_middle;
  	else 
  	  break;  	
  }
  
  u4 len1 = cover_middle - l;
  u4 k1 = 1;
  while ( len1 > k1 )
    k1 = k1 << 1;

  u4 len2 = h - cover_middle;
  u4 k2 = 1;
  while ( len2 > k2 )
    k2 = k2 << 1;
   
  if ( (k1 == cover_length) && (k2 == cover_length) )
  {
    *interval1 = ( DIVIDE + cover_l )/cover_length/2;
	*interval2 = 0;	
  	return true;
  }
  else
  {
    *interval1 = (DIVIDE + cover_middle - k1) / k1;
	*interval2 = (DIVIDE + cover_middle) / k2;;	
   	return false;  
  }
}

void mergeTopRanks(u4* tr1, u4* tr2, u4* merged_tr, u4 store, bool child_info)
{
  u4* r1 = tr1;
  u4* r2 = tr2;
  u4* merged_tr_end = merged_tr + store;
  
  for (u4* r = merged_tr; r != merged_tr_end; r++)
    if ( *r1 <= *r2 )
      *r = *(r1++);
  	else 
  	  *r = *(r2++);		  

  if ( child_info )
  {
    merged_tr_end[2] = r1-tr1;
    merged_tr_end[4] = r2-tr2;
  }
}

void SearchContext::getLists(u4 x_start, u4 x_end, u4 y_start, u4 y_end, u4 yDiv, u4** lists)
{
  u4** l;
  mrg_totalSize = border_totalSize = 0;
  il = mrgLists;
  bl = borderLists;
  for (u4 xx = x_start; xx <= x_end; xx++)
  {
    l = lists + xx*yDiv+y_start;
    for (u4 yy = y_start; yy <= y_end; yy++, l++)
    {  
	  if ( (*l)[-1] != 0 )
	  { 
        if ( xx == x_start || xx == x_end || yy == y_start || yy == y_end ) 
	      {*(bl++) = *l; border_totalSize += (*l)[-1];}
	    else 
          {*(il++) = *l; mrg_totalSize += (*l)[-1];}
      }
    }
  }
  mrg_nr = il - mrgLists;
  border_nr = bl - borderLists;
}

void SearchContext::updateReturnRanks(u4* r)
{
  u4 rank;
  bool hasReachedCount = ( cnt == COUNT);

// erre gyorsabbat irni hasznalva r[0]-t
  while ( *r < checkStart_rank ) 
    r++;
  
  for (; (rank=*r) != RANK_CELLEMPTY ; r++)
  {	
  	if ( hasReachedCount && returnRanks[COUNT - 1] <= rank )
  	  return;
    
    if ( ! isContained(rank) )
      continue;
 		
  	uint32_t pos = cnt;
  	while ( (pos > 0 ) && ( returnRanks[pos - 1] > rank ) )
  	  pos--;
	
	for( uint32_t i = cnt; i > pos; i-- )
	  returnRanks[i]= returnRanks[i-1];

	returnRanks[pos] = rank;
	
	if ( !hasReachedCount ) 
	{
      cnt++;	 
      hasReachedCount = ( cnt == COUNT);
    }
  }
  return;
}

void SearchContext::updateReturnRanks_nocheck(u4* r)
{
  u4 rank;
  bool hasReachedCount = ( cnt == COUNT);
  
// erre gyorsabbat irni hasznalva r[0]-t
  while ( *r < checkStart_rank ) 
    r++;
  
  for (; (rank=*r) != RANK_CELLEMPTY ; r++)
  {	
  	if ( hasReachedCount && returnRanks[COUNT - 1] <= rank )
  	  return;
    
//    if ( ! isContained(rank) ) continue;
 		
  	uint32_t pos = cnt;
  	while ( (pos > 0 ) && ( returnRanks[pos - 1] > rank ) )
  	  pos--;
	
	for( uint32_t i = cnt; i > pos; i-- )
	  returnRanks[i]= returnRanks[i-1];

	returnRanks[pos] = rank;
	
	if ( !hasReachedCount ) 
	{
      cnt++;	 
      hasReachedCount = ( cnt == COUNT);
    }
  }
  return;
}


u4** SearchContext::createCellLists (u4 xDiv, u4 yDiv)
{
  u4 nrOfCells = xDiv*yDiv;
  u4* cellSize = new u4[nrOfCells];
  u4** topRanks = new u4*[nrOfCells];
  	
  for (int j=0; j<nrOfCells; j++)
    cellSize[j] = 0;

  float * xyValues_end = xyValues + 2*nrOfPoints;

  for( xy = xyValues; xy != xyValues_end ; )
  {
  	float x = *(xy++);
  	float y = *(xy++);

  	if ( ( x > BOUND ) || ( x < -BOUND ) || ( y > BOUND ) || ( y < -BOUND ) ) continue;
  	
  	int x_i = get_x_index(xDiv, x);
  	int y_i = get_y_index(yDiv, y);

    cellSize[ x_i * yDiv + y_i ] ++;  	
  }


//  std::cout << "creating lists for cells...\n";
  
  u4 cell_id;
    
  for (cell_id = 0; cell_id < nrOfCells; cell_id++)
  {
    u4 * tr = new u4[ cellSize[ cell_id ] + 2 ];
    tr[0] = cellSize[ cell_id ];
    tr[ cellSize[ cell_id ] + 1 ] = RANK_CELLEMPTY;
    topRanks[ cell_id ] = tr+1;
    cellSize[ cell_id ] = 0;      
  }

//  std::cout << "filling up cells...\n";

  xy = xyValues;
  
  for(u4 rank=0 ; rank < nrOfPoints ; rank++ )
  {
  	float x = *(xy++);
  	float y = *(xy++);
  	
  	if ( ( x > BOUND ) || ( x < -BOUND ) || ( y > BOUND ) || ( y < -BOUND ) ) continue;
  	  
  	int x_i = get_x_index(xDiv, x);
  	int y_i = get_y_index(yDiv, y);

    cell_id = x_i * yDiv + y_i;  	
    topRanks[ cell_id ][ cellSize[ cell_id ] ] = rank;
 	cellSize[ cell_id ] ++;	  	
  }

//  std::cout << "sorting cells...\n";

  cell_id = 0;
    
  for (u4 xx = 0; xx < xDiv; xx++)
  {
  	for (u4 yy = 0; yy < yDiv; yy++)
  	{
      std::sort( topRanks[cell_id], topRanks[cell_id] + cellSize[ cell_id ] );
      cell_id++;
    }
  }

  delete[] cellSize;

  return topRanks;	
}

SearchContext::SearchContext(const Point* points_begin, const Point* points_end)
{
  nrOfPoints = points_end - points_begin;

  u4 nrOfCells = DIVIDE*DIVIDE;  
  xyValues = new float[ 2*nrOfPoints ];
  ids = new int8_t[ 2*nrOfPoints ];
  topRanks = new u4*[ 4*nrOfCells ];

  xMin = BOUND;
  xMax = -BOUND;
  yMin = BOUND;
  yMax = -BOUND;
      
//  std::cout << "reading points...\n";
  	  
  for (const Point * in_point = points_begin; in_point != points_end; in_point++)
  {
    u4 rank = (*(in_point)).rank;
    float x = (*(in_point)).x;
    float y = (*(in_point)).y;
    
    xyValues[2*rank] = x;
    xyValues[2*rank+1] = y;
    ids[rank] = (*(in_point)).id;
  	  
    if ( x < xMin && x > -BOUND ) xMin = x;
    if ( x > xMax && x < BOUND ) xMax = x;
    if ( y < yMin && y > -BOUND ) yMin = y;
    if ( y > yMax && y < BOUND ) yMax = y;      
  }


  xStep = (xMax - xMin) / DIVIDE;
  yStep = (yMax - yMin) / DIVIDE;
  xStepInv = 1. / xStep;
  yStepInv = 1. / yStep;   

  xLength = xMax-xMin;
  yLength = yMax-yMin;  
  xLengthInv = 1. / (xMax-xMin);
  yLengthInv = 1. / (yMax-yMin);

  if ( nrOfPoints == 0 ) return;

  

  u4** cells_S = createCellLists( DIVIDE, DIVIDE );


//  std::cout << "merging cells...\n";

  u4 cell_id, child1, child2;
    
  for (u4 xx = 2*DIVIDE - 1; xx > 0; xx--)
  {
  	for (u4 yy = 2*DIVIDE - 1; yy > 0; yy--)
  	{
      cell_id = ( xx << (DEPTH+1) ) + yy;

      if ( yy < DIVIDE)
      {
      	child1 = cell_id+yy;
      	child2 = cell_id+yy+1;
      }
      else if (xx < DIVIDE)
      {
        child1 = cell_id + ( xx << (DEPTH+1) );
		child2 = cell_id + ( (xx+1) << (DEPTH+1) );
      }
      else 
      {
      	topRanks[cell_id] = cells_S[ ( (xx-DIVIDE) << DEPTH ) + yy - DIVIDE ];
        continue;	      	
      }
       
//	  std::cout << "merging cells #" << child1 << " and #" << child2;
	    
      u4 totalSize = topRanks[child1][-1] + topRanks[child2][-1];

      u4* tr;
      
      u4 store = STORE;

//      if ( (xx==1) && (yy==1) )store = 0;

/*      
	  u4 x_depth = 0, y_depth = 0;
	  
	  for (u4 n = xx; n != 1; n = n >> 1) x_depth++;
		
	  for (u4 n = yy; n != 1; n = n >> 1) y_depth++;
		  
	  store = 300 + 50 * (2*DEPTH - x_depth - y_depth);  
*/

//      std::cout << " of sizes " << cellSize[child1] << " and " << cellSize[child2] << "\n";
      

//	  if ( (xx >= DIVIDE) || (yy >= DIVIDE) || (totalSize <= store) )

      bool child_info = false;
	  if ( totalSize <= store )
	  {
        tr = new u4[totalSize+2];
        tr[0] = totalSize; 
        tr[totalSize+1] = RANK_CELLEMPTY;
      }
      else
	  {
/*        tr = new u4[store+2];
        tr[0] = totalSize; 
        tr[store+1] = RANK_ENDOFLIST;
		totalSize = store; 	 */ 
        tr = new u4[store+6];
        tr[0] = totalSize; 
		totalSize = store; 	  
        tr[store+1] = RANK_ENDOFLIST;
        tr[store+2] = child1;
        tr[store+4] = child2;
		child_info = true;        
      }

      tr++;
      mergeTopRanks( topRanks[ child1 ], topRanks[ child2 ], tr , totalSize, child_info);
      topRanks[cell_id] = tr;

//      printPoints( tp, totalSize);

    }
//  std::cout << "xx=" << xx << " done\n";

  }
  
  delete[] cells_S;
  
  xStripsL = createCellLists( 16384, 4);
  yStripsL = createCellLists( 4, 16384);
  xStripsM = createCellLists( 2048, 32);
  yStripsM = createCellLists( 32, 2048);
  
  returnRanks = new u4[COUNT+1];
  
  dynamic_mode = false;
  
  
  u4* cover = dyadicCovers;
  for (u4 xx=0; xx<DIVIDE; xx++)
    for (u4 yy=0; yy<DIVIDE; yy++)
    {
      findDyadicCover(xx,yy+1,cover,cover+1);
      cover+=2;
    }
  
  
}
  

void SearchContext::mergingSearch_init()
{
  interval_x1 = dyadicCovers[ 2*(lx_S * DIVIDE + hx_S) ];
  interval_x2 = dyadicCovers[ 2*(lx_S * DIVIDE + hx_S)+1 ];
  interval_y1 = dyadicCovers[ 2*(ly_S * DIVIDE + hy_S) ];
  interval_y2 = dyadicCovers[ 2*(ly_S * DIVIDE + hy_S)+1 ];
  
//  bool xOne = findDyadicCover( lx_S, hx_S+1, &interval_x1, &interval_x2);
//  bool yOne = findDyadicCover( ly_S, hy_S+1, &interval_y1, &interval_y2);
  bool xOne = ( interval_x2 == 0 );
  bool yOne = ( interval_y2 == 0 );
  
   
  mrg1 = topRanks[( interval_x1 << (DEPTH+1) ) + interval_y1];
  mrg_nr = 1;

  if ( xOne )
  {
    if (!yOne)
    {
      mrg2 = topRanks[( interval_x1 << (DEPTH+1) ) + interval_y2];
      mrg_nr = 2;  
    }
  }
  else if ( yOne )
  {
    mrg2 = topRanks[( interval_x2 << (DEPTH+1) ) + interval_y1];
    mrg_nr = 2;  
  }
  else 
  {
    mrg2 = topRanks[( interval_x1 << (DEPTH+1) ) + interval_y2];
    mrg3 = topRanks[( interval_x2 << (DEPTH+1) ) + interval_y1];
    mrg4 = topRanks[( interval_x2 << (DEPTH+1) ) + interval_y2];
    mrg_nr = 4;  
  }	
}

bool SearchContext::mergingSearch_update()
{
//  if ( cnt >= COUNT ) std::cout << "\n\n\nAJJAJ\n\n\n";	
	
  if ( !dynamic_mode ) return false;	
	
  if (mrg_nr==2)
  {
    if ( cnt < dynamic_limit_2 ) return false;

    if ( (*mrg1) == RANK_ENDOFLIST)
	{
    u4 id1 = mrg1[1];
    u4 pos1 = mrg1[2];
    u4 id2 = mrg1[3];
    u4 pos2 = mrg1[4];
	    
    mrg1 = topRanks[id1]+pos1;
    mrg3 = topRanks[id2]+pos2;    
    }
  	else 
  	{
    u4 id1 = mrg2[1];
    u4 pos1 = mrg2[2];
    u4 id2 = mrg2[3];
    u4 pos2 = mrg2[4];
	    
    mrg2 = topRanks[id1]+pos1;
    mrg3 = topRanks[id2]+pos2;
    }  	
    
	mrg_nr++;	
    return mergingSearch_3();
  }
  
  if (mrg_nr==3)
  {
    if ( cnt < dynamic_limit_3 ) return false;

    if ( (*mrg1) == RANK_ENDOFLIST)
	{
    u4 id1 = mrg1[1];
    u4 pos1 = mrg1[2];
    u4 id2 = mrg1[3];
    u4 pos2 = mrg1[4];
	    
    mrg1 = topRanks[id1]+pos1;
    mrg4 = topRanks[id2]+pos2;    
    }
  	else if ( (*mrg2) == RANK_ENDOFLIST)
	{
    u4 id1 = mrg2[1];
    u4 pos1 = mrg2[2];
    u4 id2 = mrg2[3];
    u4 pos2 = mrg2[4];
	    
    mrg2 = topRanks[id1]+pos1;
    mrg4 = topRanks[id2]+pos2;
    }
    else 
  	{
    u4 id1 = mrg3[1];
    u4 pos1 = mrg3[2];
    u4 id2 = mrg3[3];
    u4 pos2 = mrg3[4];
	    
    mrg3 = topRanks[id1]+pos1;
    mrg4 = topRanks[id2]+pos2;    
    }  	
    
	mrg_nr++;	
    return mergingSearch_4();
  }

  if (mrg_nr==1)
  {
    if ( cnt < dynamic_limit_1 ) return false;

    u4 id1 = mrg1[1];
    u4 pos1 = mrg1[2];
    u4 id2 = mrg1[3];
    u4 pos2 = mrg1[4];
	    
    mrg1 = topRanks[id1]+pos1;
    mrg2 = topRanks[id2]+pos2;
    
    mrg_nr++;
    return mergingSearch_2();
  }

  if ( cnt < dynamic_limit_4 ) return false;

  mrgLists[0] = mrg1;
  mrgLists[1] = mrg2;
  mrgLists[2] = mrg3;
  mrgLists[3] = mrg4;
  
  mergingSearch_dynamic();
  return true;
}

bool SearchContext::mergingSearch()
{
  if (mrg_nr==4) return mergingSearch_4();	
  if (mrg_nr==2) return mergingSearch_2();	
  if (mrg_nr==1) return mergingSearch_1();	
  if (mrg_nr==3) return mergingSearch_3();
  
  mergingSearch_dynamic();
  return true;
}

bool SearchContext::mergingSearch_1()
{  
  target = returnRanks+cnt;
  u4 rank; 
   
  for ( ; ;mrg1++)
  {     
    rank = *mrg1;
    
    if ( rank == RANK_CELLEMPTY )
      return true;

    if ( rank == RANK_ENDOFLIST )
    {
      checkStart_rank = *(mrg1-1) + 1;
      return mergingSearch_update();
    }
    
    if ( !isContained(rank) )
      continue;
  
    *target = rank;
    target++;
    cnt++;
    
    if (cnt == COUNT)
      return true;
  }

  return true;
}

bool SearchContext::mergingSearch_2()
{  
  target = returnRanks+cnt;
  
  u4 rank, rank1, rank2;
   
  while ( true )
  {      
    rank1 = *mrg1;
    rank2 = *mrg2;
    
    if ( ( rank1 == RANK_ENDOFLIST ) || ( rank2 == RANK_ENDOFLIST ) )
    {
      checkStart_rank = rank+1;
      return mergingSearch_update();
    }
        
    if ( rank1 < rank2 )
    {
	  rank = rank1;
	  mrg1++;
    }
    else
    {
	  rank = rank2;
	  mrg2++;
    }
      
    if ( rank == RANK_CELLEMPTY )
      return true;
      
	if ( !isContained(rank) )
	  continue;    
  
    *target = rank;
    target++;
    cnt++;
    
    if (cnt == COUNT)
      return true;
  }
  

  return true;	  	
}

bool SearchContext::mergingSearch_3()
{
  target = returnRanks+cnt;
  
  u4 rank, rank1, rank2, rank3;

  while ( true )
  {      
    rank1 = *mrg1;
    rank2 = *mrg2;
    rank3 = *mrg3;
    
    if ( rank1 == RANK_ENDOFLIST || rank2 == RANK_ENDOFLIST || rank3 == RANK_ENDOFLIST  )
    {
      checkStart_rank = rank+1;
      return mergingSearch_update();
    }
        
    if ( rank1 < rank2 )
    {
	  if (rank1 < rank3) {rank = rank1; mrg1++;}
      else {rank = rank3; mrg3++;}
    }
    else 
    {
	  if (rank2 < rank3) {rank = rank2; mrg2++;}
      else {rank = rank3; mrg3++;}
    }
       
    if ( rank == RANK_CELLEMPTY )
      return true;
      
	if ( !isContained(rank) )
	  continue;    
  
    *target = rank;
    target++;
    cnt++;
    
    if (cnt == COUNT)
      return true;
  
  }
  
  return true;	  	
}

bool SearchContext::mergingSearch_4()
{
  target = returnRanks+cnt;
  
  u4 rank, rank1, rank2, rank3, rank4;

  while ( true )
  {      
    rank1 = *mrg1;
    rank2 = *mrg2;
    rank3 = *mrg3;
    rank4 = *mrg4;
    
    if ( ( rank1 == RANK_ENDOFLIST ) || ( rank2 == RANK_ENDOFLIST ) || 
	  ( rank3 == RANK_ENDOFLIST ) || ( rank4 == RANK_ENDOFLIST ) )
    {
      checkStart_rank = rank+1;
      return mergingSearch_update();
    }
        
    if ( rank1 < rank2 )
    {
      if (rank3 < rank4 ) 
	    if (rank1 < rank3) {rank = rank1; mrg1++;}
        else {rank = rank3; mrg3++;}
      else if (rank1 < rank4) {rank = rank1; mrg1++;}
      else {rank = rank4; mrg4++;}
    }
    else 
    {
      if (rank3 < rank4 ) 
	    if (rank2 < rank3) {rank = rank2; mrg2++;}
        else {rank = rank3; mrg3++;}
      else if (rank2 < rank4) {rank = rank2; mrg2++;}
      else {rank = rank4; mrg4++;}
    }
       
    if ( rank == RANK_CELLEMPTY )
      return true;
      
	if ( !isContained(rank) )
	  continue;    
  
    *target = rank;
    target++;
    cnt++;
    
    if (cnt == COUNT)
      return true;
  
  }
  
  return true;	  	
}

void SearchContext::mergingSearch_dynamic()
{
  target = returnRanks + cnt;
  
  tl_end = mrgLists + mrg_nr;
  u4 rank; 
       
  while ( true )
  { 
     
    minRank = RANK_CELLEMPTY;  
    for (tl = mrgLists; tl != tl_end; tl++) 
    {
	  rank = **tl;
	  
	  if ( rank == RANK_ENDOFLIST )
	  {  
 		u4* child_info = *tl;
	    u4 id1 = child_info[1];
	    u4 pos1 = child_info[2];
	    u4 id2 = child_info[3];
	    u4 pos2 = child_info[4];
	    
	    *tl = topRanks[id1]+pos1;
	    *tl_end = topRanks[id2]+pos2;
	    tl_end++;
	    mrg_nr++;  // KITOROLHETO
	    tl--;
	    continue;
      }

	  if ( rank < minRank )
	  {
      	minRank = rank;
      	tl_minRank = tl;
      }
    }

    (*tl_minRank)++;
    
    if ( minRank == RANK_CELLEMPTY ) return;
    
    if ( ! isContained(minRank) ) continue;
  
    *(target++) = minRank;
    cnt++;
    
    if (cnt == COUNT) return;

  }
  

}

void SearchContext::mergingSearch_nocheck()
{
  tl_end = mrgLists + mrg_nr;
    
  target = returnRanks+cnt;
   
  while ( true )
  {    
    minRank = RANK_CELLEMPTY;  
    for (tl = mrgLists; tl != tl_end; tl++) 
      if ( **tl < minRank )
      	minRank = **(tl_minRank = tl);
      
    (*tl_minRank)++;
    
    if ( minRank == RANK_CELLEMPTY ) return;
    
//    if ( ! isIncluded(minRank) ) continue;
  
    *(target++) = minRank;
    cnt++;
    
    if (cnt == COUNT) return;

  }
  
}


void SearchContext::trivialSearch()
{
  target = returnRanks + cnt;

  for ( u4 rank = checkStart_rank; rank < NUMBER_OF_POINTS; rank++ )
    if ( isContained( rank ) )
	{   
      *target = rank;
	  target++;
	  cnt++;
	  
	  if (cnt == COUNT)
	    return;	  
    }
  return;	
}

void SearchContext::referenceSearch()
{
  cnt_ref = 0;
  target = returnRanks_ref;

  for ( u4 rank = 0; rank < nrOfPoints; rank++ )
    if ( isContained( rank ) )
	{   
      *(target++) = rank;
	  cnt_ref++;
	  
	  if (cnt_ref == COUNT)
	    return;	  
    }
  return;	
}

bool SearchContext::stripSearch(u4 type, u4 bound)
{

switch (type)
{
case 1:
//  searchMode = "x-strips L";		
  getLists( lx_L, hx_L, ly_S >> ( DEPTH_S - DEPTH_l), 
	hy_S >> ( DEPTH_S - DEPTH_l), DIVIDE_l, xStripsL );


  if (mrg_nr > 0) {mergingSearch_nocheck(); checkBorder(); return true;}
  else if (border_totalSize <= bound) {checkBorder(); return true;}  
  else return false;   	
    	
  break;
  
case 2:  
  
//  searchMode = "x-strips M";		

  getLists( lx_L >> ( DEPTH_L - DEPTH_M), hx_L >> ( DEPTH_L - DEPTH_M), 
  ly_S >> ( DEPTH_S - DEPTH_m), hy_S >> ( DEPTH_S - DEPTH_m), DIVIDE_m, xStripsM );


  if (mrg_nr > 0) {mergingSearch_nocheck(); checkBorder(); return true;}
  else if (border_totalSize <= bound) {checkBorder(); return true;}  
  else return false;

  break;
  
case 3:
	
//  searchMode = "y-strips L";		

  getLists( lx_S >> ( DEPTH_S - DEPTH_l), hx_S >> ( DEPTH_S - DEPTH_l), 
    ly_L, hy_L, DIVIDE_L, yStripsL );

  if (mrg_nr > 0) {mergingSearch_nocheck(); checkBorder(); return true;}
  else if (border_totalSize <= bound) {checkBorder(); return true;}  
  else return false;   	

  break;

case 4:

//  searchMode = "y-strips M";	

  getLists( lx_S >> ( DEPTH_S - DEPTH_m), hx_S >> ( DEPTH_S - DEPTH_m), 
    ly_L >> ( DEPTH_L - DEPTH_M), hy_L >> ( DEPTH_L - DEPTH_M), DIVIDE_M, yStripsM );

  if (mrg_nr > 0) {mergingSearch_nocheck(); checkBorder(); return true;}
  else if (border_totalSize <= bound) {checkBorder(); return true;}  
  else return false;   	

  break;

default:
  return false;  
  
}
}

bool SearchContext::checkReturn()
{
  if ( cnt != cnt_ref )	
    return false;
  for (u4 p = 0; p<cnt; p++)
    if ( returnRanks[p] != returnRanks_ref[p] )
	  return false;  
  return true;	
}


s4 SearchContext::search(const int32_t count, Point* out_points)
{
  cnt = 0;
  checkStart_rank = 0;

if ( x_side*y_side*131072 < 1 )
  if ( stripSearch(strip_mode, 512) ) { returnPoints(out_points); return cnt; }
	
if ( x_side*32768 < 1. ) 
{
//  searchMode = "x-strips L";		
  if ( stripSearch(1,512) ) { returnPoints(out_points); return cnt; }
}

if ( y_side*32768 < 1. )
{
//  searchMode = "y-strips L";		
  if ( stripSearch(3,512) ) { returnPoints(out_points); return cnt; }

}


  cnt = 0; checkStart_rank = 0;

  dynamic_mode = true;

  dynamic_limit_1 = DYNAMIC_LIMIT_1;
  dynamic_limit_2 = DYNAMIC_LIMIT_2;
  dynamic_limit_3 = DYNAMIC_LIMIT_3;
  dynamic_limit_4 = DYNAMIC_LIMIT_4;
  
  mergingSearch_init();
  if ( mergingSearch() ) {returnPoints(out_points); return cnt;}
  
//  std::cout << cnt << " " << checkStart_rank << "\n";
  
  if ( x_side * y_side > TS_AREA_LIMIT )
  {
//    std::cout << "HOPPA: ";
    trivialSearch();
    returnPoints(out_points); 
    return cnt;  
  }

  
  cnt = 0;
  checkStart_rank = 0;

  stripSearch(strip_mode, nrOfPoints);

  returnPoints(out_points);   
  return cnt;

};

extern "C" DLLIMPORT SearchContext* __stdcall create(const Point* points_begin, const Point* points_end)
{
  SearchContext* sc = new SearchContext (points_begin, points_end); 
      
  return sc;              
};

extern "C" DLLIMPORT int32_t __stdcall search(SearchContext* sc, const Rect rect, const int32_t count, Point* out_points)
{
  


  s4 cnt = 0;

  if ( (*sc).setRect(rect) ) 
    cnt = (*sc).search(count, out_points);

  return cnt;  

};

extern "C" DLLIMPORT SearchContext* __stdcall destroy(SearchContext* sc)
{
  delete sc;
  return nullptr;
};


