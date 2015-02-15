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
#include "SharedQueue.h"
#include <iostream>
#include <limits>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>


// Performance tuning knobs:
const int BIN_SIZE = 2048;
const int QUERY_THREADS = 4;
const int MERGE_THREADS = 2;
const float OUTLIER_R2 = 1e4 * 1e4;

const float inf = std::numeric_limits<float>::max();


struct query_task
{
  public:
    query_task()
      : id(-1)
      , shutdown(true)
      , query_n(-1)
    {
    }

    query_task(int tid, bool done, const Rect& rect, int n)
      : id(tid)
      , shutdown(done)
      , query_rect(rect)
      , query_n(n)
    {
    }

  int id;
  int query_n;
  Rect query_rect;
  bool shutdown;
};


struct merge_task
{
  public:
    merge_task()
      : shutdown(true)
      , query_n(-1)
      , lhs_id(-1)
      , rhs_id(-1)
      , out_id(-1)
    {
    }

    merge_task(bool done, int n, int id_lhs, int id_rhs, int id_out)
      : shutdown(done)
      , query_n(n)
      , lhs_id(id_lhs)
      , rhs_id(id_rhs)
      , out_id(id_out)
    {
    }

  bool shutdown;
  int query_n;
  int lhs_id;
  int rhs_id;
  int out_id;
};


struct SearchContext;


/* Declaration of the struct that is used as the context for the calls. */
struct SearchContext
{
  public:
    SearchContext()
      : outliers(&rank_less)
    {
    } 

  public:
    std::shared_ptr<QuadTree<BIN_SIZE>> trees[QUERY_THREADS];
    std::vector<SortedVector<Point>> buffers;
    SortedVector<Point> outliers;
    std::mutex init_mutex;

    typedef std::shared_ptr<query_task> query_ptr;
    std::vector<std::thread> query_threads;
    SharedQueue<query_ptr> todo_queries;
    SharedQueue<query_ptr> done_queries;

    typedef std::shared_ptr<merge_task> merge_ptr;
    std::vector<std::thread> merge_threads;
    SharedQueue<merge_ptr> todo_merges;
    SharedQueue<merge_ptr> done_merges;
};


void init_thread_func(int id, SearchContext* context, const Point* begin, const Point* end)
{
  // Offset the point iterator for this thread (we'll interleave our data):
  const Point* const myBegin = begin + id;

  // Find the bounds of the non-outlier points:
  Rect bounds;
  bounds.lx = inf; bounds.hx = -inf;
  bounds.ly = inf; bounds.hy = -inf;

  for (const Point* it = myBegin; it < end; it += QUERY_THREADS) {
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

  for (const Point* it = myBegin; it < end; it += QUERY_THREADS) {
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
  
  // Stash the tree & buffer in our search context:
  context->trees[id] = tree;
}


void query_thread_func(SearchContext* sc)
{
  while (true) {
    // Take a task when it becomes available:
    SearchContext::query_ptr task = sc->todo_queries.pop();

    const query_task& t = *task;

    if (t.shutdown) {
      return;
    }

    // We've got an actual query to execute:
    sc->trees[t.id]->NBest(t.query_rect, t.query_n, sc->buffers[t.id]);

    // Signal that we're done with this query:
    sc->done_queries.push(task);
  }
}


void merge_thread_func(SearchContext* sc)
{
  while (true) {
    // Take a task when it becomes available:
    SearchContext::merge_ptr task = sc->todo_merges.pop();
    const merge_task& t = *task;

    if (t.shutdown) {
      return;
    }

    // We've got an actual merge to execute:
    sc->buffers[t.out_id].merge(sc->buffers[t.lhs_id], sc->buffers[t.rhs_id], t.query_n);

    // Signal that the result is available:
    sc->done_merges.push(task);
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

    for (int i = 0; i < QUERY_THREADS; ++i) {
      initThreads.push_back(std::thread(init_thread_func, i, sc, points_begin, points_end));
    }

    // Create the result buffers:
    for (int i = QUERY_THREADS; i > 0; i /= 2) {
      for (int j = 0; j < i; ++j) {
        sc->buffers.push_back(SortedVector<Point>(&rank_less));
      }
    }

    // Wait for the init threads to complete:
    for (int i = 0; i < QUERY_THREADS; ++i) {
      initThreads[i].join();
    }

    // Now create the query threads:
    for (int i = 0; i < QUERY_THREADS; ++i) {
      sc->query_threads.push_back(std::thread(&query_thread_func, sc));
      sc->query_threads.back().detach();
    }

    // Now create the merge threads:
    for (int i = 0; i < MERGE_THREADS; ++i) {
      sc->merge_threads.push_back(std::thread(&merge_thread_func, sc));
      sc->merge_threads.back().detach();
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

    // Fire off the query to each worker thread:
    for (int i = 0; i < QUERY_THREADS; ++i) {
      SearchContext::query_ptr query_i(new query_task(i, false, rect, count));
      sc->todo_queries.push(query_i);
    }

    // Collect the initial query results & merge them:
    int merge_buffer = QUERY_THREADS;
    for (int i = 0; i < QUERY_THREADS; i += 2) {
      // Grab two query results & merge them:
      SearchContext::query_ptr q1 = sc->done_queries.pop();

      // Feed the outliers to the first result we get back:
      if (i == 0) {
        for (const Point& p : sc->outliers) {
          if (contains(rect, p)) {
            sc->buffers[q1->id].insert(p);
          }
        }
      }

      SearchContext::query_ptr q2 = sc->done_queries.pop();
      sc->todo_merges.push(SearchContext::merge_ptr(new merge_task(false, count, q1->id, q2->id, merge_buffer++)));
    }

    while (merge_buffer < sc->buffers.size()) {
      SearchContext::merge_ptr m1 = sc->done_merges.pop();
      SearchContext::merge_ptr m2 = sc->done_merges.pop();
      sc->todo_merges.push(SearchContext::merge_ptr(new merge_task(false, count, m1->out_id, m2->out_id, merge_buffer++)));
    }

    // Reap the last merge:
    SearchContext::merge_ptr result = sc->done_merges.pop();
    const SortedVector<Point>& result_buffer = sc->buffers[result->out_id];
    std::copy(result_buffer.begin(), result_buffer.end(), out_points);
    return static_cast<int32_t>(result_buffer.size());
  }


  /* Release the resources associated with the context. Return nullptr if successful, "sc" otherwise. */
  __declspec(dllexport) SearchContext* __stdcall destroy(SearchContext* sc)
  {
    if (!sc) {
      return nullptr;
    }

    // Tell the worker bees to shut down:
    for (int i = 0; i < sc->query_threads.size(); ++i) {
      SearchContext::query_ptr query_i(new query_task());
      sc->todo_queries.push(query_i);
    }

    for (int i = 0; i < sc->merge_threads.size(); ++i) {
      SearchContext::merge_ptr merge_i(new merge_task());
      sc->todo_merges.push(merge_i);
    }

    // Dispose of the context:
    delete sc;
    return nullptr;
  }

} // extern "C"
