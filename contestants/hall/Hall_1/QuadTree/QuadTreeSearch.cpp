// 
// QuadTree.cpp
// 01/03/2015
//
// Drew Hall
// andrew.danger.hall@gmail.com
//
// A single-threaded implementation using a QuadTree to organize the
// search space and facilitate range queries.
// 
#include "point_search.h"
#include "QuadTree.h"
#include <iostream>
#include <limits>


const int BIN_SIZE = 1024;


/* Declaration of the struct that is used as the context for the calls. */
struct SearchContext
{
  public:
    SearchContext(const Rect& bounds)
      : mTree(bounds)
    { 
    } 

  public:
    QuadTree<BIN_SIZE> mTree;
};


extern "C" {

  /* Load the provided points into an internal data structure. The pointers follow the STL iterator convention, where
  "points_begin" points to the first element, and "points_end" points to one past the last element. The input points are
  only guaranteed to be valid for the duration of the call. Return a pointer to the context that can be used for
  consecutive searches on the data. */
  __declspec(dllexport) SearchContext* __stdcall create(const Point* points_begin, const Point* points_end)
  {
    if (points_begin == points_end) {
      return nullptr;
    }

    // First, massage the data a little bit.
    
    // 1) Find bounds to start our QuadTree with:
    Rect bounds;
    bounds.lx = bounds.hx = points_begin->x;
    bounds.ly = bounds.hy = points_begin->y;

    for (const Point* p = points_begin; p != points_end; ++p) {
      if (p->x < bounds.lx) {
        bounds.lx = p->x;
      }
      else if (bounds.hx < p->x) {
        bounds.hx = p->x;
      }

      if (p->y < bounds.ly) {
        bounds.ly = p->y;
      }
      else if (bounds.hy < p->y) {
        bounds.hy = p->y;
      }
    }


    // 2) Copy & shuffle the data so that we can (hopefully)
    //    avoid pathological cases:
    std::vector<Point> dataCopy;
    std::copy(points_begin, points_end, std::back_inserter(dataCopy));
    std::random_shuffle(dataCopy.begin(), dataCopy.end());

    // 3) Now build our search context & insert all the points:
    SearchContext* sc = new SearchContext(bounds);

    for (const Point& pt : dataCopy) {
      sc->mTree.AddPoint(pt);
    }

    //std::cout << "\n\n";
    //sc->mTree.dump();
    //std::cout << "\n\n";

    return sc;
  }


  /* Search for "count" points with the smallest ranks inside "rect" and copy them ordered by smallest rank first in
  "out_points". Return the number of points copied. "out_points" points to a buffer owned by the caller that
  can hold "count" number of Points. */
  __declspec(dllexport) int32_t __stdcall search(SearchContext* sc, const Rect rect, const int32_t count, Point* out_points)
  {
    if (!sc) {
      return 0;
    }

    assert(rect.lx <= rect.hx);
    assert(rect.ly <= rect.hy);
 
    QuadTree<BIN_SIZE>::rank_vec nbest = sc->mTree.NBest(rect, count);

    std::copy(nbest.begin(), nbest.end(), out_points);

    return static_cast<int32_t>(nbest.size()); 
  }


  /* Release the resources associated with the context. Return nullptr if successful, "sc" otherwise. */
  __declspec(dllexport) SearchContext* __stdcall destroy(SearchContext* sc)
  {
    delete sc;
    return nullptr;
  }


} // extern "C"
