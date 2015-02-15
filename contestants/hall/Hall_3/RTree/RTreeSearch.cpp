#include "point_search.h"
#include "RTree.h"
#include "SortedVector.h"
#include <algorithm>


// Performance tuning knobs:
const int BRANCH_SIZE = 64;
const int LEAF_SIZE = 64;



/* Declaration of the struct that is used as the context for the calls. */
struct SearchContext
{
  public:
    typedef RTree<BRANCH_SIZE, LEAF_SIZE> tree;

  public:
    SearchContext(const Point* points_begin, const Point* points_end)
      : mTree(points_begin, points_end)
      , mBuffer(&rank_less)
    {
    }

    tree mTree;
    SortedVector<Point> mBuffer;
};


extern "C" {

  /* Load the provided points into an internal data structure. The pointers follow the STL iterator convention, where
  "points_begin" points to the first element, and "points_end" points to one past the last element. The input points are
  only guaranteed to be valid for the duration of the call. Return a pointer to the context that can be used for
  consecutive searches on the data. */
  __declspec(dllexport) SearchContext* __stdcall create(const Point* points_begin, const Point* points_end)
  {
    SearchContext* sc = new SearchContext(points_begin, points_end);
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

    sc->mTree.NBest(rect, count, sc->mBuffer);
    std::copy(sc->mBuffer.begin(), sc->mBuffer.end(), out_points);

    return static_cast<int32_t>(sc->mBuffer.size());
  }


  /* Release the resources associated with the context. Return nullptr if successful, "sc" otherwise. */
  __declspec(dllexport) SearchContext* __stdcall destroy(SearchContext* sc)
  {
    delete sc;
    return nullptr;
  }

}