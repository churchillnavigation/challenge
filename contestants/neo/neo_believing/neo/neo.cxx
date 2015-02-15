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
#include <cmath>
#include <climits>
#include <cassert>

using namespace std;

static uint32_t s_cell_size = 72000;       // over 65536 we'd need to use uint32_t for size
static const uint32_t _num_intervals = 60;
static float    s_interval_percent = 0.08f;
static uint32_t s_depth_threshold = 5; 
static uint32_t s_max_num_hits = 10; 
static uint32_t s_interval_threshold = 20 * _num_intervals;


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
    Pt(int32_t rank)   : r(rank) {}
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
struct PtCompareX
{
    bool operator()(const Pt &a, const Pt &b) const { return a.x < b.x; }
};

// ------------------------------------------------------------------------------
struct PtCompareY
{
    bool operator()(const Pt &a, const Pt &b) const { return a.y < b.y; }
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

    float width()  const { return _max.x - _min.x; }
    float height() const { return _max.y - _min.y; }

    static const int efactor = 10;
    bool elongated() const {  float r = width() / height();  return r > efactor || r < 1.0f / efactor;  }
    bool wide() const      {  float r = width() / height();  return r > 1; }

    friend class TargetBase;
    friend class TargetW;
    friend class TargetT;

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
enum target_code
{
    tc_below,
    tc_intersect,
    tc_above
};

// ------------------------------------------------------------------------------
class TargetBase
{
public:
    TargetBase() {}

    TargetBase(const Rect &r, const uint32_t count, float cloud_area, float base) : 
        _min(r.lx, r.ly), _max(r.hx, r.hy),
        _count(count),
        _interval_percent(base)
    {
        float a = area();

        if (a < 512)
            _interval_percent = (a < 1 ? 6.2f : 3.4f) * base;
        else
            _interval_percent = (a < 32768 ? 0.8f : 0.2f)  * base;
    }
    
    // valid only when intersect() above returned ic_partial
    // -----------------------------------------------------
    float intersection_area(const Box &b) const { return intersection_width(b) * intersection_height(b); }
    float intersection_width(const Box &b)  const { return min(_max.x, b._max.x) - max(_min.x, b._min.x); }
    float intersection_height(const Box &b)  const { return min(_max.y, b._max.y) - max(_min.y, b._min.y); }

    float area()   const { return width() * height(); }
    float width()  const { return _max.x - _min.x; }
    float height() const { return _max.y - _min.y; }
    float ratio()  const { return height() != 0 ? width() / height() : 1000000.0f; }

    inline const uint32_t count() const { return _count; }

    bool small_part_of(const Box &box, bool &along_x) const
    {
        float x_percent = intersection_width(box)  / box.width();
        float y_percent = intersection_height(box) / box.height();
        
        along_x = x_percent < y_percent;
        if (along_x)
            return x_percent <= _interval_percent;
        else
            return y_percent <= _interval_percent;
    }

    bool below_x(const float *interval) const { return interval[1] < _min.x; }
    bool below_y(const float *interval) const { return interval[1] < _min.y; }

    bool above_x(const float *interval) const { return *interval > _max.x; }
    bool above_y(const float *interval) const { return *interval > _max.y; }

    target_code check_interval_x(const float *interval) const
    {
        if (interval[1] < _min.x)
            return tc_below;
        if (*interval > _max.x)
            return tc_above;
        return tc_intersect;
    }

    target_code check_interval_y(const float *interval) const
    {
        if (interval[1] < _min.y)
            return tc_below;
        if (*interval > _max.y)
            return tc_above;
        return tc_intersect;
    }

protected:
    vec2     _min; 
    vec2     _max;
    uint32_t _count;
    float    _interval_percent;
};

// -------------------------- Wide ---------------------------------------------
class TargetW : public TargetBase
{
public:
    TargetW(const Rect &r, const uint32_t count, float cloud_area, float interval_percent) :
        TargetBase(r, count, cloud_area, interval_percent) {}

    inline bool contains(const Pt &p) const
    {
        return p.y >= _min.y && p.y <= _max.y &&         // test y first
               p.x >= _min.x && p.x <= _max.x ;
    }    

    bool wide() const { return true; }

    inline intersect_code intersect(const Box &b) const
    {
        if (b._min.y > _max.y || b._max.y < _min.y ||    // test y first
            b._min.x > _max.x || b._max.x < _min.x)
            return ic_outside;
        
        if (b._min.x >= _min.x && b._max.x <= _max.x &&
            b._min.y >= _min.y && b._max.y <= _max.y)
            return ic_inside;

        return ic_partial;
    }

};

// ---------------------------- Tall ---------------------------------------------
class TargetT : public TargetBase
{
public:
    TargetT(const Rect &r, const uint32_t count, float cloud_area, float interval_percent) : 
        TargetBase(r, count, cloud_area, interval_percent) {}

    inline bool contains(const Pt &p) const
    {
        return p.x >= _min.x && p.x <= _max.x &&        // test x first
               p.y >= _min.y && p.y <= _max.y;
    }    

    bool wide() const { return false; }

    inline intersect_code intersect(const Box &b) const
    {
        if (b._min.x > _max.x || b._max.x < _min.x ||   // test x first
            b._min.y > _max.y || b._max.y < _min.y)
            return ic_outside;
        
        if (b._min.x >= _min.x && b._max.x <= _max.x &&
            b._min.y >= _min.y && b._max.y <= _max.y)
            return ic_inside;

        return ic_partial;
    }
};

// ------------------------------------------------------------------------------
class PointSetBaseBack
{
public:
    PointSetBaseBack() :  _count(20), _array_size(0), _point_array(0)
    {
        reset(_count);
    }

    PointSetBaseBack(const PointSetBaseBack &o) = delete; // no copy constructor
    PointSetBaseBack& operator=(const PointSetBaseBack &o) = delete;

    ~PointSetBaseBack()
    {
        delete _point_array;
    }

    void reset(uint32_t count) 
    {
        if (_array_size < _num_counts * count)
        {
            _array_size = _num_counts * count;
            delete _point_array;
            _point_array = new Pt[_array_size+1]; // +1 because we are assigning _begin[_size].r = INT_MAX
        }
        _count = count;
        _size = 0;
        _begin = _point_array + _array_size - _count; //  _count to leave room for initial push_back
    }

    inline int32_t nth_rank() const
    {
        return (_size >= _count) ? _begin[_count - 1].r : INT_MAX;
    }

    inline int32_t nth_rank_no_check() const
    {
        return _begin[_count - 1].r;
    }

    inline int32_t nth_rank_no_check(uint32_t idx) const
    {
        return _begin[_count - idx].r;
    }


    Pt &operator[](int i) { return _begin[i]; }

    const Pt &operator[](int i) const { return _begin[i]; }

    uint32_t size() const { return _size; }

protected:

    // returns true if point can be inserted, false if its rank is too high to fit
    // ---------------------------------------------------------------------------
    int _insert(const Pt &pt)
    {
        if (_begin <= _point_array + _count)
            _rewind();
        
        Pt *cur  = _begin;
        if (_size)
        {
            _begin[_size].r = INT_MAX; // so we stop there!
            while (cur->r < pt.r)
            {
                *(cur - 1) = *cur;
                ++cur;
            }
        }
        --_begin;

        *(cur - 1) = pt;

        if (_size < _count)
            ++_size;

        return (_size >= _count) ? _begin[_count - 1].r : INT_MAX;;
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
                    if (_size < _count)
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

    // -----------------------------------------------------------------------
    template <class Target>
    int add_intervals(const Pt **hits, uint32_t num_hits, const Target &target, int rank, int simul)
    { 
        while (num_hits)
        {
            for (int i=0; i<(int)num_hits; ++i) // i should be an integer
            {
                const Pt *pt = hits[i];

                // do simul point from each interval at a time
                // -------------------------------------------
                int num_points = simul;
                while (num_points)
                {
                    if (pt->r < rank)
                    {
                        if (target.contains(*pt))
                            rank = _insert(*pt);
                        ++pt;
                        num_points--;
                    }
                    else
                    {
                        // done with this interval
                        if (i < (int)(num_hits - 1))
                            std::swap(hits[i], hits[num_hits - 1]);
                        --num_hits;
                        --i;                       // because it will be incremented again by the for loop
                        break;
                    }
                }
                if (num_points == 0)
                    hits[i] = pt;
            }
        }

        return rank;
    }

    // -----------------------------------------------------------------------
    template <class Target>
    int add_multiple_t(const Pt *beg, const Target &target)
    { 
        if (_size)
        {
            if (_size < _count)
            {
                for (; beg->r < INT_MAX; ++beg)
                {
                    if (target.contains(*beg))
                    {
                        if (_insert(*beg) != INT_MAX)
                        {
                            // we reached _size == _count
                            ++beg;
                            break;
                        }
                    }
                }
            }

            if (beg->r < INT_MAX) // if is needed for returned rank to be correct
            {
                // now that we have _count points, check rank to avoid as many target.contains()
                // calls as possible.
                // -----------------------------------------------------------------------------
                int32_t rank = nth_rank_no_check();

                for (; beg->r < rank; ++beg)
                    if (target.contains(*beg))
                        rank = _insert(*beg);

                assert(rank == nth_rank());
                return rank;
            }
        }
        else
        {
            // _size == 0
            for (; beg->r < INT_MAX; ++beg)
            {
                if (target.contains(*beg))
                {
                    _push_back(*beg); 
                    if (_size == _count)
                        return beg->r;
                }
            }
        }
        return nth_rank();
    }
    
    // returns new max rank
    // -------------------------------------------------------------------
    int add_multiple(const Pt *beg, uint32_t num_pts)
    { 
        int rank;

        if (_size < _count)
        {
            if (_size)
            {
                num_pts = min(num_pts, _count - _size);
                _insert(beg, num_pts);
                beg += num_pts;
                rank = nth_rank();
                // no return, need to look at further points
            }
            else
            {
                // _size == 0
                _insert(beg, min(num_pts, _count));
                return nth_rank();
            }
        }
        else
            rank = nth_rank_no_check();
        
        for (; beg->r < rank; ++beg)
            rank = _insert(*beg);

        return rank;
    }
};

// ------------------------------------------------------------------------------
//                        S E A R C H C O N T E X T
// ------------------------------------------------------------------------------
struct SearchContext 
{
};

// ------------------------------------------------------------------------------
struct CellInfo
{
    CellInfo(float area, uint32_t num_points) : _area(area), _num_points(num_points) {}

    float    _area;
    uint32_t _num_points;
};

template <class qtidx_t> class Search;

// ------------------------------------------------------------------------------
//                           Q U A D T R E E 
// ------------------------------------------------------------------------------
template <class qtidx_t>
class QuadTree
{
public:
    // ------------------------------------------------------------------------
    QuadTree()
    {
        init();
    }

    void init()
    {
        _box.reset();
        memset(_child, 0, sizeof(_child)); 
        _points_x = 0;
        _points_y = 0;
        _points_r = 0;
        _num_points = 0;
        _num_children = 0;
        _total_num_points = 0;
        _leaf = true;
        _ranked = false;
    }

    // ------------------------------------------------------------------------
    ~QuadTree()
    {
    }

    // ------------------------------------------------------------------------
    uint32_t subdivide(Search<qtidx_t> *ctx, vector<Pt> &points, uint32_t depth,
                       uint32_t max_depth, uint32_t keep_count, bool pretend);

    template <class Target>
    int search(Search<qtidx_t> *ctx, PointSet &points, const Target &target, int nth_rank) const;

    template <class Target>
    int search(Search<qtidx_t> *ctx, PointSet &points, const Target &target, uint32_t depth, 
               int nth_rank) const;

    template <class Target>
    int search_partial(Search<qtidx_t> *ctx, PointSet &points, const Target &target, int nth_rank) const;
    
    int32_t min_rank() const { return _min_rank; }

    int fill(Search<qtidx_t> *ctx, PointSet &points, const uint32_t count, int nth_rank) const;

private:
    uint32_t _interval_size() const { return _num_points / _num_intervals; }

    enum { store_intervals = 1, store_ranked = 2 };

    void _storepoints(Search<qtidx_t> *ctx, vector<Pt> &points, uint32_t num_points, uint32_t keep_count,
                      bool pretend, uint32_t store_spec);

    // ------------------------------------------------------------------------
    template <class CoordAccessor, class Compare>
    void _store_intervals(Search<qtidx_t> *ctx, vector<Pt> &points, float *limits, uint32_t &pts,
                          CoordAccessor &ca, Compare &compare)
    {
        uint32_t interval_size = _interval_size();

        sort(points.begin(), points.end(), compare);
        for (uint32_t i=0; i<_num_intervals; ++i)
            limits[i] = ca(points[i * interval_size]);
        limits[_num_intervals] = ca(points[_num_points-1]);

        pts = ctx->pt_alloc(_num_points + _num_intervals); // one sentinel per interval
        Pt *p = ctx->pt_ptr(pts);
        
        uint32_t cnt = interval_size;
        uint32_t points_idx = 0;

        for (uint32_t i=0; i<_num_intervals; ++i)
        {
            if (i == _num_intervals - 1)
                cnt += _num_points - _num_intervals * interval_size;
            memcpy(p, &points[points_idx], cnt * sizeof(Pt));
            points_idx += cnt;

            sort(p, p + cnt, PtCompareR());
            p += cnt;
            *p++ = Pt(INT_MAX); // the sentinel
        }
    }

    // ------------------------------------------------------------------------
    template <class Target, class TargetCheckFn>
    void _prep_intervals(const Pt **hits, uint32_t &num_hits, const float *limits, 
                         const Pt *p, TargetCheckFn below_fn, TargetCheckFn above_fn, 
                         const Target &target) const
    {
        uint32_t cnt = _interval_size();
        uint32_t i=0;

        // skip below intervals intil we reach one that intersects the target
        // ------------------------------------------------------------------
        // TODO optimise this search for the first interval -> binary search
        // ------------------------------------------------------------------
        while ((target.*below_fn)(&limits[i]))
        {
            p += cnt + 1;          // +1 for sentinel
            ++i;
            assert(i<_num_intervals);
        }

        assert(!(target.*below_fn)(&limits[i]) && !(target.*above_fn)(&limits[i]));
        
        // accumulate in hits the pointers to points of intersecting intervals
        // -------------------------------------------------------------------
        do 
        {
            hits[num_hits++] = p;
            p += cnt + 1;          // +1 for sentinel
            ++i;
        } while (i<_num_intervals && !(target.*above_fn)(&limits[i]));
        
        assert(i == _num_intervals || (target.*above_fn)(&limits[i]));
    }

    // ------------------------------------------------------------------------
    template <class Target>
    int _prep_ranked_intervals(Search<qtidx_t> *ctx, PointSet &pointset, 
                               const Pt **hits, uint32_t &num_hits, 
                               const Target &target, int nth_rank) const
    {
        if (nth_rank < _min_rank)
            return nth_rank;

        intersect_code ic = target.intersect(_box);
        switch (ic)
        {
        case ic_inside:
            return fill(ctx, pointset, target.count(), nth_rank);

        case ic_partial:
            if (_ranked)
            {
                Pt *p = ctx->pt_ptr(_points_r);
                hits[num_hits++] = p;
            }
            else
            {
                for (uint32_t i=0; i<4; ++i)
                    if (_child[i])
                        nth_rank = ctx->qt_ptr(_child[i])->_prep_ranked_intervals(ctx, pointset, hits, num_hits, target, nth_rank);
                return nth_rank;
            }
            break;

        case ic_outside:
        default:
            break;
        }
 
        return nth_rank;
    }

    // ------------------------------------------------------------------------
    unsigned char _quadrant(const Pt& v, const vec2 &center) const
    {
        return ((v.x > center.x) ? 2 : 0) + ((v.y > center.y) ? 1 : 0);
    }

    Box           _box;
    qtidx_t       _child[4];
    uint32_t      _points_x;
    uint32_t      _points_y;
    uint32_t      _points_r;
    uint32_t      _num_children;
    float         _x_limit[_num_intervals+1];
    float         _y_limit[_num_intervals+1];
    uint32_t      _num_points;
    uint32_t      _total_num_points;
    bool          _leaf;
    bool          _ranked;
    int32_t       _min_rank; // lowest rank that this cell contains
};

// ------------------------------------------------------------------------------
// Single-threaded Search 
// ------------------------------------------------------------------------------
template <class qtidx_t>
class Search : public SearchContext
{
public:
    typedef QuadTree<qtidx_t> Qt;

    // ------------------------------------------------------------------------------
    struct Partial 
    {
        Partial() : _qt(nullptr) {}
        Partial(const Qt *qt, float ia) : _qt(qt), _ia(ia) {} 

        const Qt  *_qt;
        float      _ia; // intersection area
    };
        

    // ------------------------------------------------------------------------
    Search(const Point *points_begin, const Point *points_end) :
        _num_points((uint32_t)(points_end - points_begin)),
        _median_leaf_cell_area(0.0f),
        _root_qt(0)
    {
        const uint32_t keep_count = 20; // stored on non-leaf nodes

        _num_quadtrees = 65535;
        _quadtrees.reserve(_num_quadtrees);

        vector<Pt> points;

        // just do a pretend subdivide to see how many quadtrees we need
        // -------------------------------------------------------------
        Box box;
        _fillpoints(points, points_begin, points_end, &box);
        _cloud_area = box.width() * box.height();

        float ratio = box.width() / box.height();
        _points_log2r = log2(ratio);

        if (abs(_points_log2r) > 4)
            s_interval_percent *= 1.0f + (abs(_points_log2r) - 4.0f) * 0.33f; //  * 0.66f;

        _root_qt = qt_alloc();
        qt_ptr(_root_qt)->subdivide(this, points, 0, INT_MAX, keep_count, true);

        // ok, _quadtrees.size() is the number of quadtrees we'll need
        // -----------------------------------------------------------
        _num_quadtrees = (uint32_t)_quadtrees.size();

        _points_prealloc = (uint32_t)((_num_points + _num_quadtrees * _num_intervals) * 2 + // overshooting here
                                       _num_points + _num_quadtrees +                       // should be only _leaf qt
                                       (keep_count + 1) * _num_quadtrees); // +1 for sentinel
        _points.reserve(_points_prealloc);

        // finally build the quadtree
        // --------------------------
        vector<Qt>().swap(_quadtrees);    // free excess memory
        _quadtrees.reserve(_num_quadtrees);
        _root_qt = qt_alloc();
        
        _partial.reserve(1000);        // way too much, but who knows?
        _full.reserve(1000);           // same
        _next_partial.reserve(1000);   // same

        _fillpoints(points, points_begin, points_end, nullptr); // destroyed by the previous subdivide

        qt_ptr(_root_qt)->subdivide(this, points, 0, INT_MAX, keep_count, false);
    }

    ~Search()
    {
    }

    // ------------------------------------------------------------------------
    void _fillpoints(vector<Pt> &points, const Point *points_begin, const Point *points_end, Box *box)
    {
        points.resize(0);
        points.reserve(_num_points);
        if (box)
            box->reset();
        
        // prepare sorted vector of points for subdivision
        // -----------------------------------------------
        for (const Point *p = points_begin; p != points_end; ++p)
        {
            if (box && abs(p->x) < 1.0e6 && abs(p->y) < 1.0e6 )
                box->add(*p);
                
            points.push_back(*p);
        }
        assert(_num_points == points.size());
        sort(points.begin(), points.end(), PtCompareR());
    }

    // ------------------------------------------------------------------------
    template <class Target>
    void _search(const Target &target)
    {
        // fill in "fully inside" first, and then process the "partial"
        // ------------------------------------------------------------
        int nth_rank = qt_ptr(_root_qt)->search(this, _pointset, target, 0, INT_MAX);

        size_t num_partial = _partial.size();
        if (num_partial > 1)
            std::sort(_partial.begin(), _partial.end(), 
                      [](const Partial &a, const Partial &b) -> bool { return a._ia > b._ia; });

        for (size_t i=0; i<num_partial; ++i)
            nth_rank = _partial[i]._qt->search_partial(this, _pointset, target, nth_rank);
    }

    // ------------------------------------------------------------------------
    int32_t search(const Rect rect, const int32_t count, Point* out_points)
    {
        float dx = rect.hx - rect.lx;
        float dy = rect.hy - rect.ly;

        if (count <= 0 || !out_points || dx < 0 || dy < 0)
            return 0;

        reset((uint32_t)count);

        if (dx > dy)
            _search(TargetW(rect, (uint32_t)count, _cloud_area, s_interval_percent));
        else
            _search(TargetT(rect, (uint32_t)count, _cloud_area, s_interval_percent));
        
        // fill out the result buffer
        // --------------------------
        uint32_t num_points = (uint32_t)_pointset.size();
        if (num_points > (uint32_t)count)
            num_points = count;

        for (uint32_t i=0; i<num_points; ++i)
            _pointset[i].copy(out_points[i]);

        return num_points;
    }

    void reset(const uint32_t count)
    {
        _partial.resize(0);
        _full.resize(0);
        _next_partial.resize(0);
        _pointset.reset(count);
    }

    void add_partial(const Qt *qt, float ia)      { _partial.emplace_back(Partial(qt, ia)); }
    void add_full(const Qt *qt)                   { _full.emplace_back(qt); }
    void add_next_partial(const Qt *qt, float ia) { _next_partial.emplace_back(Partial(qt, ia)); }

    qtidx_t qt_alloc() 
    { 
        qtidx_t res = (qtidx_t)_quadtrees.size(); 
        assert(sizeof(qtidx_t) == 4 || res < 65534);
        _quadtrees.resize(res + 1);
        return res; 
    }

    QuadTree<qtidx_t> *qt_ptr(qtidx_t idx) { return &_quadtrees[idx]; }

    uint32_t pt_alloc(uint32_t count) 
    { 
        uint32_t res = (uint32_t)_points.size(); 
        _points.resize(res + count);
        return res;
    }

    Pt *pt_ptr(uint32_t idx) { return &_points[idx]; }

    uintptr_t num_points() const { return _num_points; }

protected:
    uint32_t           _num_points;   // how many points this search context 
    PointSet           _pointset;     // contains only points which are within target    

    qtidx_t            _root_qt;      // root quadtree for our points
    vector<Qt>         _quadtrees;    // allocator for quadtrees
    qtidx_t            _num_quadtrees;
               
    vector<Pt>         _points;       // allocator for point arrays
    uint32_t           _points_prealloc;

    float              _points_log2r;

    vector<Partial>    _partial;      // partial quadtrees to be processed in a second pass
    vector<Partial>    _next_partial; 
    vector<const Qt *> _full;
    
    float              _median_leaf_cell_area;
    float              _center_average_area;
    float              _cloud_area;
};

// ------------------------------------------------------------------------------
template <class T>
bool get_from_environment(T &var, const char *name)
{
    const char *s = getenv(name);
    if (s)
        var = (T)atol(s);
    return !!s;
}

// ------------------------------------------------------------------------------
extern "C" __declspec(dllexport) SearchContext* __stdcall create(const Point* points_begin,
                                                                 const Point* points_end)
{
    if (points_begin == points_end)
        return 0;

    get_from_environment(s_depth_threshold,   "depth_threshold");
    get_from_environment(s_cell_size,         "cell_size");
    get_from_environment(s_max_num_hits,      "max_num_hits");

    uint32_t interval_percent;
    if (get_from_environment(interval_percent,"interval_percent"))
        s_interval_percent = interval_percent / 100.0f;
    
    uintptr_t num_points = points_end - points_begin;
    if (s_cell_size < num_points / 10000)
        s_cell_size = (uint32_t)(num_points / 10000); // otherwise we may have over 65535 cells

    return new Search<uint16_t>(points_begin, points_end);
}

extern "C" __declspec(dllexport) int32_t search(SearchContext* sc, const Rect rect, 
                                                const int32_t count, Point* out_points)
{
    if (!sc)
        return 0;

    Search<uint16_t> *s = static_cast<Search<uint16_t> *>(sc);
    return s->search(rect, count, out_points);
}

extern "C" __declspec(dllexport) SearchContext* __stdcall destroy(SearchContext* sc)
{
    if (!sc)
        return nullptr;

    Search<uint16_t> *s = static_cast<Search<uint16_t> *>(sc);
    delete s;

    return nullptr;
}

// ------------------------------------------------------------------------
// ------------------------------------------------------------------------
struct Gap
{
    Gap(uint32_t idx=0, float gap=0, float percent=0, float pos=0, float split=0) : 
        _idx(idx), _gap(gap), _percent(percent), _split(split)
    {}

    uint32_t _idx;
    float    _gap;
    float    _percent;
    float    _split;
};

// ------------------------------------------------------------------------
// ------------------------------------------------------------------------
template <class qtidx_t>
uint32_t QuadTree<qtidx_t>::subdivide(Search<qtidx_t> *ctx,
                                      vector<Pt> &points, 
                                      uint32_t depth, 
                                      uint32_t max_depth,
                                      uint32_t keep_count,
                                      bool     pretend)
{
    _total_num_points = (uint32_t)points.size();
    assert(_total_num_points > 0);

    _num_children = 0;
    _min_rank = points[0].r;

    // recompute box so that it is as tight as possible
    // ------------------------------------------------
    _box.reset();
    for (uint32_t i=0; i<_total_num_points; ++i)
        _box.add(points[i]);
            
    if (_total_num_points <= s_cell_size || supertight(_box) || depth == max_depth)
    {
        // either reached the threshold to stop subdividing, or
        // points close enough (in 2D) we can't discriminate... stop subdividing
        // ---------------------------------------------------------------------
        bool final_leaf = (max_depth != INT_MAX);
        _leaf = true;

        uint32_t new_cell_size = s_cell_size / 30;
        if (1 || !final_leaf && (_total_num_points <= new_cell_size || supertight(_box)))
        {
            // we won't subdivide further
            // --------------------------
            _storepoints(ctx, points, _total_num_points, keep_count, pretend, store_ranked | store_intervals); 
            vector<Pt>().swap(points); 
            return _num_children; 
        }

        // subdivide twice more... little hacky here
        // -----------------------------------------
        if (final_leaf)
            _storepoints(ctx, points, _total_num_points, keep_count, pretend, store_ranked);
        else
        {
            _storepoints(ctx, points, _total_num_points, keep_count, pretend, store_intervals);

            uint32_t s_cell_size_old = s_cell_size;

            s_cell_size = new_cell_size;
            subdivide(ctx, points, depth, depth+2, keep_count, pretend);

            assert(!_leaf); // otherwise it means we didn't subdivide further which should not happen
            _leaf = true;   // yes, we need to set it *again*

            s_cell_size = s_cell_size_old;
        }

        vector<Pt>().swap(points); 
        return _num_children; 
    }
        
    _leaf = false;

    auto split_fn = &QuadTree<qtidx_t>::_quadrant;

    vec2 c = center(_box); 

    uint32_t count[4];
    memset(count, 0, sizeof(count));
        
    for (uint32_t i=0; i<_total_num_points; ++i)
        count[(this->*split_fn)(points[i], c)]++;

    vector<Pt> child_points[4];

    for (uint32_t i=0; i<4; ++i)
        child_points[i].reserve(count[i]);

    for (uint32_t i=0; i<_total_num_points; ++i)
        child_points[(this->*split_fn)(points[i], c)].push_back(points[i]);

    // release memory, keep first keep_count points in _points_r
    // -------------------------------------------------------
    if (!_points_r)
        _storepoints(ctx, points, _total_num_points, keep_count, pretend, 0);
    vector<Pt>().swap(points); 

    // create child quadtrees, and  move points to child quadtrees
    // -----------------------------------------------------------
    for (uint32_t i=0; i<4; ++i)
        if (count[i])
            _child[i] = ctx->qt_alloc(); 
    
    // subdivide further (if above threshold)
    // --------------------------------------
    for (uint32_t i=0; i<4; ++i)
        if (_child[i])
            _num_children += 1 + ctx->qt_ptr(_child[i])->subdivide(ctx, child_points[i], depth+1, 
                                                                   max_depth, keep_count, pretend);
    return _num_children;
}

// ---------------------------------------------------------------
// Fills PointSet with points within target (in sorted rank order)
// ---------------------------------------------------------------
template <class qtidx_t>
template <class Target>
int QuadTree<qtidx_t>::search(Search<qtidx_t> *ctx, PointSet &pointset, 
                              const Target &target, int nth_rank) const
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
        return fill(ctx, pointset, target.count(), nth_rank);

    case ic_partial:
        return search_partial(ctx, pointset, target, nth_rank);

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
// Fills PointSet with points within target (in sorted rank order), but quit for 
// "partial" cells of depth >= s_depth_threshold
// -----------------------------------------------------------------------------
template <class qtidx_t>
template <class Target>
int QuadTree<qtidx_t>::search(Search<qtidx_t> *ctx, PointSet &pointset, const Target &target, 
                              uint32_t depth, int nth_rank) const
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
#if SORT_FULL
        ctx->add_full(this);
#else
        nth_rank = fill(ctx, pointset, target.count(), nth_rank);
#endif
        break;

    case ic_partial:
    {
        // some points from this cell match
        // --------------------------------
        if (_leaf || depth == s_depth_threshold)
        {
            // leaf cell... save work for later
            // --------------------------------
            ctx->add_partial(this, target.intersection_area(_box));
        } 
        else
        {
            for (uint32_t i=0; i<4; ++i)
                if (_child[i])
                    nth_rank = ctx->qt_ptr(_child[i])->search(ctx, pointset, target, depth + 1, nth_rank);
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
// when quadrant fully in target box, add points without checking rectangle
// returns true if at least one point had too high a rank to be added.
// ------------------------------------------------------------------------
template <class qtidx_t>
int QuadTree<qtidx_t>::fill(Search<qtidx_t> *ctx, PointSet &pointset, const uint32_t count, int nth_rank) const
{
    if (_min_rank < nth_rank)
    {
        if (_leaf || count <= _num_points)
        {
            // leaf cell... 
            Pt *p = ctx->pt_ptr(_points_r);
            nth_rank = pointset.add_multiple(p, _num_points);
        }
        else
        {
            for (uint32_t i=0; i<4; ++i)
                if (_child[i])
                    nth_rank = ctx->qt_ptr(_child[i])->fill(ctx, pointset, count, nth_rank);
        }
    }
    return nth_rank;
}
        
// ------------------------------------------------------------------------
// ------------------------------------------------------------------------
template <class qtidx_t>
template <class Target>
int QuadTree<qtidx_t>::search_partial(Search<qtidx_t> *ctx, PointSet &pointset, const Target &target, 
                                      int nth_rank) const
{
    if (_leaf)
    {
        // leaf cell... add points inside target
        // -------------------------------------
        bool      along_x;

        if (_num_points > s_interval_threshold && target.small_part_of(_box, along_x))
        {
            const Pt *hits[_num_intervals];
            uint32_t  num_hits = 0;
            
            if (along_x)
                _prep_intervals(hits, num_hits, _x_limit, ctx->pt_ptr(_points_x), 
                                &Target::below_x, &Target::above_x, target);
            else
                _prep_intervals(hits, num_hits, _y_limit, ctx->pt_ptr(_points_y), 
                                &Target::below_y, &Target::above_y, target);
            assert(num_hits);

            // finally add the points of intersecting intervals to pointset
            // ------------------------------------------------------------
            if (num_hits < s_max_num_hits)
            {
                if (num_hits > 1)
                    return pointset.add_intervals(hits, num_hits, target, nth_rank, 3);
                else
                    return pointset.add_multiple_t(hits[0], target);
            }
        }

        // intervals didn't work out. 
        // --------------------------
        if (_ranked)
        {
            // just use points sorted by rank, which were stored in this cell
            Pt *p = ctx->pt_ptr(_points_r);
            return pointset.add_multiple_t(p, target);
        }
        else
        {
            // ranked points are store in child cell. prep interval list with these ranked points,
            // filter fully inside first though!
            // ----------------------------------------------------------------------------------
            const Pt *hits[_num_intervals];
            uint32_t  num_hits = 0;
            
            assert(_num_children < _num_intervals);
            for (uint32_t i=0; i<4; ++i)
                if (_child[i])
                    nth_rank = ctx->qt_ptr(_child[i])->_prep_ranked_intervals(ctx, pointset, hits, num_hits,
                                                                              target, nth_rank);
            // finally add the points of intersecting intervals to pointset
            // ------------------------------------------------------------
            return num_hits ? pointset.add_intervals(hits, num_hits, target, nth_rank, 3) : nth_rank;
        }
    }
    else
    {
        // go down the tree so we test with smaller cells
        // ----------------------------------------------
        for (uint32_t i=0; i<4; ++i)
            if (_child[i])
                nth_rank = ctx->qt_ptr(_child[i])->search(ctx, pointset, target, nth_rank);
    }

    return nth_rank;
}

// ------------------------------------------------------------------------
// ------------------------------------------------------------------------
template <class qtidx_t>
void QuadTree<qtidx_t>::_storepoints(Search<qtidx_t> *ctx, 
                                     vector<Pt> &points, 
                                     uint32_t num_points,
                                     uint32_t keep_count,
                                     bool     pretend,
                                     uint32_t store_spec) // true -> 
{
    if (pretend)
        return;

    assert(num_points);
    if (store_spec & store_ranked)
    {
        _ranked = true;
        _num_points = num_points;
    }
    else
        _num_points = min(keep_count, num_points);

    assert(_points_r == 0);
    _points_r = ctx->pt_alloc(_num_points + 1);               // +1 for sentinel
    Pt *dest = ctx->pt_ptr(_points_r);
    memcpy(dest, &points[0], _num_points * sizeof(Pt));
    dest[_num_points] = Pt(INT_MAX);                         // the sentinel

    if (_leaf && (store_spec & store_intervals) && _num_points > s_interval_threshold)
    {
        // for leaf nodes, also store intervals of points sorted by x and by y
        // -------------------------------------------------------------------
        _store_intervals(ctx, points, _x_limit, _points_x, 
                         [](const Pt &p) -> float { return p.x; }, PtCompareX());

        _store_intervals(ctx, points, _y_limit, _points_y, 
                         [](const Pt &p) -> float { return p.y; }, PtCompareY());
    }
}
