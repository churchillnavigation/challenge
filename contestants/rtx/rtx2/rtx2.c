
// WARNING: This is literally my first C/C++ program
// It's not pretty.

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include <string.h>

#define RESULT_SIZE (20)
#define NODE_SIZE (8)
#define FIRST_PARENT_SIZE (32)
#define LEAF_SIZE (2048)

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

#pragma pack(pop)

typedef struct {
  Rect mbb;
  void* children;
  int32_t lowestRank;
  uint16_t size;
  bool is_leaf;
} Node __attribute__ ((aligned));

typedef struct {
  Node* nodes;
  int32_t size;
} NArray;

typedef struct {
  Point* out_points;
  int32_t largest;
  uint16_t size;
  uint16_t count;
} Result;

typedef struct {
  Point out_points[RESULT_SIZE];
  Node root;
} SearchContext;

inline bool
contains(const float x, const float y, const Rect r) {
  return (r.lx <= x && r.hx >= x && r.ly <= y && r.hy >= y);
}

inline bool
intersects(Rect r1, Rect r2) {
  return (r2.lx <= r1.hx && r2.hx >= r1.lx && r2.ly <= r1.hy && r2.hy >= r1.ly);
}

void
insert_ordered(const Point p, Result* const restrict result) {
  Point* const restrict array = result->out_points;
  if (result->size < RESULT_SIZE)
    result->size += 1;
  
  const int32_t size = result->size;

  for (int32_t i = 0; i < size; i++) {
    if (p.rank < array[i].rank) {
      memmove((array + i + 1), (array + i), (size - i - 1) * (sizeof *array));
      array[i] = p;
      result->largest = array[size - 1].rank;
      break;
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
sort_tile_recursive(NArray array, const int32_t node_capacity) {

  int32_t size = array.size;
  Node* nodes = array.nodes;

  if (size <= NODE_SIZE)
    return array;
  
  int32_t parent_size = 0;
  int32_t parent_capacity = 1024;
  Node* parents = malloc(parent_capacity * sizeof *parents);
  
  qsort(nodes, size, sizeof *nodes, sort_nodes_x);

  int32_t num_leaves = ceil(size / (double) node_capacity);
  int32_t num_slices = ceil(sqrt(num_leaves));
  int32_t slice_size = num_slices * node_capacity;

  int32_t node_idx = 0;
  int32_t slice_offset = 0;

  for (int32_t slice_num = 0; slice_num < num_slices; slice_num++) {
    int32_t current_slice_size = fmax(0, fmin(slice_size, size - node_idx));
    int32_t current_size = 0;

    qsort(nodes + slice_offset, current_slice_size, sizeof *nodes, sort_nodes_y);

    int32_t num_nodes = ceil(current_slice_size / (double) node_capacity);
    int32_t node_offset = slice_offset;

    for (int32_t node_num = 0; node_num < num_nodes; node_num++) {
      Node node;
      Node* node_children = malloc(node_capacity * sizeof * node_children);
      Node first = nodes[node_offset];
      Rect r = {first.mbb.lx, first.mbb.ly, first.mbb.hx, first.mbb.hy};
      
      int32_t node_size = 0;
      for (; node_size < node_capacity && current_size < current_slice_size; node_size++) {
        Node n = nodes[node_offset + node_size];
        node_idx++;
        current_size++;
        if (n.mbb.lx < r.lx) r.lx = n.mbb.lx;
        if (n.mbb.hx > r.hx) r.hx = n.mbb.hx;
        if (n.mbb.ly < r.ly) r.ly = n.mbb.ly;
        if (n.mbb.hy > r.hy) r.hy = n.mbb.hy;
        node_children[node_size] = n;
      }

      node.is_leaf = false;
      node.children = node_children;
      node.size = node_size;
      node.mbb = r;
      node.lowestRank = node_children[0].lowestRank;

      if (parent_size == parent_capacity) {
        parent_capacity *= 1.5;
        parents = realloc(parents, parent_capacity * sizeof * parents);
      }
      parents[parent_size++] = node;

      node_offset += node_size;
    }
    slice_offset += current_size;
  }

  free(nodes);

  NArray new_array = {parents, parent_size};
  return sort_tile_recursive(new_array, NODE_SIZE);
}

void
build_tree(Node* root, const Point* points_begin, const Point* end) {
  int32_t size = 0;

  while(points_begin + size != end) size++;
  
  Point* points = malloc(size * sizeof *points);
  memcpy(points, points_begin, size * (sizeof *points_begin));

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

  if (size <= LEAF_SIZE) {
    root->children = points;
    root->lowestRank = points[0].rank;
    root->size = size;
    root->is_leaf = true;
  } else {
    
    int32_t children_size = 0;
    int32_t children_capacity = 1024;
    Node* children = malloc(children_capacity * sizeof *children);

    qsort(points, size, sizeof *points, sort_points_x);
    
    int32_t num_leaves = ceil(size / (double) LEAF_SIZE);
    int32_t num_slices = ceil(sqrt(num_leaves));
    int32_t slice_size = num_slices * LEAF_SIZE;

    int32_t point_idx = 0;
    int32_t slice_offset = 0;

    for (int32_t slice_num = 0; slice_num < num_slices; slice_num++) {
      int32_t current_slice_size = fmax(0, fmin(slice_size, size - point_idx));
      int32_t current_size = 0;

      qsort(points + slice_offset, current_slice_size, sizeof *points, sort_points_y);

      int32_t num_nodes = ceil(current_slice_size / (double) LEAF_SIZE);
      int32_t node_offset = slice_offset;

      for (int32_t node_num = 0; node_num < num_nodes; node_num++) {
        Node node;
        Point* node_points = malloc(LEAF_SIZE * sizeof *node_points);
        Point fp = points[node_offset];
        Rect r = {fp.x, fp.y, fp.x, fp.y};
        
        int32_t node_size = 0;
        for (; node_size < LEAF_SIZE && current_size < current_slice_size; node_size++) {
          Point p = points[node_offset + node_size];
          point_idx++;
          current_size++;
          if (p.x < r.lx) r.lx = p.x;
          if (p.x > r.hx) r.hx = p.x;
          if (p.y < r.ly) r.ly = p.y;
          if (p.y > r.hy) r.hy = p.y;
          node_points[node_size] = p;
        }

        node.is_leaf = true;
        node.children = node_points;
        node.mbb = r;
        node.size = node_size;
        node.lowestRank = node_points[0].rank;

        if (children_size == children_capacity) {
          children_capacity *= 1.5;
          children = realloc(children, children_capacity * sizeof *children);
        }
        
        children[children_size++] = node;
        
        node_offset += node_size;
      }
      slice_offset += current_size;
    }

    free(points);
    
    root->lowestRank = children[0].lowestRank;
    root->is_leaf = false;
    if (children_size > FIRST_PARENT_SIZE) {
      NArray children_array = {children, children_size};
      NArray str_array = sort_tile_recursive(children_array, FIRST_PARENT_SIZE);
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
  const int32_t count = result->count;

  if (n.is_leaf) {
    const Point* const restrict points = n.children;
    for (int32_t i = 0; i < size; i++) {
      if (points[i].rank >= result->largest && result->size >= count) 
        break;
      __builtin_prefetch(points + i + 1, 0, 0);
      if (contains(points[i].x, points[i].y, rect))
        insert_ordered(points[i], result);
    }
  } else {
    const Node* const restrict children = n.children;
    for (int32_t i = 0; i < size; i++) {
      Node ni = children[i];
      if (ni.lowestRank >= result->largest && result->size >= count)
        break;
      __builtin_prefetch(children + i + 1, 0, 3);
      if (intersects(rect, ni.mbb))
        search_node(ni, rect, result);
    }
  }
}

void
free_node(Node n) {
  if (!n.is_leaf)
    for (int32_t i = 0; i < n.size; i++)
      free_node(((Node*)n.children)[i]);  
  free(n.children);
}

void
sort_tree(Node* n) {
  if (n->is_leaf) {
    qsort(n->children, n->size, sizeof(Point), sort_points_rank);
    n->lowestRank = ((Point*)n->children)[0].rank;
  } else {
    for (int32_t i = 0; i < n->size; i++)
      sort_tree(&((Node*)n->children)[i]);
    qsort(n->children, n->size, sizeof(Node), sort_nodes_rank);
    n->lowestRank = ((Node*)n->children)[0].lowestRank;
  }
}

__declspec(dllexport) SearchContext* __stdcall 
create(const Point* points_begin, const Point* points_end) {

  SearchContext* sc = malloc(sizeof *sc);
  if (points_begin == points_end) {
    sc->root.size = 0;
    sc->root.children = NULL;
  } else {
    build_tree(&sc->root, points_begin, points_end);
    sort_tree(&sc->root);
  }
  
  return sc;
}

__declspec(dllexport) int32_t  __stdcall
search(SearchContext* sc, const Rect rect, const int32_t count, Point* out_points) {
  if (sc->root.size < 1)
    return count;

  Result result;
  
  result.size = 0;
  result.count = count;
  result.largest = INT32_MIN;
  result.out_points = sc->out_points;

  for (int32_t i = 0; i < count; i++)
    result.out_points[i].rank = INT32_MAX;

  search_node(sc->root, rect, &result);

  memcpy(out_points, result.out_points, result.size * (sizeof *out_points));

  return result.size;
}

__declspec(dllexport) SearchContext* __stdcall
destroy(SearchContext* sc)  {
  free_node(sc->root);
  free(sc);
  return NULL;
}
