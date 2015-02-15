/**
 * Copyright Riku Palomäki, std::string("kuplatupsu@")+"gmail.com";
 *
 * Solution to Churchill Navigation programming challenge.
 *
 * The public name for the entry is opengl_clip_plane_search.dll,
 * just to mess with people.
 *
 * Developed in Debian Linux, compiled using GCC 4.9.1 (cross compiler).
*/

#include "point_search.h"

#include <vector>
#include <algorithm>
#include <memory>
#include <future>

#define EXPORT extern "C" __attribute__ ((dllexport))

const int leaf_size = 1000;
const int max_level = 10;
const int min_level = 6;

struct Node;

using Points = std::vector<Point>;
using Nodes = std::vector<std::pair<float, Node*>>;

struct Node
{
  template <typename It>
  Node(It begin, It end)
    : split(0),
      points(begin, end)
  {
    rect.lx = rect.ly = std::numeric_limits<float>::lowest();
    rect.hx = rect.hy = std::numeric_limits<float>::max();
  }

  bool isLeaf() const { return left == nullptr; }

  Rect rect;
  std::unique_ptr<Node> left, right;
  float split;

  Points points;
};

struct SearchContext
{
  std::unique_ptr<Node> root;
};

// For some reason, things work faster if this is static and outside SearchContext
// In non-challenge use, move this to SearchContext
static std::vector<std::pair<float, Node*>> s_nodes{100};
static int32_t s_out_iterator = 0;
static int32_t s_rank_threshold = std::numeric_limits<int32_t>::max();

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

void build(Node& node, int level = 0)
{
  if (node.points.size() <= leaf_size || level >= max_level) {
    sortByRank(node.points);
  } else {
    const bool by_x = (level % 2) == 0;
    sortByLocation(node.points, by_x);

    int middle_idx = node.points.size() / 2;
    node.split = by_x ? node.points[middle_idx].x : node.points[middle_idx].y;

    node.left.reset(new Node(node.points.begin(), node.points.begin() + middle_idx));
    node.right.reset(new Node(node.points.begin() + middle_idx, node.points.end()));
    node.left->rect = node.rect;
    node.right->rect = node.rect;
    if (by_x) {
      node.left->rect.hx = node.split;
      node.right->rect.lx = node.split;
    } else {
      node.left->rect.hy = node.split;
      node.right->rect.ly = node.split;
    }

    std::future<void> futures[2];
    if (level <= min_level) {
      Points empty;
      node.points.swap(empty);
    } else {
      futures[0] = std::async(std::launch::async, [&node] { sortByRank(node.points); });
    }

    futures[1] = std::async(std::launch::async, [&node, level] {
      build(*node.left, level + 1);
    });
    build(*node.right, level + 1);

    for (auto& f: futures)
      if (f.valid()) f.wait();
  }
}

static std::vector<std::vector<Point>> out;

EXPORT SearchContext* __stdcall create(const Point* points_begin, const Point* points_end)
{
  auto sc = new SearchContext;
  sc->root.reset(new Node(points_begin, points_end));

  build(*sc->root);

  return sc;
}

inline bool addPoint(const Point& p, const int32_t capacity, Point* out_points, int& points_in_output)
{
  for (; s_out_iterator < points_in_output; ++s_out_iterator) {
    if (p.rank < out_points[s_out_iterator].rank) {
      for (int32_t j = std::min(points_in_output, capacity-1); j > s_out_iterator; --j)
        out_points[j] = out_points[j-1];
      points_in_output = std::min(points_in_output + 1, capacity);
      out_points[s_out_iterator] = p;
      s_rank_threshold = out_points[capacity-1].rank;
      return true;
    }
  }

  if (points_in_output == capacity)
    return false;

  out_points[points_in_output++] = p;
  s_rank_threshold = out_points[capacity-1].rank;
  return points_in_output != capacity;
}

inline float coverage(const Rect a, const Rect b)
{
  float lx = std::max(a.lx, b.lx);
  float hx = std::min(a.hx, b.hx);

  float ly = std::max(a.ly, b.ly);
  float hy = std::min(a.hy, b.hy);

  float x = (hx-lx) / (a.hx-a.lx);
  float y = (hy-ly) / (a.hy-a.ly);

  return -x*y;
}

void searchNode(const Node& node, const Rect rect, const int32_t count, Point* out_points, int& points_in_output)
{
  s_out_iterator = 0;
  for (const Point& p: node.points) {
    if (p.rank > s_rank_threshold) break;
    if (isInside(rect, p)) {
      if (!addPoint(p, count, out_points, points_in_output)) return;
    }
  }
}

// Returns true if rect fully overlaps n
bool buildQuery(const Node& n, const Rect rect, const int32_t count, Point* out_points, bool x)
{
  if (n.isLeaf()) {
    return true;
  } else {
    bool leftFullyOverlaps = false, rightFullyOverlaps = false;
    if (x) {
      if (rect.lx < n.split)
        leftFullyOverlaps = buildQuery(*n.left, rect, count, out_points, false);
      if (rect.hx >= n.split)
        rightFullyOverlaps = buildQuery(*n.right, rect, count, out_points, false);
    } else {
      if (rect.ly < n.split)
        leftFullyOverlaps = buildQuery(*n.left, rect, count, out_points, true);
      if (rect.hy >= n.split)
        rightFullyOverlaps = buildQuery(*n.right, rect, count, out_points, true);
    }

    if (leftFullyOverlaps && rightFullyOverlaps) {
      if (n.points.empty()) {
        s_nodes.emplace_back(coverage(n.left->rect, rect), n.left.get());
        s_nodes.emplace_back(coverage(n.right->rect, rect), n.right.get());
        return false;
      }
      return true;
    } else if (leftFullyOverlaps) {
      s_nodes.emplace_back(coverage(n.left->rect, rect), n.left.get());
    } else if (rightFullyOverlaps) {
      s_nodes.emplace_back(coverage(n.right->rect, rect), n.right.get());
    }
    return false;
  }
}

EXPORT int32_t __stdcall search(SearchContext* sc, const Rect rect, const int32_t count, Point* out_points)
{
  if (count == 0)
    return 0;

  s_nodes.clear();
  s_rank_threshold = std::numeric_limits<int32_t>::max();
  out_points[count-1].rank = s_rank_threshold;

  buildQuery(*sc->root, rect, count, out_points, true);

  std::sort(s_nodes.begin(), s_nodes.end());
  int pio = 0;
  for (auto p: s_nodes)
    searchNode(*p.second, rect, count, out_points, pio);

  return pio;
}

EXPORT SearchContext* __stdcall destroy(SearchContext* sc)
{
  delete sc;
  return nullptr;
}
