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
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>


// Performance tuning knobs:
const int BIN_SIZE = 1024;
const int NUM_THREADS = 4;
const float OUTLIER_R2 = 1e4 * 1e4;


const float inf = std::numeric_limits<float>::max();


struct SearchContext;



/* Declaration of the struct that is used as the context for the calls. */
struct SearchContext
{
  public:
    SearchContext()
      : outliers(&rank_less)
      , start_query(false)
      , finished_count(0)
      , stop_query(false)
      , shutdown(false)
    {
    } 

  public:
    std::vector<std::thread> query_threads;
    std::shared_ptr<QuadTree<BIN_SIZE>> trees[NUM_THREADS];
    std::shared_ptr<SortedVector<Point>> buffers[NUM_THREADS];
    SortedVector<Point> outliers;
    std::mutex init_mutex;

    // Query parameters:
    Rect query_area;
    int32_t query_n;
    std::mutex query_mutex;
    std::condition_variable start_cv;
    std::condition_variable finished_cv;
    std::condition_variable stop_cv;
    bool start_query;
    int finished_count;
    bool stop_query;
    bool shutdown;
};


void init_thread_func(int id, SearchContext* context, const Point* begin, const Point* end)
{
  // Offset the point iterator for this thread (we'll interleave our data):
  const Point* const myBegin = begin + id;

  // Find the bounds of the non-outlier points:
  Rect bounds;
  bounds.lx = inf; bounds.hx = -inf;
  bounds.ly = inf; bounds.hy = -inf;

  for (const Point* it = myBegin; it < end; it += NUM_THREADS) {
    const float x = it->x;
    const float y = it->y;
    const float r2 = x*x + y*y;

    if (r2 < OUTLIER_R2) {
      if (x < bounds.lx) {
        bounds.lx = x;
      }
      else if (bounds.hx < x) {
        bounds.hx = x;
      }
      if (y < bounds.ly) {
        bounds.ly = y;
      }
      else if (bounds.hy < y) {
        bounds.hy = y;
      }
    }
    else {
      // Point is an outlier.  We'll save it but avoid polluting our
      // bounds with a ridiculous value:
      std::unique_lock<std::mutex> outlierLock(context->init_mutex);
      context->outliers.insert(*it);
    }
  }

  // Now build the QuadTree for this thread and populate it:
  std::shared_ptr<QuadTree<BIN_SIZE>> tree(new QuadTree<BIN_SIZE>(bounds));

  for (const Point* it = myBegin; it < end; it += NUM_THREADS) {
    const float x = it->x;
    const float y = it->y;
    const float r2 = x*x + y*y;

    if (r2 < OUTLIER_R2) {
      tree->AddPoint(*it);
    }
    else {
      // Ignore this point.  We've already stashed it in the outliers.
    }
  }

  std::shared_ptr<SortedVector<Point>> buffer(new SortedVector<Point>(&rank_less));

  // Stash the tree & buffer in our search context:
  context->trees[id] = tree;
  context->buffers[id] = buffer;
}


void query_thread_func(int id, SearchContext* sc)
{
  while (true) {
    // Await next task:
    {
      std::unique_lock<std::mutex> start_lock(sc->query_mutex);
      while (!sc->start_query && !sc->shutdown) {
        sc->start_cv.wait(start_lock);
      }

      if (sc->shutdown) {
        std::cout << "Shutting down thread " << id << "\n";
        return;
      }

      std::cout << "Thread " << id << " received task.\n";
    }

    // Forward the query to our tree:
    SortedVector<Point>& myBuffer = *(sc->buffers[id]);
    myBuffer.clear();
    sc->trees[id]->NBest(sc->query_area, sc->query_n, myBuffer);

    // Announce that we're done:
    {
      std::unique_lock<std::mutex> finished_lock(sc->query_mutex);
      std::cout << "Thread " << id << " done with query.\n";
      ++sc->finished_count;
      sc->finished_cv.notify_all();
    }

    // Wait to be released for the next query:
    {
      std::unique_lock<std::mutex> stop_lock(sc->query_mutex);
      while (!sc->stop_query && sc->start_query) {
        std::cout << "Thread " << id << " waiting for release.\n";
        sc->stop_cv.wait(stop_lock);
      }
      std::cout << "Thread " << id << " released.\n";
    }
  }
}



extern "C" {

  /* Load the provided points into an internal data structure. The pointers follow the STL iterator convention, where
  "points_begin" points to the first element, and "points_end" points to one past the last element. The input points are
  only guaranteed to be valid for the duration of the call. Return a pointer to the context that can be used for
  consecutive searches on the data. */
  __declspec(dllexport) SearchContext* __stdcall create(const Point* points_begin, const Point* points_end)
  {  
    // Avoid degenerate case:
    if (points_begin == points_end) {
      return nullptr;
    }
    
    // Build our search context:
    SearchContext* sc = new SearchContext();

    // Now create the search trees for each thread in parallel:
    std::vector<std::thread> initThreads;

    for (int i = 0; i < NUM_THREADS; ++i) {
      initThreads.push_back(std::thread(init_thread_func, i, sc, points_begin, points_end));
    }

    // Wait for the init threads to complete:
    for (int i = 0; i < NUM_THREADS; ++i) {
      initThreads[i].join();
    }

    // Now create the query threads:
    for (int i = 0; i < NUM_THREADS; ++i) {
      sc->query_threads.push_back(std::thread(&query_thread_func, i, sc));
    }

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

    // Package up the query for the worker bees:
    {
      std::unique_lock<std::mutex> start_lock(sc->query_mutex);
      sc->query_area = rect;
      sc->query_n = count;
      sc->finished_count = 0;
      sc->stop_query = false;
      sc->start_query = true;

      std::cout << "Kicking off query [ " << rect.lx << ", " << rect.hx << " ] x [ " << rect.ly << ", " << rect.hy << " ]\n";

      // Kick off the worker bees:
      sc->start_cv.notify_all();
    }

    // Wait for them to complete
    {
      std::unique_lock<std::mutex> done_lock(sc->query_mutex);
      while (sc->finished_count < NUM_THREADS) {
        std::cout << "Waiting for results\n";
        sc->finished_cv.wait(done_lock);
      }
    }

    std::cout << "Merging results..." << std::flush;
    // Add the outliers to the mix (shove them in to buffer 0 if
    // they're in the search rectangle:
    for (const Point& p : sc->outliers) {
      if (contains(rect, p)) {
        sc->buffers[0]->insert(p);
      }
    }

    // Prepare to merge the subquery results (single threaded for now):
    SortedVector<Point>::const_iterator mergeIt[NUM_THREADS];

    // Figure out how many results we'll return:
    int32_t n = 0;
    for (int i = 0; i < NUM_THREADS; ++i) {
      n += static_cast<int32_t>(sc->buffers[i]->size());
      mergeIt[i] = sc->buffers[i]->begin();
    }

    n = std::min(count, n);

    // Now merge the output buffers:
    for (int i = 0; i < n; ++i) {
      int32_t min_rank = std::numeric_limits<int32_t>::max();
      int min_index = -1;

      for (int j = 0; j < NUM_THREADS; ++j) {
        if (mergeIt[j] != sc->buffers[j]->end()) {
          const Point& p_j = *mergeIt[j];
          if (p_j.rank < min_rank) {
            min_rank = p_j.rank;
            min_index = j;
          }
        }
      }

      assert(min_index >= 0);
      out_points[i] = *mergeIt[min_index];
      ++mergeIt[min_index];
    }

    std::cout << "done.\n";

    // Release the worker bees to await the next query:
    {
      std::unique_lock<std::mutex> stop_lock(sc->query_mutex);
      std::cout << "Releasing worker bees.\n";
      sc->stop_query = true;
      sc->start_query = false;
      sc->stop_cv.notify_all();
    }

    return n;
  }


  /* Release the resources associated with the context. Return nullptr if successful, "sc" otherwise. */
  __declspec(dllexport) SearchContext* __stdcall destroy(SearchContext* sc)
  {
    // Tell the worker bees to shut down:
    {
      std::unique_lock<std::mutex> shutdown_lock(sc->query_mutex);
      sc->shutdown = true;
      std::cout << "Shutting down worker bees.\n";
      sc->start_cv.notify_all();
    }

    // Now reap the worker bee threads:
    for (int i = 0; i < NUM_THREADS; ++i) {
      std::cout << "Reaping thread " << i << "..." << std::flush;
      sc->query_threads.back().join();
      sc->query_threads.pop_back();
      std::cout << "ok.\n";
    }

    // Dispose of the context:
    std::cout << "Deleting context..." << std::flush;
    delete sc;
    std::cout << "ok.\n";
    return nullptr;
  }

} // extern "C"
