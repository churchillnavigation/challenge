// 
// QuadTree.h
// 01/03/2015
//
// Drew Hall
// andrew.danger.hall@gmail.com
//
// A QuadTree class with parameterized bin size and using a sorted
// vector of contiguous points at each leaf.  This should
// allow 
// 
#include "point_search.h"
#include "SortedVector.h"
#include <algorithm>
#include <iostream>
#include <assert.h>


template <int BIN_SIZE>
class QuadTree
{
  public:
    typedef SortedVector<Point> rank_vec;
    static size_t sNodeCount;

  public:
    QuadTree(const Rect& bounds);
    ~QuadTree();

    bool AddPoint(const Point& p);
    bool Contains(const Point& p) const;
    Rect Bounds() const;

    rank_vec NBest(const Rect& area, int n) const;

    void dump(int level = 0) const;

  private:
    enum Dir { DIR_NW = 0, DIR_NE, DIR_SW, DIR_SE };
    enum node_type { NT_LEAF, NT_BRANCH };

    bool add_point(const Point& pt);
    bool insert_subtree(const Point& pt);
    void subdivide();
    void build_subtree(Dir dir, float midX, float midY);

    void n_best(const Rect& area, int n, rank_vec& working_set, bool containsAll, int32_t& worstRank) const;

  private:
    Rect mBounds;
    node_type mNodeType;
    QuadTree* mChildren[4];
    rank_vec* mPoints;
};


//**************************************************************************
//* Inline function definitions
//**************************************************************************


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


inline bool rank_less(const Point& lhs, const Point& rhs)
{
  return (lhs.rank < rhs.rank);
}



//**************************************************************************
//* Template function definitions
//**************************************************************************

template <int BIN_SIZE>
size_t QuadTree<BIN_SIZE>::sNodeCount = 0;


template <int BIN_SIZE>
QuadTree<BIN_SIZE>::QuadTree(const Rect& bounds)
  : mNodeType(NT_LEAF)
  , mBounds(bounds)
  , mPoints(new rank_vec(&rank_less))
{
  ++sNodeCount;

  for (int i = 0; i < 4; ++i) {
    mChildren[i] = nullptr;
  }
}


template <int BIN_SIZE>
QuadTree<BIN_SIZE>::~QuadTree()
{
  // Delete children, being careful to ensure that
  // the proper destructor is called:
  if (mNodeType == NT_BRANCH) {
    for (int i = 0; i < 4; ++i) {
      delete mChildren[i];
    }
    assert(mPoints == nullptr);
  }
  else {
    delete mPoints;
    
    for (int i = 0; i < 4; ++i) {
      assert(mChildren[i] == nullptr);
    }
  }

  --sNodeCount;
}


template <int BIN_SIZE>
bool QuadTree<BIN_SIZE>::AddPoint(const Point& p)
{
  return add_point(p);
}


template <int BIN_SIZE>
inline bool QuadTree<BIN_SIZE>::Contains(const Point& p) const
{
  return contains(mBounds, p);
}


template <int BIN_SIZE>
typename QuadTree<BIN_SIZE>::rank_vec QuadTree<BIN_SIZE>::NBest(const Rect& area, int n) const
{
  rank_vec working_set(&rank_less);
  int32_t worstRank = std::numeric_limits<int32_t>::max();
  this->n_best(area, n, working_set, false, worstRank);
  return working_set;
}


template <int BIN_SIZE>
void QuadTree<BIN_SIZE>::n_best(const Rect& area, int n, typename QuadTree::rank_vec& working_set, bool containsAll, int32_t& worstRank) const
{
  if (containsAll) {
    // No need for further bounds queries  
    if (mNodeType == NT_LEAF) {
      // We can wholesale add all points without checking their bounds:
      for (const Point& p : *mPoints) {
        if (p.rank < worstRank) {
          working_set.insert(p);
        }
        else {
          // Since this bin is sorted by rank, all remaining points will fail the rank test.
          break;
        }
      }

      // Prune the excess points:
      if (working_set.size() > n) {
        working_set.resize(n);
        worstRank = (--working_set.end())->rank;
      }
    }
    else {
      // Branch node.  Forward to children, and let them know they
      // don't need to check bounds either:
      for (int i = 0; i < 4; ++i) {
        const QuadTree* child_i = mChildren[i];
        if (child_i) {
          child_i->n_best(area, n, working_set, true, worstRank);
        }
      }
    }
  }
  else {
    // Need to do a little more work on bounds queries:
    if (!intersects(mBounds, area)) {
      return;
    }

    containsAll = contains_all(area, mBounds);

    // If we get here, we have at least a partial intersection between
    // our bounding rectangle and the query rectangle.

    if (mNodeType == NT_LEAF) {
      // Check if our bounds are completely contained by the query area:
      if (containsAll) {
        // Yes. We can wholesale add all points without checking their bounds:
        for (const Point& p : *mPoints) {
          if (p.rank < worstRank) {
            working_set.insert(p);
          }
          else {
            // Since this bin is sorted by rank, all remaining points will fail the rank test.
            break;
          }
        }
      }
      else {
        // We need to be more careful--some are in, some are out:
        for (const Point& p : *mPoints) {
          if (contains(area, p)) {
            if (p.rank < worstRank) {
              working_set.insert(p);
            }
            else {
              break;
            }
          }
        }
      }

      // Prune the excess points:
      if (working_set.size() > n) {
        working_set.resize(n);
        worstRank = (--working_set.end())->rank;
      }
    }
    else {
      // Branch node.  Forward to children:
      for (int i = 0; i < 4; ++i) {
        const QuadTree* child_i = mChildren[i];
        if (child_i) {
          child_i->n_best(area, n, working_set, containsAll, worstRank);
        }
      }
    }
  }
}


template <int BIN_SIZE>
void QuadTree<BIN_SIZE>::dump(int level) const
{
  for (int i = 0; i < level; ++i) {
    std::cout << "  ";
  }

  if (mNodeType == NT_BRANCH) {
    std::cout << "Branch [ " << mBounds.lx << ", " << mBounds.hx << " ] x [ "
      << mBounds.ly << ", " << mBounds.hy << " ]\n";

    for (int i = 0; i < 4; ++i) {
      if (mChildren[i]) {
        mChildren[i]->dump(level + 1);
      }
      else {
        for (int j = 0; j < level + 1; ++j) {
          std::cout << "  ";
        }
        std::cout << "null\n";
      }
    }
  }
  else {
    std::cout << "Leaf [ " << mBounds.lx << ", " << mBounds.hx << " ] x [ "
      << mBounds.ly << ", " << mBounds.hy << " ]\n";

    for (auto it = mPoints->begin(); it != mPoints->end(); ++it) {
      for (int j = 0; j < level + 1; ++j) {
        std::cout << "  ";
      }
      std::cout << "(" << it->x << ", " << it->y << ") -> " << it->rank << '\n';
    }
  }
}


template <int BIN_SIZE>
bool QuadTree<BIN_SIZE>::add_point(const Point& pt)
{
  if (!Contains(pt)) {
    std::cout << "Pt (" << pt.x << ", " << pt.y << ") not contained.\n";
    return false;
  }

  if (mNodeType == NT_LEAF) {
    // This is a LEAF node.  Try to insert this point at the end:
    if (mPoints->size() < BIN_SIZE) {
      mPoints->insert(pt);
      return true;
    }
    else {
      // If we get here, we're full.  We need to subdivide,
      // then send all of our points (including the new one)
      // to the appropriate subtree:
      subdivide();
      return insert_subtree(pt);
    }
  }
  else {
    // We're subdivided.  Need to figure out which subtree
    // this point will be added to: 
    return insert_subtree(pt);
  }
}


template <int BIN_SIZE>
bool QuadTree<BIN_SIZE>::insert_subtree(const Point& pt)
{
  assert(mNodeType == NT_BRANCH);

  const float midX = (0.5f * mBounds.lx) + (0.5f * mBounds.hx);
  const float midY = (0.5f * mBounds.ly) + (0.5f * mBounds.hy);

  Dir dir;
  if (pt.x < midX) {
    if (pt.y < midY) {
      dir = DIR_SW;
    }
    else {
      dir = DIR_NW;
    }
  }
  else {
    if (pt.y < midY) {
      dir = DIR_SE;
    }
    else {
      dir = DIR_NE;
    }
  }

  if (!mChildren[dir]) {
    build_subtree(dir, midX, midY);
  }

  return mChildren[dir]->add_point(pt);
}


template <int BIN_SIZE>
void QuadTree<BIN_SIZE>::subdivide()
{
  assert(mNodeType == NT_LEAF);
  assert(mPoints->size() == BIN_SIZE);

  rank_vec* pts = mPoints;

  mNodeType = NT_BRANCH;
  mPoints = nullptr;

  // Now send our points to our subtrees:
  for (const Point& p : *pts) {
    insert_subtree(p);
  }

  // Get rid of our points
  delete pts;
}


template <int BIN_SIZE>
void QuadTree<BIN_SIZE>::build_subtree(Dir dir, float midX, float midY)
{
  assert(mNodeType == NT_BRANCH);
  assert(mChildren[dir] == nullptr);

  Rect r;
  switch (dir) {
    case DIR_NW: {
      r.lx = mBounds.lx; r.hx = midX;
      r.ly = midY;       r.hy = mBounds.hy;
    } break;

    case DIR_NE: {
      r.lx = midX;       r.hx = mBounds.hx;
      r.ly = midY;       r.hy = mBounds.hy;
    } break;

    case DIR_SW: {
      r.lx = mBounds.lx; r.hx = midX;
      r.ly = mBounds.ly; r.hy = midY;
    } break;

    case DIR_SE: {
      r.lx = midX;       r.hx = mBounds.hx;
      r.ly = mBounds.ly; r.hy = midY;
    } break;
  }

  mChildren[dir] = new QuadTree(r);
}

