
// WARNING: This is literally my first C/C++ program
// It's not pretty.

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>

#define RESULT_SIZE (20)
#define NODE_SIZE (512)

#pragma pack(push, 1)

typedef struct {
  int8_t id;
  int32_t rank;
  float x;
  float y;
} Point;

typedef struct {
  float lx;
  float ly;
  float hx;
  float hy;
} Rect;

typedef struct Node {
  Rect mbb;
  int32_t lowestRank;
  int32_t size;
  struct Node* children;
  Point* points;
} Node;

#pragma pack(pop)

typedef struct {
  int32_t size;
  Node* nodes;
} NArray;

typedef struct {
  int32_t size;
  int32_t count;
  int64_t largest;
  Point* out_points;
} Result;

typedef struct {
  Node root;
  Point* out_points;
} SearchContext;

bool
contains(const float x, const float y, const Rect r) {
  return (r.lx <= x && r.hx >= x && r.ly <= y && r.hy >= y);
}

bool
intersects(Rect r1, Rect r2) {
  return !(r2.lx > r1.hx
           || r2.hx < r1.lx
           || r2.ly > r1.hy
           || r2.hy < r1.ly);
}

void
insert_ordered(const Point p, Result* const restrict result) {
  Point* const restrict array = result->out_points;

  if (result->size < RESULT_SIZE)
    result->size += 1;
  const int32_t size = result->size;
  for (int8_t i = 0; i < size; i++) {
    if (p.rank < array[i].rank) {
      memmove((array + i + 1), (array + i), (size - i - 1) * sizeof *array);
      array[i] = p;
      result->largest = array[size - 1].rank;
      return;
    }
  }
}

int
sort_nodes_rank(const void* a, const void* b) {
  int32_t r1 = ((Node*) a)->lowestRank;
  int32_t r2 = ((Node*) b)->lowestRank;
  if (r1 < r2) return -1;
  if (r1 > r2) return 1;
  return 0;
}

int
sort_nodes_x(const void* a, const void* b) {
  Rect r1 = ((Node*) a)->mbb;
  Rect r2 = ((Node*) b)->mbb;
  float f1 = r1.lx + (r1.hx - r1.lx) / 2;
  float f2 = r2.lx + (r2.hx - r2.lx) / 2;
  if (f1 < f2) return -1;
  if (f1 > f2) return 1;
  return 0;
}

int
sort_nodes_y(const void* a, const void* b) {
  Rect r1 = ((Node*) a)->mbb;
  Rect r2 = ((Node*) b)->mbb;
  float f1 = r1.ly + (r1.hy - r1.ly) / 2;
  float f2 = r2.ly + (r2.hy - r2.ly) / 2;
  if (f1 < f2) return -1;
  if (f1 > f2) return 1;
  return 0;
}

int
sort_points_rank(const void* a, const void* b) {
  int32_t r1 = ((Point*) a)->rank;
  int32_t r2 = ((Point*) b)->rank;
  if (r1 < r2) return -1;
  if (r1 > r2) return 1;
  return 0;
}

int
sort_points_x(const void* a, const void* b) {
  float r1 = ((Point*) a)->x;
  float r2 = ((Point*) b)->x;
  if (r1 < r2) return -1;
  if (r1 > r2) return 1;
  return 0;
}

int
sort_points_y(const void* a, const void* b) {
  float r1 = ((Point*) a)->y;
  float r2 = ((Point*) b)->y;
  if (r1 < r2) return -1;
  if (r1 > r2) return 1;
  return 0;
}


// https://archive.org/stream/nasa_techdoc_19970016975/19970016975#page/n9/mode/2up
NArray
sort_tile_recursive(NArray array) {

  int32_t size = array.size;
  Node* nodes = array.nodes;

  if (size <= NODE_SIZE)
    return array;
  
  int32_t parent_size = 0;
  int32_t parent_capacity = 1024;
  Node* parents = malloc(parent_capacity * sizeof *parents);
  
  qsort(nodes, size, sizeof *nodes, sort_nodes_x);

  int32_t num_leaves = ceil(size / (double) NODE_SIZE);
  int32_t num_slices = ceil(sqrt(num_leaves));
  int32_t slice_size = num_slices * NODE_SIZE;

  int32_t node_idx = 0;
  int32_t slice_offset = 0;

  for (int32_t slice_num = 0; slice_num < num_slices; slice_num++) {
    int32_t current_slice_size = fmax(0, fmin(slice_size, size - node_idx));
    int32_t current_size = 0;

    qsort(nodes + slice_offset, current_slice_size, sizeof *nodes, sort_nodes_y);

    int32_t num_nodes = ceil(current_slice_size / (double) NODE_SIZE);
    int32_t node_offset = slice_offset;

    for (int32_t node_num = 0; node_num < num_nodes; node_num++) {
      Node node;
      Node* node_children = malloc(NODE_SIZE * sizeof * node_children);
      Node first = nodes[node_offset];
      Rect r = {first.mbb.lx, first.mbb.ly, first.mbb.hx, first.mbb.hy};
      
      int32_t node_size = 0;
      for (; node_size < NODE_SIZE && current_size < current_slice_size; node_size++) {
        Node n = nodes[node_offset + node_size];
        node_idx++;
        current_size++;
        if (n.mbb.lx < r.lx) r.lx = n.mbb.lx;
        if (n.mbb.hx > r.hx) r.hx = n.mbb.hx;
        if (n.mbb.ly < r.ly) r.ly = n.mbb.ly;
        if (n.mbb.hy > r.hy) r.hy = n.mbb.hy;
        node_children[node_size] = n;
      }

      node.children = node_children;
      node.size = node_size;
      node.mbb = r;
      node.points = NULL;
      node.lowestRank = node_children[0].lowestRank;

      if (parent_size == parent_capacity) {
        parent_capacity *= 2;
        parents = realloc(parents, parent_capacity * sizeof * parents);
      }
      parents[parent_size++] = node;

      node_offset += node_size;
    }
    slice_offset += current_size;
  }

  free(nodes);
  
  parents = realloc(parents, parent_size * sizeof *parents);

  NArray new_array = {parent_size, parents};
  return sort_tile_recursive(new_array);
}

void
build_tree(Node* root, const Point* points_begin, const Point* end) {
  int32_t size = 0;

  while(points_begin + size != end) size++;
  
  Point* points = malloc(size * sizeof *points);
  memcpy(points, points_begin, size * sizeof *points_begin);

  int32_t x = points[0].x;
  int32_t y = points[0].y;
  Rect rect = {x, y, x, y};
  
  for (int32_t i = 0; i < size; i++) {
    Point p = points[i];
    if (p.x < rect.lx) rect.lx = p.x;
    if (p.x > rect.hx) rect.hx = p.x;
    if (p.y < rect.ly) rect.ly = p.y;
    if (p.y > rect.hy) rect.hy = p.y;
  }

  root->mbb = rect;

  if (size <= NODE_SIZE) { // root is leaf
    root->points = points;
    root->lowestRank = points[0].rank;
    root->size = size;
    root->children = NULL;
  } else {
    root->points = NULL;
    
    int32_t children_size = 0;
    int32_t children_capacity = 1024;
    Node* children = malloc(children_capacity * sizeof *children);

    qsort(points, size, sizeof *points, sort_points_x);
    
    int32_t num_leaves = ceil(size / (double) NODE_SIZE);
    int32_t num_slices = ceil(sqrt(num_leaves));
    int32_t slice_size = num_slices * NODE_SIZE;

    int32_t point_idx = 0;
    int32_t slice_offset = 0;

    for (int32_t slice_num = 0; slice_num < num_slices; slice_num++) {
      int32_t current_slice_size = fmax(0, fmin(slice_size, size - point_idx));
      int32_t current_size = 0;

      qsort(points + slice_offset, current_slice_size, sizeof *points, sort_points_y);

      int32_t num_nodes = ceil(current_slice_size / (double) NODE_SIZE);
      int32_t node_offset = slice_offset;

      for (int32_t node_num = 0; node_num < num_nodes; node_num++) {
        Node node;
        Point* node_points = malloc(NODE_SIZE * sizeof *node_points);
        Point fp = points[node_offset];
        Rect r = {fp.x, fp.y, fp.x, fp.y};
        
        int32_t node_size = 0;
        for (; node_size < NODE_SIZE && current_size < current_slice_size; node_size++) {
          Point p = points[node_offset + node_size];
          point_idx++;
          current_size++;
          if (p.x < r.lx) r.lx = p.x;
          if (p.x > r.hx) r.hx = p.x;
          if (p.y < r.ly) r.ly = p.y;
          if (p.y > r.hy) r.hy = p.y;
          node_points[node_size] = p;
        }

        node.children = NULL;
        node.mbb = r;
        node.points = node_points;
        node.size = node_size;
        node.lowestRank = node_points[0].rank;

        if (children_size == children_capacity) {
          children_capacity *= 2;
          children = realloc(children, children_capacity * sizeof *children);
        }
        
        children[children_size++] = node;
        
        node_offset += node_size;
      }
      slice_offset += current_size;
    }

    free(points);

    children = realloc(children, children_size * sizeof *children);
    
    root->lowestRank = children[0].lowestRank;
    if (children_size > NODE_SIZE) {
      NArray children_array = {children_size, children};
      NArray str_array = sort_tile_recursive(children_array);
      root->size = str_array.size;
      root->children = str_array.nodes;
    } else {
      root->size = children_size;
      root->children = children;
    }
  }
}

void
search_node(const Node n, const Rect rect, Result* const restrict result) {
  const int32_t size = n.size;
  if (size < 1)
    return;

  const int32_t count = result->count;

  if (n.points != NULL) {
    const Point* const restrict points = n.points;
    for (int32_t i = 0; i < size; i++) {
      __builtin_prefetch(points + i + 1, 0, 0);
      Point p = points[i];
      if (p.rank >= result->largest && result->size == count) 
        break;
      if (contains(p.x, p.y, rect))
        insert_ordered(p, result);
    }
  } else {
    const Node* const restrict children = n.children;
    for (int32_t i = 0; i < size; i++) {
      __builtin_prefetch(children + i + 1, 0, 3);
      Node ni = children[i];
      if (ni.lowestRank >= result->largest && result->size == count)
        break;
      if (intersects(rect, ni.mbb))
        search_node(ni, rect, result);
    }
  }
}

void
free_node(Node n) {
  if (n.points != NULL)
    free(n.points);
  if (n.children != NULL) {
    for (int32_t i = 0; i < n.size; i++)
      free_node(n.children[i]);

    free(n.children);
  }
}

void
sort_tree(Node* n) {
  if (n->points != NULL) {
    qsort(n->points, n->size, sizeof *(n->points), sort_points_rank);
    n->lowestRank = n->points[0].rank;
  }
  if (n->children != NULL) {
    for (int32_t i = 0; i < n->size; i++)
      sort_tree(&n->children[i]);
    qsort(n->children, n->size, sizeof *(n->children), sort_nodes_rank);
    n->lowestRank = n->children[0].lowestRank;
  }
}

__declspec(dllexport) SearchContext* __stdcall 
create(const Point* points_begin, const Point* points_end) {
  SearchContext* sc = malloc(sizeof *sc);
  if (points_begin == points_end) {
    sc->root.size = 0;
    sc->root.points = NULL;
    sc->root.children = NULL;
    sc->root.lowestRank = 0;
  } else {
    build_tree(&sc->root, points_begin, points_end);
    sort_tree(&sc->root);
  }
  
  sc->out_points = malloc(RESULT_SIZE * sizeof *(sc->out_points));

  return sc;
}

__declspec(dllexport) int32_t  __stdcall
search(SearchContext* sc, const Rect rect, const int32_t count, Point* out_points) {
  Result result;
  // find up to count rather than RESULT_SIZE
  result.size = 0;
  result.count = count;
  result.largest = INT64_MAX;
  result.out_points = sc->out_points;

  for (int32_t i = 0; i < count; i++)
    result.out_points[i].rank = INT32_MAX;

  search_node(sc->root, rect, &result);

  memcpy(out_points, result.out_points, result.size * sizeof *out_points);
  return result.size;
}

__declspec(dllexport) SearchContext* __stdcall
destroy(SearchContext* sc)  {
  if (sc->root.points != NULL)
    free(sc->root.points);
  if (sc->root.children != NULL)
    for (int32_t i = 0; i < sc->root.size; i++)
      free_node(sc->root.children[i]);

  free(sc->out_points);
  free(sc);
  return NULL;
}
