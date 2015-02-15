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

static uint32_t s_cell_size = 300000;
static uint32_t s_cell_size_small = 8000;
static const uint32_t _num_intervals = 100;
static float    s_interval_percent = 0.28f;
static uint32_t s_depth_threshold = 5; 
static uint32_t s_max_num_hits = 10; 
static float    s_max_num_hits_factor = 1.3f; 
static uint32_t s_simul_ranked = 2;
static uint32_t s_simul_unranked = 4;
static float    s_area_threshold = 0.0035f; // ratio target_area / cloud_area
static float    s_log_dist = 2.2f;
static uint32_t s_interval_threshold = 20 * _num_intervals;

static int      s_filter = 0;
static float    s_area_filter_lb = 200.0f;
static float    s_area_filter_hb = 2000000000;

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
        
    uint32_t num_alloc() const { return _num_allocated; }

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

    vec2 operator/(const vec2 &o) { return vec2(x / o.x, y / o.y); }

    void set_min() { x = -FLT_MAX; y = -FLT_MAX;  }
    void set_max() { x =  FLT_MAX; y =  FLT_MAX;  }

    float x, y;
};

// ------------------------------------------------------------------------------
static vector<int8_t> s_ids;

// ------------------------------------------------------------------------------
struct Point_
{
    Point_() {} 
    Point_(const Point &p) : id(p.id), x(p.x), y(p.y), r(p.rank) {}

    int8_t    id;
    float     x, y;
    int32_t   r;       // rank
};

// ------------------------------------------------------------------------------
struct Pt
{
    Pt() {} 
    Pt(int32_t rank)   : r(rank) {}
    Pt(const Point_ &p) :  x(p.x), y(p.y), r(p.r) {}
    Pt(const Point &p) :  x(p.x), y(p.y), r(p.rank) {}

    float     x, y;
    int32_t   r;       // rank
};

// ------------------------------------------------------------------------------
template <class P>
struct CompareR
{
    bool operator()(const P &a, const P &b) const { return a.r < b.r; }
};

// ------------------------------------------------------------------------------
template <class P>
struct CompareX
{
    bool operator()(const P &a, const P &b) const { return a.x < b.x; }
};

// ------------------------------------------------------------------------------
template <class P>
struct CompareY
{
    bool operator()(const P &a, const P &b) const { return a.y < b.y; }
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
    vec2  dimensions() const { return vec2(width(), height()); }

    static const int efactor = 10;
    bool elongated() const {  float r = width() / height();  return r > efactor || r < 1.0f / efactor;  }

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

    TargetBase(const Rect &r, const uint32_t count, float base) : 
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
    float intersection_area(const Box &b)    const { return intersection_width(b) * intersection_height(b); }
    float intersection_width(const Box &b)   const { return std::min(_max.x, b._max.x) - std::max(_min.x, b._min.x); }
    float intersection_height(const Box &b)  const { return std::min(_max.y, b._max.y) - std::max(_min.y, b._min.y); }
    vec2  intersection(const Box &b)         const { return vec2(intersection_width(b), intersection_height(b)); }

    float area()   const { return width() * height(); }
    float width()  const { return _max.x - _min.x; }
    float height() const { return _max.y - _min.y; }
    float ratio()  const { return height() != 0 ? width() / height() : 1000000.0f; }

    inline const uint32_t count() const { return _count; }

    bool fully_in_x(const Box &b) const { return _min.x <= b._min.x && _max.x >= b._max.x; }
    bool fully_in_y(const Box &b) const { return _min.y <= b._min.y && _max.y >= b._max.y; }

    inline bool contains_x(const Pt &p)  const { return p.x >= _min.x && p.x <= _max.x; }
    inline bool contains_y(const Pt &p)  const { return p.y >= _min.y && p.y <= _max.y; }

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

    const vec2 &min() const { return _min; }
    const vec2 &max() const { return _max; }
    float interval_percent() const { return _interval_percent; }

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
    TargetW(const Rect &r, const uint32_t count, float interval_percent) :
        TargetBase(r, count, interval_percent) {}

    inline bool contains(const Pt &p) const
    {
        return p.y >= _min.y && p.y <= _max.y &&         // test y first
               p.x >= _min.x && p.x <= _max.x ;
    }    

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
    TargetT(const Rect &r, const uint32_t count, float interval_percent) : 
        TargetBase(r, count, interval_percent) {}

    inline bool contains(const Pt &p) const
    {
        return p.x >= _min.x && p.x <= _max.x &&        // test x first
               p.y >= _min.y && p.y <= _max.y;
    }    

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
            _point_array = new int32_t[_array_size+1]; // +1 because we are assigning _begin[_size] = INT_MAX
        }
        _count = count;
        _size = 0;
        _begin = _point_array + _array_size - _count; //  _count to leave room for initial push_back
    }

    inline int32_t nth_rank() const
    {
        return (_size >= _count) ? _begin[_count - 1] : INT_MAX;
    }

    inline int32_t nth_rank_no_check() const
    {
        return _begin[_count - 1];
    }

    inline int32_t nth_rank_no_check(uint32_t idx) const
    {
        return _begin[_count - idx];
    }


    int32_t operator[](int i) const { return _begin[i]; }

    uint32_t size() const { return _size; }

    // returns true if point can be inserted, false if its rank is too high to fit
    // ---------------------------------------------------------------------------
    inline int _insert(int32_t rank)
    {
        if (_begin <= _point_array + _count)
            _rewind();
        
        int32_t *cur  = _begin;
        if (_size)
        {
            _begin[_size] = INT_MAX; // so we stop there!
            while (*cur < rank)
            {
                *(cur - 1) = *cur;
                ++cur;
            }
        }
        --_begin;

        *(cur - 1) = rank;

        if (_size < _count)
            ++_size;

        return (_size >= _count) ? _begin[_count - 1] : INT_MAX;;
    }

protected:

    void _insert(const Pt *beg, uint32_t num_pts)
    {
        if (_begin <= _point_array + _count)
            _rewind();

        if (_size)
        {
            int32_t *last = _begin + _size;
            int32_t *cur  = _begin;
            const Pt *end = beg + num_pts;

            int32_t *insert    = _begin - num_pts;
            _begin -= num_pts;

            while (beg < end && (insert - _begin) < _count) // only first _count matter
            {
                if (cur < last && *cur < beg->r)
                {
                    *insert++ = *cur++;
                }
                else
                {
                    *insert++ = beg->r;
                    beg++;
                    if (_size < _count)
                        ++_size;
                }
            }
        }
        else
        {
            for (uint32_t i=0; i<num_pts; ++i)
                _begin[i] = beg[i].r;
            _size = num_pts;
        }
        assert(_size <= _count);
    }

    inline void _push_back(int32_t r)
    {
        assert(_begin + _size < &_point_array[_array_size]);
        _begin[_size++] = r;
    }

private:
    const int _num_counts = 50;
            
    void _rewind()
    {
        assert(_size <= _count);
        int32_t *new_begin = _point_array + _array_size - _count;

        for (uint32_t i=0; i<_size; ++i)
            new_begin[i] = _begin[i];
        _begin = new_begin;
    }

protected:
    uint32_t  _size;
    uint32_t  _count;

private:
    int32_t  *_begin;
    int32_t  *_point_array;
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
                for (; num_points > 0; --num_points)
                {
                    if (pt->r < rank)
                    {
                        if (target.contains(*pt))
                            rank = _insert(pt->r);
                        ++pt;
                    }
                    else
                    {
                        // done with this interval
                        // -----------------------
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
                        if (_insert(beg->r) != INT_MAX)
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
                        rank = _insert(beg->r);

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
                    _push_back(beg->r); 
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
            rank = _insert(beg->r);

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

class Search;

// ------------------------------------------------------------------------------
//                           Q U A D T R E E 
// ------------------------------------------------------------------------------
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
        _leaf = true;
        _has_intervals = false;
    }

    // ------------------------------------------------------------------------
    ~QuadTree()
    {
    }

    // ------------------------------------------------------------------------
    void subdivide(Search *ctx, vector<Pt> &points, uint32_t depth, uint32_t cell_size);

    template <class Target>
    int search(Search *ctx, PointSet &points, const Target &target, int nth_rank) const;

    template <class Target>
    int search(Search *ctx, PointSet &points, const Target &target, uint32_t depth, 
               int nth_rank) const;

    template <class Target>
    int search_partial(Search *ctx, PointSet &points, const Target &target, int nth_rank) const;
    
    int32_t min_rank() const { return _min_rank; }

    int fill(Search *ctx, PointSet &points, const uint32_t count, int nth_rank) const;

    void *operator new(size_t sz)  { return _allocator.alloc(); }
    void  operator delete(void *o) { _allocator.free((QuadTree *)o); }

private:
    uint32_t _interval_size() const { return _num_points / _num_intervals; }

    enum { store_intervals = 1, store_ranked = 2 };

    void _storepoints(Search *ctx, vector<Pt> &points, uint32_t num_points);

    // ------------------------------------------------------------------------
    template <class CoordAccessor, class Compare>
    void _store_intervals(Search *ctx, vector<Pt> &points, float *limits, uint32_t &pts,
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

            sort(p, p + cnt, CompareR<Pt>());
            p += cnt;
            *p++ = Pt(INT_MAX); // the sentinel
        }
    }

    // ------------------------------------------------------------------------
    void _prep_intervals(const Pt **hits, uint32_t &num_hits, const float *limits, 
                         const Pt *p, float target_min, float target_max) const
    {
        uint32_t cnt = _interval_size();
        const float *end = limits + _num_intervals;

        // skip below intervals intil we reach one that intersects the target
        // ------------------------------------------------------------------
        // TODO optimise this search for the first interval -> binary search
        // ------------------------------------------------------------------
        while (limits[1] < target_min)
        {
            p += cnt + 1;          // +1 for sentinel
            ++limits;
            assert(limits < end);
        }

        assert(!(limits[1] < target_min) && !(*limits > target_max));
        
        // accumulate in hits the pointers to points of intersecting intervals
        // -------------------------------------------------------------------
        do 
        {
            hits[num_hits++] = p;
            p += cnt + 1;          // +1 for sentinel
            ++limits;
        } while (limits < end && !(*limits > target_max));
        
        assert(limits == end || (*limits > target_max));
    }

    // ------------------------------------------------------------------------
    unsigned char _quadrant(const Pt& v, const vec2 &center) const
    {
        return ((v.x > center.x) ? 2 : 0) + ((v.y > center.y) ? 1 : 0);
    }

    unsigned char _hsplit(const Pt& v, const float (&split)[3]) const
    {
        if (v.x > split[1])
            return 2 + ((v.x > split[2]) ? 1 : 0);
        return (v.x > split[0]) ? 1 : 0;
    }

    unsigned char _vsplit(const Pt& v, const float (&split)[3]) const
    {
        if (v.y > split[1])
            return 2 + ((v.y > split[2]) ? 1 : 0);
        return (v.y > split[0]) ? 1 : 0;
    }

    Box           _box;
    QuadTree *    _child[4];
    uint32_t      _points_x;
    uint32_t      _points_y;
    uint32_t      _points_r;
    float         _x_limit[_num_intervals+1];
    float         _y_limit[_num_intervals+1];
    uint32_t      _num_points;
    bool          _leaf;
    bool          _has_intervals;
    int32_t       _min_rank; // lowest rank that this cell contains
    static ObjAllocator<QuadTree> _allocator;
};


ObjAllocator<QuadTree> QuadTree::_allocator(100000);

// ------------------------------------------------------------------------------
// ------------------------------------------------------------------------------
class Search : public SearchContext
{
public:
    typedef QuadTree Qt;
    static const uint32_t _num_qt = 4;

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
        _count(20),
        _root_qt(nullptr),
        _root_qt_small(nullptr)
    {
        vector<Pt> points;
        Box box;

        _fillpoints(points, points_begin, points_end, &box);
        _cloud_dims  = box.dimensions();          // without crazy points
        _cloud_area  = _cloud_dims.x * _cloud_dims.y;
        _cloud_ratio = _cloud_dims.x / _cloud_dims.y;
        _cloud_log2r = log2(_cloud_ratio);        

        if (abs(_cloud_log2r) > 4)
            s_interval_percent *= 1.0f + (abs(_cloud_log2r) - 4.0f) * 0.33f; //  * 0.66f;

        _points.reserve(_num_qt * (_num_points + 25000 * 21)); // stored on Qt intermediate nodes + sentinels
        _partial.reserve(1000);        // way too much, but who knows?

        vector<Pt> points2 = points;
        _root_qt = new QuadTree;
        _root_qt->subdivide(this, points, 0, s_cell_size);

        s_interval_threshold = INT_MAX; // no intervals for smaller quadtree
        _root_qt_small = new QuadTree;
        _root_qt_small->subdivide(this, points2, 0, s_cell_size_small);
    }

    // ------------------------------------------------------------------------
    template <class Target>
    void _search(const Target &target)
    {
        Qt *qt;
        float target_log2r = log2(_target_dims.x / _target_dims.y);

        if (abs(target_log2r - _cloud_log2r) < s_log_dist && target.area() / _cloud_area < s_area_threshold)
            qt = _root_qt_small;
        else
            qt = _root_qt;

        // fill in "fully inside" first, and then process the "partial"
        // ------------------------------------------------------------
        int nth_rank = qt->search(this, _pointset, target, 0, INT_MAX);

        size_t num_partial = _partial.size();
        if (num_partial > 1)
            std::sort(_partial.begin(), _partial.end(), 
                      [](const Partial &a, const Partial &b) -> bool { return a._ia > b._ia; });

        for (size_t i=0; i<num_partial; ++i)
            nth_rank = _partial[i]._qt->search_partial(this, _pointset, target, nth_rank);
    }

    ~Search()
    {
    }

    // ------------------------------------------------------------------------
    void _fillpoints(vector<Pt> &points, const Point *points_begin, const Point *points_end, Box *box)
    {
        vector<Point_> points_;
        points_.reserve(_num_points);
        for (const Point *p = points_begin; p != points_end; ++p)
            points_.emplace_back(Point_(*p));

        sort(points_.begin(), points_.end(), CompareR<Point_>());
        
        points.resize(0);
        points.reserve(_num_points);
        s_ids.resize(0);
        s_ids.reserve(_num_points);

        if (box)
            box->reset();
        
        // prepare sorted vector of points for subdivision
        // -----------------------------------------------
        for (uint32_t i=0; i<_num_points; ++i)
        {
            Point_ *p = &points_[i];

            if (abs(p->x) < 1.0e6 && abs(p->y) < 1.0e6)
            {
                if (box)
                    box->add(*p);
                points.emplace_back(Pt(*p));
            }
            s_ids.emplace_back(p->id);
        }
    }

    // ------------------------------------------------------------------------
    int32_t search(const Rect rect, const int32_t count, Point* out_points)
    {
        _target_dims = vec2(rect.hx - rect.lx, rect.hy - rect.ly);

#ifdef _DEBUG
        if (count <= 0 || !out_points || _target_dims.x < 0 || _target_dims.y < 0)
            return 0;
#endif
        _count = (uint32_t)count;

        reset();

        if (_target_dims.x > _target_dims.y)
            _search(TargetW(rect, (uint32_t)count, s_interval_percent));
        else
            _search(TargetT(rect, (uint32_t)count, s_interval_percent));
        
        // fill out the result buffer
        // --------------------------
        uint32_t num_points = (uint32_t)_pointset.size();
        if (num_points > (uint32_t)count)
            num_points = count;

        for (uint32_t i=0; i<num_points; ++i)
        {
            out_points[i].rank = _pointset[i];
            out_points[i].id = s_ids[_pointset[i]]; 
        }

        return num_points;
    }

    void reset()
    {
        _partial.resize(0);
        _pointset.reset(_count);
    }

    void add_partial(const Qt *qt, float ia)  
    { 
        _partial.emplace_back(Partial(qt, ia));
    }

    int add_full(const Qt *qt, int nth_rank)
    { 
        return qt->fill(this, _pointset, _count, nth_rank);
    }
    
    uint32_t pt_alloc(uint32_t count) 
    { 
        uint32_t res = (uint32_t)_points.size(); 
        _points.resize(res + count);
        return res;
    }

    Pt *pt_ptr(uint32_t idx) { return &_points[idx]; }

    uint32_t keep_count() { return _count; }
    uintptr_t num_points() const { return _num_points; }
    

protected:
    uint32_t           _count;
    uint32_t           _num_points;   // how many points this search context 
    PointSet           _pointset;     // contains only points which are within target    

    QuadTree *         _root_qt;      // root quadtree for our points
    QuadTree *         _root_qt_small; // root quadtree for our points
               
    vector<Pt>         _points;       // allocator for point arrays
    uint32_t           _points_prealloc;

    vector<Partial>    _partial;      // partial quadtrees to be processed in a second pass
    
    float              _median_leaf_cell_area;
    float              _center_average_area;

    vec2               _target_dims; 

    float              _cloud_area;
    float              _cloud_ratio;
    vec2               _cloud_dims;
    float              _cloud_log2r;
    float              _ratios[2];
};

// ------------------------------------------------------------------------------
extern "C" __declspec(dllexport) SearchContext* __stdcall create(const Point* points_begin,
                                                                 const Point* points_end)
{
    if (points_begin == points_end)
        return 0;

    return new Search(points_begin, points_end);
}

extern "C" __declspec(dllexport) int32_t search(SearchContext* sc, const Rect rect, 
                                                const int32_t count, Point* out_points)
{
    if (!sc)
        return 0;

    Search *s = static_cast<Search *>(sc);
    return s->search(rect, count, out_points);
}

extern "C" __declspec(dllexport) SearchContext* __stdcall destroy(SearchContext* sc)
{
    if (!sc)
        return nullptr;

    Search *s = static_cast<Search *>(sc);
    delete s;

    return nullptr;
}

// ------------------------------------------------------------------------
// ------------------------------------------------------------------------
void QuadTree::subdivide(Search *ctx,
                         vector<Pt> &points, 
                         uint32_t depth, 
                         uint32_t cell_size)
{
    uint32_t num_points = (uint32_t)points.size();
    assert(num_points > 0);

    _min_rank = points[0].r;

    // recompute box so that it is as tight as possible
    // ------------------------------------------------
    _box.reset();
    for (uint32_t i=0; i<num_points; ++i)
        _box.add(points[i]);
            
    if (num_points <= cell_size || supertight(_box))
    {
        // either reached the threshold to stop subdividing, or
        // points close enough (in 2D) we can't discriminate... stop subdividing
        // ---------------------------------------------------------------------
        _leaf = true;

        _storepoints(ctx, points, num_points); 
        vector<Pt>().swap(points); 
        return; 
    }
        
    _leaf = false;

    auto split_fn = &QuadTree::_quadrant;

    vec2 c = center(_box); 

    uint32_t count[4];
    memset(count, 0, sizeof(count));
        
    for (uint32_t i=0; i<num_points; ++i)
        count[(this->*split_fn)(points[i], c)]++;

    vector<Pt> child_points[4];

    for (uint32_t i=0; i<4; ++i)
        child_points[i].reserve(count[i]);

    for (uint32_t i=0; i<num_points; ++i)
        child_points[(this->*split_fn)(points[i], c)].push_back(points[i]);

    // release memory, keep first keep_count points in _points_r
    // -------------------------------------------------------
    _storepoints(ctx, points, std::min(num_points, ctx->keep_count()));
    vector<Pt>().swap(points); 

    // create child quadtrees, and  move points to child quadtrees
    // -----------------------------------------------------------
    for (uint32_t i=0; i<4; ++i)
        if (count[i])
            _child[i] =  new QuadTree; 
    
    // subdivide further (if above threshold)
    // --------------------------------------
    for (uint32_t i=0; i<4; ++i)
        if (_child[i])
            _child[i]->subdivide(ctx, child_points[i], depth+1, cell_size);
}

// ---------------------------------------------------------------
// Fills PointSet with points within target (in sorted rank order)
// ---------------------------------------------------------------
template <class Target>
int QuadTree::search(Search *ctx, PointSet &pointset, 
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
template <class Target>
int QuadTree::search(Search *ctx, PointSet &pointset, const Target &target, 
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
        nth_rank = fill(ctx, pointset, target.count(), nth_rank);
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
                    nth_rank = _child[i]->search(ctx, pointset, target, depth + 1, nth_rank);
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
int QuadTree::fill(Search *ctx, PointSet &pointset, const uint32_t count, int nth_rank) const
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
                    nth_rank = _child[i]->fill(ctx, pointset, count, nth_rank);
        }
    }
    return nth_rank;
}
        
// ------------------------------------------------------------------------
// ------------------------------------------------------------------------
template <class Target>
int QuadTree::search_partial(Search *ctx, PointSet &pointset, const Target &target, 
                                      int nth_rank) const
{
    if (_leaf)
    {
        // leaf cell... add points inside target
        // -------------------------------------
        if (_has_intervals)
        {
            bool along_x;

            if (target.small_part_of(_box, along_x))
            {
                const Pt *hits[_num_intervals];
                uint32_t  num_hits = 0;
            
                if (along_x)
                    _prep_intervals(hits, num_hits, _x_limit, ctx->pt_ptr(_points_x), 
                                    target.min().x, target.max().x);
                else
                    _prep_intervals(hits, num_hits, _y_limit, ctx->pt_ptr(_points_y), 
                                    target.min().y, target.max().y);

                assert(num_hits);

                bool has_rank = nth_rank != INT_MAX;
                if (num_hits < (has_rank ? s_max_num_hits : (uint32_t)(s_max_num_hits * s_max_num_hits_factor)))
                {
                    if (num_hits > 1)
                        return pointset.add_intervals(hits, num_hits, target, nth_rank, 
                                                      has_rank ? s_simul_ranked : s_simul_unranked);
                    else
                        return pointset.add_multiple_t(hits[0], target);
                }
            }
        }

        // intervals didn't work out. 
        // --------------------------
        // just use points sorted by rank, which were stored in this cell
        Pt *p = ctx->pt_ptr(_points_r);

        return pointset.add_multiple_t(p, target);
    }
    else
    {
        // go down the tree so we test with smaller cells
        // ----------------------------------------------
        for (uint32_t i=0; i<4; ++i)
            if (_child[i])
                nth_rank = _child[i]->search(ctx, pointset, target, nth_rank);
    }

    return nth_rank;
}

// ------------------------------------------------------------------------
// ------------------------------------------------------------------------
void QuadTree::_storepoints(Search *ctx, vector<Pt> &points, uint32_t num_points)
{
    assert(num_points);
    _num_points = num_points;

    assert(_points_r == 0);
    _points_r = ctx->pt_alloc(_num_points + 1);               // +1 for sentinel

    Pt *dest = ctx->pt_ptr(_points_r);
    memcpy(dest, &points[0], _num_points * sizeof(Pt));
    dest[_num_points] = Pt(INT_MAX);                         // the sentinel

    if (_leaf && _num_points > s_interval_threshold)
    {
        // for leaf nodes, also store intervals of points sorted by x and by y
        // -------------------------------------------------------------------
        _has_intervals = true;

        _store_intervals(ctx, points, _x_limit, _points_x, 
                         [](const Pt &p) -> float { return p.x; }, CompareX<Pt>());

        _store_intervals(ctx, points, _y_limit, _points_y, 
                         [](const Pt &p) -> float { return p.y; }, CompareY<Pt>());
    }
}
