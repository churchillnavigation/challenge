// 
// RTree.h
// 02/03/2015
//
// Drew Hall
// andrew.danger.hall@gmail.com
//
// An R-Tree class for spatial indexing.  It is templatized so that
// the branch & leaf sizes can be experimented with, but other than that
// is fairly specific to the Rect and Point data structures provided
// by Churchill.
//
// The tree is loaded via a Sort/Tile/Recurse (STR) bulk loading
// process that builds optimally packed trees.
//
// Queries are further optimized by sorting children at each node in order
// of increasing lowest rank, thereby allowing maximal early pruning
// using a simple conditional (much easier than box intersection).
// Box intersection tests are used only as a last resort--wherever
// possible the knowledge of intersection is passed down from higher
// in the tree to allow child nodes to avoid expensive intersection/
// containment tests.
//
// Finally, node sizes are optimized for cache performance.
//
// Overall result is roughly 100x faster than the reference implementation.
// 
#ifndef RTREE_H
#define RTREE_H

#include "point_search.h"
#include "SortedVector.h"
#include <algorithm>
#include <limits>
#include <vector>
#include <deque>
#include <iostream>
#include <cassert>


template <short BranchSize, short LeafSize>
class RTree
{
  public:
    typedef SortedVector<Point> rank_vec;

  public:
    RTree();
    RTree(const Point* begin, const Point* end);
    ~RTree();

    int NBest(const Rect& area, int n, rank_vec& result) const;

    void dump() const;

  private:
    class node
    {
      public:
        node();
        node(const Point& p);
        virtual ~node();

        virtual void n_best(const Rect& area, int n, rank_vec& working_set, bool containsAll, int32_t& worstRank) const = 0;

        virtual void dump(int level = 0) const = 0;
        virtual void optimize() = 0;

        int32_t lowest_rank() const;
        const Rect& bounding_box() const;
  
      public:
        friend inline bool center_x_less(const node* lhs, const node* rhs)
        {
          const Rect& lbox = lhs->bounding_box();
          const Rect& rbox = rhs->bounding_box();
          return 0.5 * (lbox.lx + lbox.hx) < 0.5 * (rbox.lx + rbox.hx);
        }

        friend inline bool center_y_less(const node* lhs, const node* rhs)
        {
          const Rect& lbox = lhs->bounding_box();
          const Rect& rbox = rhs->bounding_box();
          return 0.5 * (lbox.ly + lbox.hy) < 0.5 * (rbox.ly + rbox.hy);
        }

        friend inline bool lowest_rank_less(const node* lhs, const node* rhs)
        {
          return lhs->lowest_rank() < rhs->lowest_rank();
        }

      protected:
        Rect mBoundingBox;
        int32_t mLowestRank;
        short mN;
    };

    class leaf_node : public node
    {
      public:
        leaf_node(const Point* pbeg, const Point* pend);

        virtual void n_best(const Rect& area, int n, rank_vec& working_set, bool containsAll, int32_t& worstRank) const;

        virtual void dump(int level) const;
        virtual void optimize();

      private:
        Point mChildren[LeafSize];
    };

    class branch_node : public node
    {
      public:
        virtual ~branch_node();
        virtual void insert(node* child);

        virtual void n_best(const Rect& area, int n, rank_vec& working_set, bool containsAll, int32_t& worstRank) const;

        virtual void dump(int level) const;
        virtual void optimize();

      private:
        node* mChildren[BranchSize];
        Rect mBoundingBoxes[BranchSize];
        int32_t mLowestRanks[BranchSize];
    };

    node* mRoot;
};



//*****************************************************************************
//*  Inline definitions:
//*****************************************************************************


//+---------------------------------------------------------------------------
//|  Auxiliary functions for sorting & manipulating Points & Rects:
//+---------------------------------------------------------------------------

inline bool x_less(const Point& lhs, const Point& rhs)
{
  return lhs.x < rhs.x;
}


inline bool y_less(const Point& lhs, const Point& rhs)
{
  return lhs.y < rhs.y;
}


inline bool rank_less(const Point& lhs, const Point& rhs)
{
  return lhs.rank < rhs.rank;
}


inline Rect rect_from_pt(const Point& pt)
{
  Rect r;
  r.lx = r.hx = pt.x;
  r.ly = r.hy = pt.y;
  return r;
}

inline Rect rect_union(const Rect& lhs, const Rect& rhs)
{
  Rect u;
  u.lx = std::min(lhs.lx, rhs.lx);
  u.hx = std::max(lhs.hx, rhs.hx);
  u.ly = std::min(lhs.ly, rhs.ly);
  u.hy = std::max(lhs.hy, rhs.hy);
  return u;
}


inline bool contains(const Rect& r, const Point& p)
{
  return (r.lx <= p.x) && (p.x <= r.hx) && (r.ly <= p.y) && (p.y <= r.hy);
}


inline bool contains_all(const Rect& outer, const Rect& inner)
{
  return (outer.lx <= inner.lx) && (inner.hx <= outer.hx) && (outer.ly <= inner.ly) && (inner.hy <= outer.hy);
}


inline bool intersects(const Rect& r1, const Rect& r2)
{
  // Look for a separating axis:
  return !((r2.lx > r1.hx) || (r1.lx > r2.hx) || (r2.ly > r1.hy) || (r1.ly > r2.hy));
}



//+---------------------------------------------------------------------------
//|  RTree Class Implementation:
//+---------------------------------------------------------------------------

template <short BranchSize, short LeafSize>
RTree<BranchSize, LeafSize>::RTree()
  : mRoot(nullptr)
{
}


template <short BranchSize, short LeafSize>
RTree<BranchSize, LeafSize>::RTree(const Point* pbegin, const Point* pend)
  : mRoot(nullptr)
{
  // Implement sort-tile recursive (STR) bulk loading for
  // optimal packing:
  size_t n = std::distance(pbegin, pend);

  if (n == 0) {
    return;
  }

  // Copy data so we can sort it to our liking:
  std::vector<Point> pts;
  pts.reserve(n);
  std::copy(pbegin, pend, std::back_inserter(pts));

  // Buffer for holding created nodes until we've assembled them into a tree:
  std::deque<node*> nodes;

  // 1) Assemble leaf nodes covering all input points:

  // Determine some key layout parameters:
  const int nLeaves = static_cast<int>(ceil(static_cast<double>(n) / LeafSize));
  const int nSlices = static_cast<int>(ceil(sqrt(static_cast<double>(nLeaves))));
  const int ptsPerSlice = static_cast<int>(ceil(static_cast<double>(n) / nSlices));

  // Sort points by x coordinate:
  std::sort(pts.begin(), pts.end(), &x_less);

  //std::cout << "Sorted by x.\n";

  std::vector<Point>::iterator pb = pts.begin();
  std::vector<Point>::iterator pe = pb;
  for (int i = 0; i < nSlices; ++i) {
    assert(pb == pe);
    pe += std::min(ptsPerSlice, static_cast<int>(pts.end() - pb));
    
    // Sort this vertical slice by y-coordinate so we can build a stack
    // of boxes:
    std::sort(pb, pe, &y_less);

    // Now build a leaf for each box:
    while (pb < pe) {
      const int m = std::min(static_cast<int>(LeafSize), static_cast<int>(pe - pb));
      Point* p = &*pb;
      nodes.push_back(new RTree<BranchSize, LeafSize>::leaf_node(p, p + m));
      pb += m;
    }
  }

  // Subsequent passes:  construct branch nodes to coalesce the lower layers:
  int nNodes = static_cast<int>(nodes.size());
  while (nNodes > 1) {
    // Determine some key layout parameters:
    const int nBranches = static_cast<int>(ceil(static_cast<double>(nNodes) / BranchSize));
    const int nSlices = static_cast<int>(ceil(sqrt(static_cast<double>(nBranches))));
    const int nodesPerSlice = static_cast<int>(ceil(static_cast<double>(nNodes) / nSlices));

    // Sort existing nodes by center x value:
    std::sort(nodes.begin(), nodes.end(), &center_x_less);

    // Now assemble branch nodes to hold the subordinate nodes:
    int remainingNodes = nNodes;
    for (int i = 0; i < nSlices; ++i) {
      // Sort this x-cluster in the y dim:
      int m = std::min(nodesPerSlice, remainingNodes);
      std::sort(nodes.begin(), nodes.begin() + m, &center_y_less);

      int fullPasses = m / BranchSize;
      for (int j = 0; j < fullPasses; ++j) {
        branch_node* newBranch = new branch_node();
        nodes.push_back(newBranch);

        for (int k = 0; k < BranchSize; ++k) {
          node* n = nodes.front();
          newBranch->insert(n);
          nodes.pop_front();
          --remainingNodes;
          --m;
        }
      }

      if (m > 0) {
        branch_node* newBranch = new branch_node();
        nodes.push_back(newBranch);

        while (m > 0) {
          node* n = nodes.front();
          newBranch->insert(n);
          nodes.pop_front();
          --remainingNodes;
          --m;
        }
      }
    }

    nNodes = static_cast<int>(nodes.size());
  }

  // Take ownership of the last remaining node as the root of the tree:
  assert(nodes.size() == 1);
  mRoot = nodes.front();
  nodes.pop_front();

  // Now that we've established the geometric structure (x/y tiles),
  // impose the rank sort on each node to allow our searches to
  // short-circuit optimally:
  mRoot->optimize();
}


template <short BranchSize, short LeafSize>
RTree<BranchSize, LeafSize>::~RTree()
{
  delete mRoot;
}


template <short BranchSize, short LeafSize>
int RTree<BranchSize, LeafSize>::NBest(const Rect& area, int n, rank_vec& result) const
{
  result.clear();
  if (mRoot) {
    int32_t worstRank = std::numeric_limits<int32_t>::max();
    mRoot->n_best(area, n, result, false, worstRank);
    return static_cast<int>(result.size());
  }
  else {
    return 0;
  }
}


//+---------------------------------------------------------------------------
//|  RTree::node Class Implementation:
//+---------------------------------------------------------------------------

template <short BranchSize, short LeafSize>
inline RTree<BranchSize, LeafSize>::node::node()
  : mLowestRank(std::numeric_limits<int32_t>::max())
  , mN(0)
{
  mBoundingBox.lx = std::numeric_limits<float>::max();
  mBoundingBox.hx = -std::numeric_limits<float>::max();
  mBoundingBox.ly = std::numeric_limits<float>::max();
  mBoundingBox.hy = -std::numeric_limits<float>::max();
}


template <short BranchSize, short LeafSize>
inline RTree<BranchSize, LeafSize>::node::node(const Point& pt)
  : mBoundingBox(rect_from_pt(pt))
  , mLowestRank(pt.rank)
  , mN(1)
{
}


template <short BranchSize, short LeafSize>
inline RTree<BranchSize, LeafSize>::node::~node()
{
  // No-op
}

template <short BranchSize, short LeafSize>
inline int32_t RTree<BranchSize, LeafSize>::node::lowest_rank() const
{
  return mLowestRank;
}


template <short BranchSize, short LeafSize>
inline const Rect& RTree<BranchSize, LeafSize>::node::bounding_box() const
{
  return mBoundingBox;
}



//+---------------------------------------------------------------------------
//|  RTree::branch_node Class Implementation:
//+---------------------------------------------------------------------------

template <short BranchSize, short LeafSize>
RTree<BranchSize, LeafSize>::branch_node::~branch_node()
{
  for (int i = 0; i < mN; ++i) {
    delete mChildren[i];
  }
}


template <short BranchSize, short LeafSize>
void RTree<BranchSize, LeafSize>::branch_node::insert(typename RTree<BranchSize, LeafSize>::node* child)
{
  // Just collect the vitals from this child.  We'll organize later with the optimize() call:
  assert(mN < BranchSize);

  mChildren[mN] = child;
  mLowestRanks[mN] = child->lowest_rank();
  mBoundingBoxes[mN] = child->bounding_box();
  mBoundingBox = rect_union(mBoundingBox, child->bounding_box());

  ++mN;
}


template <short BranchSize, short LeafSize>
void RTree<BranchSize, LeafSize>::branch_node::
  n_best(const Rect& area, int n, rank_vec& working_set, bool containsAll, int32_t& worstRank) const
{
  if (containsAll) {
    // No need for further bounds queries--we can recurse to all children
    // whose lowest ranks make them contenders:
    for (int i = 0; i < mN; ++i) {
      if (mLowestRanks[i] < worstRank) {
        mChildren[i]->n_best(area, n, working_set, true, worstRank);
      }
      else {
        // Since the children are sorted by rank, all remaining children will fail the rank test.
        break;
      }
    }
  }
  else {
    // Don't know a priori whether or not each child is contained in search box:
    for (int i = 0; i < mN; ++i) {
      if (mLowestRanks[i] < worstRank) {
        if (intersects(area, mBoundingBoxes[i])) {
          mChildren[i]->n_best(area, n, working_set, contains_all(area, mBoundingBoxes[i]), worstRank);
        }
      }
      else {
        // Since children are sorted by rank, all subsequent children will also fail the rank test.
        break;
      }
    }
  }
}


template <short BranchSize, short LeafSize>
void RTree<BranchSize, LeafSize>::branch_node::dump(int level) const
{
  for (int i = 0; i < level; ++i) {
    std::cout << "  ";
  }

  std::cout << "Branch:  [ " << mBoundingBox.lx << ", " << mBoundingBox.hx
    << " ] x [ " << mBoundingBox.ly << ", " << mBoundingBox.hy << " ] -> "
    << mLowestRank << ":\n";

  for (int i = 0; i < mN; ++i) {
    mChildren[i]->dump(level + 1);
  }
}


template <short BranchSize, short LeafSize>
void RTree<BranchSize, LeafSize>::branch_node::optimize()
{
  // First, optimize all the children (recursively):
  for (int i = 0; i < mN; ++i) {
    mChildren[i]->optimize();
  }

  // Now, sort the child pointers by increasing lowest rank:
  std::sort(&mChildren[0], &mChildren[mN], &lowest_rank_less);

  // ...update the cached box & rank info:
  for (int i = 0; i < mN; ++i) {
    mLowestRanks[i] = mChildren[i]->lowest_rank();
    mBoundingBoxes[i] = mChildren[i]->bounding_box();
  }

  // Finally, update our node summary info:
  if (mN > 0) {
    mLowestRank = mLowestRanks[0];
  }
}



//+---------------------------------------------------------------------------
//|  RTree::leaf_node Class Implementation:
//+---------------------------------------------------------------------------

template <short BranchSize, short LeafSize>
RTree<BranchSize, LeafSize>::leaf_node::leaf_node(const Point* pbeg, const Point* pend)
{
  // Insert & maintain the bounding box.  We'll handle the rank sort & update
  // later with the optimize() call.
  assert((pend - pbeg) <= LeafSize);

  Point* cit = &mChildren[0];
  for (const Point* pit = pbeg; pit != pend; ++pit, ++cit) {
    *cit = *pit;
    mBoundingBox = rect_union(mBoundingBox, rect_from_pt(*pit));
    ++mN;
  }
}


template <short BranchSize, short LeafSize>
void RTree<BranchSize, LeafSize>::leaf_node::
  n_best(const Rect& area, int n, rank_vec& working_set, bool containsAll, int32_t& worstRank) const
{
  if (containsAll) {
    // No need for further bounds queries--we can wholesale add
    // all points without checking their bounds (but still need to check ranks):
    for (int i = 0; i < mN; ++i) {  
      if (mChildren[i].rank < worstRank) {
        working_set.insert(mChildren[i]);
      }
      else {
        // Since this bin is sorted by rank, all remaining points will fail the rank test.
        break;
      }
    }
  }
  else {
    // Need to check for containment of these points...but only if their rank indicates
    // that they are candidates:
    for (int i = 0; i < mN; ++i) {
      if (mChildren[i].rank < worstRank) {
        if (contains(area, mChildren[i])) {
          working_set.insert(mChildren[i]);
        }
      }
      else {
        break;
      }
    }
  }

  // Prune the excess points to keep our working set as small as possible,
  // and update the worstRank sentinel value:
  if (working_set.size() > n) {
    working_set.resize(n);
    worstRank = (--working_set.end())->rank;
  }
}


template <short BranchSize, short LeafSize>
void RTree<BranchSize, LeafSize>::leaf_node::dump(int level) const
{
  for (int i = 0; i < level; ++i) {
    std::cout << "  ";
  }

  std::cout << "Leaf:  [ " << mBoundingBox.lx << ", " << mBoundingBox.hx
    << " ] x [ " << mBoundingBox.ly << ", " << mBoundingBox.hy << " ] -> "
    << mLowestRank << ":\n";

  for (int i = 0; i < mN; ++i) {
    for (int j = 0; j < level + 1; ++j) {
      std::cout << "  ";
    }
    const Point& p_i = mChildren[i];
    std::cout << "(" << p_i.x << ", " << p_i.y << ") -> " << p_i.rank << " {" << static_cast<int>(p_i.id) << "}\n";
  }
}


template <short BranchSize, short LeafSize>
void RTree<BranchSize, LeafSize>::leaf_node::optimize()
{
  // Force the points to be sorted by increasing rank.
  std::sort(&mChildren[0], &mChildren[mN], &rank_less);

  // Now, note the smallest rank:
  mLowestRank = mChildren[0].rank;
}

#endif // RTREE_H