/*
 * (c) Gregory Popovitch 2014-2015
 *     greg7mdp@gmail.com
 *
 */

#include "../point_search.h"
#include <vector>
#include <algorithm>
#include <cstring>
#include <cfloat>
#include <climits>
#include <cassert>

using namespace std;
template <class Iter> class QuadTree;

// ------------------------------------------------------------------------------
bool almostEqual(float A, float B, int maxUlps)
{
    // Make sure maxUlps is non-negative and small enough that the
    // default NAN won't compare as equal to anything.
    assert(maxUlps > 0 && maxUlps < 1024);

    if (A == B)
        return true;
    
    int aInt = *(int*)&A;
 
    // Make aInt lexicographically ordered as a twos-complement int
    if (aInt < 0)
        aInt = 0x80000000 - aInt;
 
    // Make bInt lexicographically ordered as a twos-complement int
    int bInt = *(int*)&B;
    if (bInt < 0)
        bInt = 0x80000000 - bInt;
 
    int intDiff = aInt - bInt;   // better not to use abs()
    if (intDiff <= maxUlps && intDiff >= -maxUlps)
        return true;
 
    return false;
}
 
inline int iround(float x) { return (int)(x > 0.0f ? x + 0.5f : x - 0.5f); }

// ------------------------------------------------------------------------------
template <class T>
class ObjAllocator
{
public:
    ObjAllocator(uint32_t batch_size = 1024) :
        _batch_size(batch_size),
        _free_list(nullptr),
        _num_allocated(0)
    {
        assert(sizeof(T) >= sizeof(void *)); // free list linking
        _blocks.reserve(32);
        new_block();
    }

    ~ObjAllocator() 
    {
        for (size_t i=0; i<_blocks.size(); ++i)
            free(_blocks[i]);
    }
        
    T *alloc()
    {
        if (!_free_list)
            new_block();
        assert(_free_list);
        T *res = _free_list;
        _free_list = *(T **)_free_list;
        _num_allocated++;
        return res;
    }
    
    void free(T *t)
    {
        if (!t)
            return;
        *(T **)t = _free_list;
        _free_list = t;
        _num_allocated--;
    }
        
private:
    void new_block()
    {
        T *ts = (T *)malloc(_batch_size * sizeof(T));
        _blocks.push_back(ts);

        // add block of Ts to free list
        // ----------------------------
        T *last = ts + (_batch_size - 1);
        while (ts < last)
        {
            *(T **)ts = ts + 1;
            ++ts;
        }
        *(T **)ts = _free_list;
        _free_list = _blocks.back();
    }

    uint32_t    _batch_size;
    T          *_free_list;
    vector<T *> _blocks;
    uint32_t    _num_allocated;
};

// ------------------------------------------------------------------------------
class vec2
{
public:
    vec2() {}
    vec2(float _x, float _y) : x(_x), y(_y) {}

    vec2 operator+(const vec2& v) const { return vec2(x + v.x, y + v.y); }
    vec2 operator-(const vec2& v) const { return vec2(x - v.x, y - v.y); }

    vec2 operator*(float f) const { return vec2(x * f, y * f); }

    void set_min() { x = -FLT_MAX; y = -FLT_MAX;  }
    void set_max() { x =  FLT_MAX; y =  FLT_MAX;  }

    float x, y;
};

// ------------------------------------------------------------------------------
class vec2r
{
public:
    vec2r() {}
    vec2r(float _x, float _y, int32_t _r) : x(_x), y(_y), r(_r) {}

    vec2r operator+(const vec2r& v) const { return vec2r(x + v.x, y + v.y, r + v.r); }
    vec2r operator-(const vec2r& v) const { return vec2r(x - v.x, y - v.y, r - v.r); }

    vec2r operator*(float f) const { return vec2r(x * f, y * f, iround(r * f)); }

    float     x, y;
    int32_t   r;       // rank
};

// ------------------------------------------------------------------------------
struct Pt
{
    Pt() {}
    Pt(const Point &p) : _id(p.id), x(p.x), y(p.y), r(p.rank) {}

    void copy(Point &p) const { p.id = _id; p.rank = r; p.x = x; p.y = y; }

    float     x, y;
    int32_t   r;       // rank
    int8_t   _id;
};

// ------------------------------------------------------------------------------
struct PtCompareR
{
    bool operator()(const Pt &a, const Pt &b) const { return a.r < b.r; }
};

// ------------------------------------------------------------------------------
class Box
{
public:
    Box() { _min.set_max(); _max.set_min(); }
    
    void reset() { _min.set_max(); _max.set_min(); }

    void add(const Pt& v) 
    { 
        if (v.x < _min.x)  _min.x = v.x;
        if (v.y < _min.y)  _min.y = v.y;

        if (v.x > _max.x)  _max.x = v.x;
        if (v.y > _max.y)  _max.y = v.y;
    }

    Box divide(uint32_t quadrant, const vec2 &c) const
    {
        assert(quadrant < 4);

        Box res = *this;

        if (quadrant & 2)
            res._min.x = c.x;
        else
            res._max.x = c.x;

        if (quadrant & 1)
            res._min.y = c.y;
        else
            res._max.y = c.y;

        return res;
    }

    float area() const
    {
        return (_max.x - _min.x) * (_max.y - _min.y);
    }

    friend vec2 center(const Box &b)     { return (b._min + b._max) * 0.5f; }
    friend bool supertight(const Box &b) { return almostEqual(b._min.x, b._max.x, 5) && 
                                                  almostEqual(b._min.y, b._max.y, 5); }
    friend class Target;

private:
    vec2 _min; 
    vec2 _max;   // actually half size (from center)
};

// ------------------------------------------------------------------------------
enum intersect_code
{
    ic_outside,
    ic_partial,
    ic_inside
};

// ------------------------------------------------------------------------------
class Target
{
public:
    Target() {}

    Target(const Rect &r, const uint32_t count) : 
        _min(r.lx, r.ly), _max(r.hx, r.hy),
        _count(count)
    {}
    
    inline intersect_code intersect(const Box &b) const
    {
        if (b._min.x > _max.x || b._max.x < _min.x ||
            b._min.y > _max.y || b._max.y < _min.y)
            return ic_outside;
        
        if (b._min.x >= _min.x && b._max.x <= _max.x &&
            b._min.y >= _min.y && b._max.y <= _max.y)
            return ic_inside;

        return ic_partial;
    }

    // valid only when intersect() above returned ic_partial
    // -----------------------------------------------------
    inline float intersection_area(const Box &b) const
    {
        float dx = min(_max.x, b._max.x) - max(_min.x, b._min.x);
        float dy = min(_max.y, b._max.y) - max(_min.y, b._min.y);
        return dx * dy;
    }

    inline bool contains(const Pt &p) const
    {
        return p.x >= _min.x && p.x <= _max.x &&
               p.y >= _min.y && p.y <= _max.y;
    }

    float area() const
    {
        return (_max.x - _min.x) * (_max.y - _min.y);
    }

    inline const uint32_t count() const { return _count; }

private:
    vec2     _min; 
    vec2     _max;
    uint32_t _count;
};

// ------------------------------------------------------------------------------
class PointSetBaseBack
{
public:
    PointSetBaseBack() 
        :  _count(20), _array_size(0), _point_array(0)
    {
        reset(_count);
    }

    PointSetBaseBack(const PointSetBaseBack &o) = delete; // no copy constructor

    ~PointSetBaseBack()
    {
        delete _point_array;
    }

    PointSetBaseBack& operator=(const PointSetBaseBack &o) 
    {
        reset(o._count);
        assert(_point_array);
        _size = o._size;
        for (uint32_t i=0; i<_size; ++i)
            _begin[i] = o._begin[i];
        return *this;
    }

    void reset(uint32_t count) 
    {
        if (_array_size < _num_counts * count)
        {
            _array_size = _num_counts * count;
            delete _point_array;
            _point_array = new Pt[_array_size];
        }
        _count = count;
        _begin = _point_array + _array_size - _count; //  _count to leave room for initial push_back
        _size = 0;
    }

    inline int32_t nth_rank() const
    {
        return (_size >= _count) ? _begin[_count - 1].r : INT_MAX;
    }

    inline int32_t nth_rank_no_check() const
    {
        return _begin[_count - 1].r;
    }

    Pt &operator[](int i) { return _begin[i]; }

    const Pt &operator[](int i) const { return _begin[i]; }

    uint32_t size() const { return _size; }

protected:

    // returns true if point can be inserted, false if its rank is too high to fit
    // ---------------------------------------------------------------------------
    bool _insert(const Pt &pt)
    {
        if (_begin <= _point_array + _count)
            _rewind();
        
        if (_size >= _count && pt.r > _begin[_count - 1].r)
            return false;

        Pt *last = _begin + _size;
        Pt *cur  = _begin;
 
        while (cur < last && cur->r < pt.r)
        {
            *(cur - 1) = *cur;
            ++cur;
        }
        --_begin;

        *(cur - 1) = pt;

        if (_size < _count)
            ++_size;

        return true;
    }

    void _insert(const Pt *beg, uint32_t num_pts)
    {
        if (_begin <= _point_array + _count)
            _rewind();

        if (_size)
        {
            Pt *last = _begin + _size;
            Pt *cur  = _begin;
            const Pt *end = beg + num_pts;

            Pt *insert    = _begin - num_pts;
            _begin -= num_pts;

            while (beg < end && (insert - _begin) < _count) // only first _count matter
            {
                if (cur < last && cur->r < beg->r)
                {
                    *insert++ = *cur++;
                }
                else
                {
                    *insert++ = *beg++;
                    ++_size;
                }
            }
        }
        else
        {
            memcpy(_begin, beg, num_pts * sizeof(Pt));
            _size = num_pts;
        }
        assert(_size <= _count);
    }

    inline void _push_back(const Pt &pt)
    {
        assert(_begin + _size < &_point_array[_array_size]);
        _begin[_size++] = pt;
    }

private:
    const int _num_counts = 50;
            
    void _rewind()
    {
        assert(_size <= _count);
        Pt *new_begin = _point_array + _array_size - _count;

        for (uint32_t i=0; i<_size; ++i)
            new_begin[i] = _begin[i];
        _begin = new_begin;
    }

protected:
    uint32_t  _size;
    uint32_t  _count;

private:
    Pt       *_begin;
    Pt       *_point_array;
    uint32_t  _array_size;
};    

// ------------------------------------------------------------------------------
//                         P O I N T S E T
// ------------------------------------------------------------------------------
class PointSet : public PointSetBaseBack
{
public:
    PointSet() {}

    PointSet(const PointSet &) = delete; // no copy constructor

    // returns nth_rank
    // -----------------------------------------------------------------------
    int add_multiple(const Pt *beg, const Pt *end, const Target &target)
    { 
        if (_size)
        {
            if (_size < _count)
            {
                for (; beg < end; ++beg)
                {
                    if (target.contains(*beg))
                    {
                        if (!_insert(*beg))
                            return nth_rank_no_check(); // further ones have higher rank
                        else if (_size >= _count)
                        {
                            ++beg;
                            break;
                        }
                    }
                }
            }

            if (beg < end) // if is needed for returned rank to be correct
            {
                // now that we have _count points, check rank to avoid as many target.contains()
                // calls as possible.
                // -----------------------------------------------------------------------------
                int32_t rank = nth_rank_no_check();

                for (; beg < end && beg->r < rank; ++beg)
                {
                    if (target.contains(*beg))
                    {
                        if (_insert(*beg))
                            rank = nth_rank_no_check();
                        else
                            break; // further ones have higher rank
                    }
                }

                assert(rank == nth_rank());
                return rank; 
            }
        }
        else
        {
            // _size == 0
            for (; beg < end; ++beg)
            {
                if (target.contains(*beg))
                {
                    _push_back(*beg); 
                    if (_size == _count)
                        break;
                }
            }
        }
        return nth_rank();
    }
    
    // returns true if at least one point had too high a rank to be added,
    // or if we have added _count points.
    // -------------------------------------------------------------------
    int add_multiple(const Pt *beg, const Pt *end)
    { 
        if (_size < _count)
        {
            uint32_t num_pts = (uint32_t)(end - beg);

            if (_size)
            {
                num_pts = min(num_pts, _count - _size);
                _insert(beg, num_pts);
                beg += num_pts;
                // no return, need to look at further points
            }
            else
            {
                // _size == 0
                _insert(beg, min(num_pts, _count));
                return nth_rank();
            }
        }
        
        for (; beg < end; ++beg)
            if (!_insert(*beg))
                break;

        return nth_rank();
    }
};

// ------------------------------------------------------------------------------
//                        S E A R C H C O N T E X T
// ------------------------------------------------------------------------------
struct SearchContext 
{
    typedef QuadTree<const Point*> Qt;

    SearchContext()
    {
        _qt_partial.reserve(2000); // way too much, but who knows?
    }

    void reset(const uint32_t count)
    {
        _pointset.reset(count);
        _qt_partial.resize(0);
    }

    void add(const Qt *qt) { _qt_partial.push_back(qt); }

protected:
    PointSet           _pointset;     // contains only points which are within target    
    vector<const Qt *> _qt_partial;   // partial quadtrees to be processed in a second pass
};


static size_t s_cell_size = 1900; 


// ------------------------------------------------------------------------------
struct CellInfo
{
    CellInfo(float area, uint32_t num_points) : _area(area), _num_points(num_points) {}

    float    _area;
    uint32_t _num_points;
};

// ------------------------------------------------------------------------------
//                           Q U A D T R E E 
// ------------------------------------------------------------------------------
template <class Iter>
class QuadTree
{
public:
    // ------------------------------------------------------------------------
    QuadTree(Iter first, Iter last) : _leaf(true)
    {
        memset(_child, 0, sizeof(_child)); 

        uint32_t num_points = (uint32_t)(last - first);
        _points.reserve(num_points);
        
        // initially, all points are in the first cell. Compute bounding box
        for (Iter p=first; p!=last; ++p)
        {
            _points.push_back(*p);
            _box.add(_points.back());
        }
        assert(num_points == _points.size());
    }

    // ------------------------------------------------------------------------
    QuadTree(const Box &box, uint32_t num_points) : _leaf(true)
    {
        memset(_child, 0, sizeof(_child)); 
        _box = box;
        _points.reserve(num_points);
    }

    // ------------------------------------------------------------------------
    QuadTree(const QuadTree &o) :
        _box(o._box),
        _points(o._points),
        _min_rank(o._min_rank),
        _leaf(o._leaf),
        _intersection_area(o._intersection_area)
    {
        for (uint32_t i=0; i<4; ++i)
            _child[i] = o._child[i] ? new QuadTree(*o._child[i]) : nullptr;
    }

    // ------------------------------------------------------------------------
    ~QuadTree()
    {
        for (int i=0; i<4; ++i)
            delete _child[i];
    }

    // ------------------------------------------------------------------------
    void populate_cell_info(vector<CellInfo> &cell_info)
    {
        if (_leaf)
            cell_info.push_back(CellInfo(_box.area(), (uint32_t)_points.size()));
        else
        {
            for (uint32_t i=0; i<4; ++i)
                if (_child[i])
                    _child[i]->populate_cell_info(cell_info);
        }
    }

    // ------------------------------------------------------------------------
    void subdivide(size_t point_threshold, int depth, uint32_t keep_count)
    {
        if (depth)
            _min_rank = _points[0].r;

        uint32_t num_points = (uint32_t)_points.size();

        if (num_points <= point_threshold)
            return;
        
        // recompute box so that it is as tight as possible
        // ------------------------------------------------
        _box.reset();
        for (uint32_t i=0; i<num_points; ++i)
            _box.add(_points[i]);
            
        if (supertight(_box))
        {
            // points close enough (in 2D) we can't discriminate... stop subdividing
            return; 
        }
        
        _leaf = false;

        vec2 c = center(_box); // cull out extreme points

        if (depth == 0)         // they will remain sorted in children
        {
            sort(_points.begin(), _points.end(), PtCompareR());
            _min_rank = _points[0].r;
        }

        uint32_t count[4];
        memset(count, 0, sizeof(count));
        
        for (uint32_t i=0; i<num_points; ++i)
            count[_quadrant(_points[i], c)]++;

        // create child quadtrees, and  move points to child quadtrees
        // -----------------------------------------------------------
        for (uint32_t i=0; i<4; ++i)
            if (count[i])
                _child[i] = new QuadTree(_box.divide(i, c), count[i]);
        
        for (size_t i=0; i<num_points; ++i)
            _child[_quadrant(_points[i], c)]->_add_point(_points[i]);
        
        vector<Pt> tmp; // holds 
        uint32_t copy_count = min(keep_count, (uint32_t)_points.size());
        tmp.insert(tmp.begin(), &_points[0], &_points[0] + copy_count);

        tmp.swap(_points); // release memory, keep first keep_count points in _points

        // subdivide further (if above threshold)
        // --------------------------------------
        for (uint32_t i=0; i<4; ++i)
        {
            if (_child[i])
                _child[i]->subdivide(point_threshold, depth+1, keep_count);
        }
    }

    // ------------------------------------------------------------------------
    void subdivide(size_t point_threshold, float area_threshold, int depth, uint32_t keep_count)
    {
        if (!_leaf)
        {
            for (uint32_t i=0; i<4; ++i)
            {
                if (_child[i])
                    _child[i]->subdivide(point_threshold, area_threshold, depth+1, keep_count);
            }
            return;
        }

        assert(area_threshold > 0);

        if (depth)
            _min_rank = _points[0].r;

        uint32_t num_points = (uint32_t)_points.size();

        // recompute box so that it is as tight as possible
        // ------------------------------------------------
        _box.reset();
        for (uint32_t i=0; i<num_points; ++i)
            _box.add(_points[i]);
            
        float box_area = _box.area();
        if (num_points * box_area < point_threshold * area_threshold)
            return;
        
        if (supertight(_box))
        {
            // points close enough (in 2D) we can't discriminate... stop subdividing
            return; 
        }
        
        _leaf = false;

        vec2 c = center(_box); // cull out extreme points

        if (depth == 0)         // they will remain sorted in children
        {
            sort(_points.begin(), _points.end(), PtCompareR());
            _min_rank = _points[0].r;
        }

        uint32_t count[4];
        memset(count, 0, sizeof(count));
        
        for (uint32_t i=0; i<num_points; ++i)
            count[_quadrant(_points[i], c)]++;

        // create child quadtrees, and  move points to child quadtrees
        // -----------------------------------------------------------
        for (uint32_t i=0; i<4; ++i)
            if (count[i])
                _child[i] = new QuadTree(_box.divide(i, c), count[i]);
        
        for (size_t i=0; i<num_points; ++i)
            _child[_quadrant(_points[i], c)]->_add_point(_points[i]);
        
        vector<Pt> tmp; // holds 
        uint32_t copy_count = min(keep_count, (uint32_t)_points.size());
        tmp.insert(tmp.begin(), &_points[0], &_points[0] + copy_count);

        tmp.swap(_points); // release memory, keep first keep_count points in _points

        // subdivide further (if above threshold)
        // --------------------------------------
        for (uint32_t i=0; i<4; ++i)
        {
            if (_child[i])
                _child[i]->subdivide(point_threshold, area_threshold, depth+1, keep_count);
        }
    }

    // Fills PointSet with points within target (in sorted rank order)
    // ---------------------------------------------------------------
    int search(PointSet &points, const Target &target, int nth_rank) const
    {
        if (nth_rank < _min_rank)
            return nth_rank;

        intersect_code ic = target.intersect(_box);

        switch (ic)
        {
        case ic_inside:
            // all points from this cell are inside the target rectangle...
            // we just need the "count" with smallest rank
            // ------------------------------------------------------------
            nth_rank = _fill(points, target.count(), nth_rank);
            break;

        case ic_partial:
        {
            // some points from this cell match
            // --------------------------------
            if (_leaf)
            {
                // leaf cell... add points inside target
                // -------------------------------------
                nth_rank = points.add_multiple(&_points[0], &_points[0] + (uint32_t)_points.size(), target);
            } 
            else
            {
                // first 4 cells always contain lower rank points than last 4
                // ----------------------------------------------------------
                for (uint32_t i=0; i<4; ++i)
                    if (_child[i])
                        nth_rank = _child[i]->search(points, target, nth_rank);
            }
            break;
        }

        case ic_outside:
            // no point to add from this cell
            // ------------------------------
            break;

        default:
            assert(0);
            break;
        }

        return nth_rank;
    }

    // -----------------------------------------------------------------------------
    int search(PointSet &points, const Target &target, SearchContext &sc, uint32_t depth, int nth_rank) const
    {
        if (nth_rank < _min_rank)
            return nth_rank;

        intersect_code ic = target.intersect(_box);

        switch (ic)
        {
        case ic_inside:
            // all points from this cell are inside the target rectangle...
            // we just need the "count" with smallest rank
            // ------------------------------------------------------------
            nth_rank = _fill(points, target.count(), nth_rank);
            break;

        case ic_partial:
        {
            // some points from this cell match
            // --------------------------------
            if (_leaf)
            {
                // leaf cell... save work for later
                // --------------------------------
                _intersection_area = target.intersection_area(_box);
                sc.add(this);
            } 
            else
            {
                for (uint32_t i=0; i<4; ++i)
                    if (_child[i])
                        nth_rank = _child[i]->search(points, target, sc, depth + 1, nth_rank);
            }
            break;
        }

        case ic_outside:
            // no point to add from this cell
            // ------------------------------
            break;

        default:
            assert(0);
            break;
        }

        return nth_rank;
    }

    // ------------------------------------------------------------------------
    int search_partial(PointSet &points, const Target &target, int nth_rank) const
    {
        if (_leaf)
        {
            nth_rank = points.add_multiple(&_points[0], &_points[0] +  (uint32_t)_points.size(), target);
        }
        else
        {
            for (uint32_t i=0; i<4; ++i)
                if (_child[i])
                    nth_rank = _child[i]->search(points, target, nth_rank);
        }

        return nth_rank;
    }

    void *operator new(size_t sz) { return _allocator.alloc(); }
    void operator delete(void *o) { _allocator.free((QuadTree *)o); }

    float ia() const { return _intersection_area; }

private:
    // when quadrant fully in target box, add points without checking rectangle
    // returns true if at least one point had too high a rank to be added.
    // ------------------------------------------------------------------------
    int _fill(PointSet &points, const uint32_t count, int nth_rank) const
    {
        if (_min_rank < nth_rank)
        {
            const uint32_t num_points = (uint32_t)_points.size();

            if (_leaf || count <= num_points)
            {
                // leaf cell... 
                nth_rank = points.add_multiple(&_points[0], &_points[0] + num_points);
            }
            else
            {
                for (uint32_t i=0; i<4; ++i)
                    if (_child[i])
                        nth_rank = _child[i]->_fill(points, count, nth_rank);
            }
        }
        return nth_rank;
    }
        
    // ------------------------------------------------------------------------
    unsigned char _quadrant(const Pt& v, const vec2 &center) const
    {
        return ((v.x > center.x) ? 2 : 0) + ((v.y > center.y) ? 1 : 0);
    }

    // ------------------------------------------------------------------------
    void _add_point(const Pt &p)
    {
        _points.push_back(p);
    }
        
    QuadTree    *_child[4];
    Box          _box;
    vector<Pt>   _points;
    int32_t      _min_rank; // lowest rank that this cell contains
    bool         _leaf;
    mutable float _intersection_area;

    static ObjAllocator<QuadTree> _allocator;
};

template<> ObjAllocator<QuadTree<const Point*>> QuadTree<const Point*>::_allocator(100000);

// ------------------------------------------------------------------------------
// Single-threaded Search 
// ------------------------------------------------------------------------------
class Search : public SearchContext
{
public:
    
    // ------------------------------------------------------------------------
    Search(const Point* points_begin, const Point* points_end) :
        _ot(points_begin, points_end), _median_leaf_cell_area(0.0f)
    {
        _ot.subdivide(s_cell_size, 0, 20);
    }

    // ------------------------------------------------------------------------
    int32_t search(const Rect rect, const int32_t count, Point* out_points)
    {
        if (count <= 0 || !out_points || rect.lx > rect.hx || rect.ly > rect.hy)
            return 0;

        reset((uint32_t)count);

        Target target(rect, (uint32_t)count);
        _ot.search(_pointset, target, INT_MAX);
        
        uint32_t num_points = (uint32_t)_pointset.size();
        if (num_points > (uint32_t)count)
            num_points = count;

        for (uint32_t i=0; i<num_points; ++i)
            _pointset[i].copy(out_points[i]);

        return num_points;
    }

protected:
    QuadTree<const Point*>  _ot;
    QuadTree<const Point*> *_ot_fine;
    float                   _median_leaf_cell_area;
    float                   _center_average_area;
};

// ----------------------------------------------------
// Search    = single_threaded, 
// MTSearch  = multi_threaded
// MTGSearch = multi_threaded using generic threadpool
// ----------------------------------------------------
typedef Search ActualSearch; 

// ------------------------------------------------------------------------------
template <class T>
void get_from_environment(T &var, const char *name)
{
    const char *s = getenv(name);
    if (s)
        var = (T)atol(s);
}

// ------------------------------------------------------------------------------
extern "C" __declspec(dllexport) SearchContext* __stdcall create(const Point* points_begin,
                                                                 const Point* points_end)
{
    if (points_begin == points_end)
        return 0;

    get_from_environment(s_cell_size,         "cell_size");

    return new ActualSearch(points_begin, points_end);
}

extern "C" __declspec(dllexport) int32_t search(SearchContext* sc, const Rect rect, 
                                                const int32_t count, Point* out_points)
{
    if (!sc)
        return 0;

    ActualSearch *s = static_cast<ActualSearch *>(sc);
    return s->search(rect, count, out_points);
}

extern "C" __declspec(dllexport) SearchContext* __stdcall destroy(SearchContext* sc)
{
    if (sc)
    {
        ActualSearch *s = static_cast<ActualSearch *>(sc);
        delete s;
    }
    return nullptr;
}

