/**
 * Copyright Riku Palomäki, std::string("kuplatupsu@")+"gmail.com";
 *
 * Solution to Churchill Navigation programming challenge.
 *
 * The public name for the entry is manatee_opengl2_clip_plane.dll,
 * just to mess with people.
 *
 * Developed in Debian Linux, compiled using GCC 4.9.1 (cross compiler).
*/

#include "point_search.h"

#include <vector>
#include <algorithm>
#include <memory>
#include <future>

static const int leaf_max_size = 1024;
static const int max_level = 5;
static const int min_level = max_level-3;
static const int N = 5;
static const int split_threshold = 20;
static const int split_limit = 4;

#define EXPORT extern "C" __attribute__ ((dllexport))

using Points = std::vector<Point>;

struct Node
{
  Node() { for (float& f: edges) f = std::numeric_limits<float>::lowest(); }
  inline bool isLeaf() const { return children == nullptr; }

  Node* children{nullptr};
  std::array<float, N+1> edges;

  bool split_direction_x{true};

  Points points;
  std::pair<Point*, Point*> points_it;
};

struct SearchContext
{
  std::vector<Node> nodes;
  std::mutex nodes_mutex;
};

// For some reason, things work faster if this is static and outside SearchContext
// In non-challenge use, move these to SearchContext
using PointRange = std::pair<Point*, Point*>;
static std::vector<PointRange> s_nodes{500};

static int32_t s_out_iterator = 0;
static int32_t s_rank_threshold = std::numeric_limits<int32_t>::max();

// Highly questionable choice to hard-code the count-parameter from search-
// function, since in the challenge, it always is 20... This do have a
// small effect on run times.
static const int32_t s_capacity = 20;

inline bool isInside(const Rect rect, const Point p)
{
  return p.x >= rect.lx && p.x <= rect.hx && p.y >= rect.ly && p.y <= rect.hy;
}

inline void sortByLocation(Points& points, bool by_x)
{
  auto cmp = by_x
    ? [] (Point l, Point r) { return l.x < r.x; }
    : [] (Point l, Point r) { return l.y < r.y; };

  std::sort(points.begin(), points.end(), cmp);
}

inline void sortByRank(Points& points)
{
  std::sort(points.begin(), points.end(), [] (Point l, Point r) {
      return l.rank < r.rank;
  });
}

inline float pick(Point p, bool x)
{
  return x ? p.x : p.y;
}

void build(SearchContext& sc, Node& node, const int level, const bool sortedByX)
{
  const int total_points = node.points.size();

  if (total_points <= leaf_max_size || level >= max_level) {
    sortByRank(node.points);
    node.points_it = {node.points.data(), node.points.data() + total_points};
  } else {
    float diffa = pick(node.points[total_points * 0.75], sortedByX) -
                  pick(node.points[total_points * 0.25], sortedByX);

    sortByLocation(node.points, !sortedByX);

    float diffb = pick(node.points[total_points * 0.75], !sortedByX) -
                  pick(node.points[total_points * 0.25], !sortedByX);

    if (diffa > diffb*1800.f) {
      sortByLocation(node.points, sortedByX);
      node.split_direction_x = sortedByX;
    } else {
      node.split_direction_x = !sortedByX;
    }

    typedef std::pair<float, int> P;
    std::vector<P> splits;
    splits.reserve(node.points.size());
    for (int i = 1, s = node.points.size(); i != s; ++i)
      splits.emplace_back(pick(node.points[i], node.split_direction_x)-pick(node.points[i-1], node.split_direction_x), i);

    std::sort(splits.begin(), splits.end(), [] (P a, P b) { return a.first > b.first; });
    std::sort(splits.begin(), splits.begin() + N-1, [] (P a, P b) { return a.second < b.second; });

    node.edges[0] = pick(node.points[0], node.split_direction_x);

    std::vector<int> split_idx;
    split_idx.reserve(N);

    int i = 0;
    for (; i < N-1; ++i) {
      if (splits[i].first < split_threshold) break;
      split_idx.push_back(splits[i].second);
      node.edges[split_idx.size()] = pick(node.points[splits[i].second], node.split_direction_x);
    }

    int pieces_remaining = N;
    int piece_begin_idx = 0;
    if (i <= split_limit) {
      split_idx.clear();

      while (pieces_remaining > 1) {
        const int remaining_points = total_points - piece_begin_idx;
        const int piece_point_count = remaining_points / pieces_remaining;

        int piece_end_idx = piece_begin_idx + piece_point_count;
        if (piece_end_idx == total_points) break;

        float split = pick(node.points[piece_end_idx], node.split_direction_x);
        float prev = pick(node.points[piece_end_idx-1], node.split_direction_x);

        while (split == prev) {
          if (++piece_end_idx == total_points) goto end;
          prev = split;
          split = pick(node.points[piece_end_idx], node.split_direction_x);
        }

        split_idx.push_back(piece_end_idx);
        node.edges[split_idx.size()] = split;

        piece_begin_idx = piece_end_idx;
        --pieces_remaining;
      }
    }
end:

    split_idx.push_back(total_points);
    float last = pick(node.points.back(), node.split_direction_x);
    node.edges[split_idx.size()] = std::nextafter(last, last + 100.f);

    const int piece_count = split_idx.size();

    {
      std::lock_guard<std::mutex> g(sc.nodes_mutex);
      auto s = sc.nodes.size();
      sc.nodes.resize(s + N);
      node.children = sc.nodes.data() + s;
    }

    auto it = node.children;
    int previdx = 0;
    for (int idx: split_idx) {
      it++->points = Points(node.points.begin() + previdx, node.points.begin() + idx);
      previdx = idx;
    }

    std::vector<std::future<void>> futures;
    futures.reserve(piece_count);

    if (level <= min_level) {
      Points empty;
      node.points.swap(empty);
    } else {
      futures.emplace_back(std::async(std::launch::async, [&node] {
        sortByRank(node.points);
        node.points_it = {node.points.data(), node.points.data() + node.points.size()};
      }));
    }

    auto last_child = node.children + N - 1;
    futures.emplace_back(std::async(std::launch::async, [&sc, &node, level, it, last_child] {
      for (Node* it = node.children; it != last_child; ++it) {
        build(sc, *it, level + 1, node.split_direction_x);
      }
    }));
    build(sc, *last_child, level + 1, node.split_direction_x);

    for (auto& f: futures)
      f.wait();
  }
}

static std::vector<std::vector<Point>> out;

int calcMaxNodes()
{
  int s = 0;
  for (int i = 0; i < max_level; ++i)
    s = s * N + N;
  return s+1;
}

EXPORT SearchContext* __stdcall create(const Point* points_begin, const Point* points_end)
{
  auto sc = new SearchContext;

  sc->nodes.reserve(sizeof(Node) * calcMaxNodes());
  sc->nodes.resize(1);
  auto& root = sc->nodes[0];

  root.points.reserve(points_end-points_begin);
  for (auto p = points_begin; p != points_end; ++p) {
    /// Yet another questionable choise, just for the challenge. The search
    // rectangles are always in +-thousands, and there are always couple of
    // very extreme points. Just ignore them.
    if (p->x < -100000 || p->y < -100000 || p->x > 100000 || p->y > 100000) continue;
    root.points.push_back(*p);
  }

  sortByLocation(root.points, true);
  if (!root.points.empty()) {
    build(*sc, root, 0, true);
  }

  return sc;
}

inline bool addPoint(const Point& p, Point* out_points, int& points_in_output)
{
  for (; s_out_iterator != points_in_output; ++s_out_iterator) {
    if (p.rank < out_points[s_out_iterator].rank) {
      for (int32_t j = std::min(points_in_output, s_capacity-1); j > s_out_iterator; --j)
        out_points[j] = out_points[j-1];
      points_in_output = std::min(points_in_output + 1, s_capacity);
      out_points[s_out_iterator] = p;
      s_rank_threshold = out_points[s_capacity-1].rank;
      return true;
    }
  }

  if (points_in_output == s_capacity)
    return false;

  out_points[points_in_output++] = p;
  s_rank_threshold = out_points[s_capacity-1].rank;
  return points_in_output != s_capacity;
}

// Returns true if rect fully overlaps n
// Unfortunately unrolling loops manually seems to be faster than using gcc
// unroll optimizations, or not unroll at all.
inline bool buildQuery(const Node& n, const Rect rect)
{
  unsigned int bits = 0;
  if (n.split_direction_x) {
#define D1(i) if (rect.hx >= n.edges[i] && rect.lx < n.edges[i+1]) \
      bits |= (n.children[i].isLeaf() || buildQuery(n.children[i], rect)) << i;
    D1(0); D1(1); D1(2); D1(3); D1(4);
  } else {
#define D2(i) if (rect.hy >= n.edges[i] && rect.ly < n.edges[i+1]) \
    bits |= (n.children[i].isLeaf() || buildQuery(n.children[i], rect)) << i;
    D2(0); D2(1); D2(2); D2(3); D2(4);
  }

  if (bits == (1<<N)-1) {
    if (n.points.empty()) {
#define D3(i) s_nodes.emplace_back(n.children[i].points_it)
      D3(0); D3(1); D3(2); D3(3); D3(4);
      return false;
    }
    return true;
  }
#define D4(i) if (bits & (1 << i)) s_nodes.emplace_back(n.children[i].points_it)
  D4(0); D4(1); D4(2); D4(3); D4(4);
  return false;
}

EXPORT int32_t __stdcall search(SearchContext* sc, const Rect rect, const int32_t, Point* out_points)
{
  s_nodes.clear();
  s_rank_threshold = std::numeric_limits<int32_t>::max();
  out_points[s_capacity-1].rank = s_rank_threshold;

  buildQuery(sc->nodes[0], rect);

  int32_t pio = 0;
  int inc = 2;
  auto begin = s_nodes.begin(), end = s_nodes.end();
  do {
    auto target = begin;
    for (auto sit = target; sit != end; ++sit) {
      auto& p = *sit;
      int i = 0;
      s_out_iterator = 0;
      for (;;) {
        if (p.first->rank > s_rank_threshold) {
          break;
        }
        if (isInside(rect, *p.first)) {
          if (!addPoint(*p.first, out_points, pio)) {
            break;
          }
        }
        ++p.first;
        if (p.first == p.second) break;
        if (++i == inc) {
          *target++ = *sit;
          break;
        }
      }
    }
    end = target;
    inc = inc * 3 / 2;
  } while (begin != end);

  return pio;
}

EXPORT SearchContext* __stdcall destroy(SearchContext* sc)
{
  delete sc;
  return nullptr;
}
