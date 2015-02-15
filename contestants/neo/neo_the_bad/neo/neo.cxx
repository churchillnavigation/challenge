/*
 * (c) Gregory Popovitch 2014-2015
 *     greg7mdp@gmail.com
 *
 */

#include "../point_search.h"
#include <vector>
#include <queue>
#include <algorithm>
#include <cstring>
#include <cfloat>
#include <cmath>
#include <climits>
#include <cassert>

using namespace std;

static uint32_t s_cell_size = 0;         
static uint32_t s_cell_size_main_l = 65000;
static uint32_t s_cell_size_main_s = 3500;
static uint32_t s_cell_size_side = 1000;
static uint32_t s_depth_threshold = 2; 
static uint32_t s_side_depth = 7;
static float    s_log_dist = 2.2f;

static uint32_t s_vec_threshold  = 6000;
static float    s_vec_percent    = 0.0050f;
static float    s_area_threshold = 0.01f;


static int      s_filter = 0;
static float    s_area_filter_lb = 200.0f;
static float    s_area_filter_hb = 2000000000;


static const  uint32_t s_n_leaf = 100;
static const  uint32_t s_n_node = 10;

static const  uint32_t s_count  = 20;

class Search;


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
inline uint32_t iceil(float x) { return (uint32_t)(x + 0.999f); }

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
    vec2 operator/(float f) const { return vec2(x / f, y / f); }

    vec2 operator/(const vec2 &o) { return vec2(x / o.x, y / o.y); }

    void set_min() { x = -FLT_MAX; y = -FLT_MAX;  }
    void set_max() { x =  FLT_MAX; y =  FLT_MAX;  }

    float x, y;
};

// ------------------------------------------------------------------------------
class ivec2
{
public:
    ivec2() {}

    uint32_t x, y;
};

// ------------------------------------------------------------------------------
static vector<int8_t> s_ids;

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
    Pt(float _x) : x(_x) {}
    explicit Pt(float _x, float _y, int32_t _r) : x(_x), y(_y), r(_r) {}
    explicit Pt(const Point_ &p) : r(p.r), x(p.x), y(p.y) {}
    
    vec2 pos() const { return vec2(x, y); }

    void copy(Point &p) const 
    { 
        p.id = id();
        p.rank = r; 
        //p.x = x; 
        //p.y = y; 
    }

    const int8_t id() const   { return s_ids[r]; }

    float     x, y;
    int32_t   r;       // rank
};

// ------------------------------------------------------------------------------
// x field actually contains y coordinates!
// ------------------------------------------------------------------------------
struct Pt_y
{
    Pt_y() {} 
    Pt_y(int32_t rank)   : r(rank) {}
    Pt_y(float x) : x(x) {}
    explicit Pt_y(const Pt &p) : r(p.r), x(p.y), y(p.x) {}

    void copy(Point &p) const 
    { 
        p.id = id();
        p.rank = r; 
        //p.x = y; 
        //p.y = x; 
    }

    const int8_t id() const   { return s_ids[r]; }

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
template <class T>
struct CompX
{
    bool operator()(const T &a, const T &b) const { return a.pos().x < b.pos().x; }
};

template <class T>
struct CompY
{
    bool operator()(const T &a, const T &b) const { return a.pos().y < b.pos().y; }
};

// ------------------------------------------------------------------------------
class Box
{
public:
    Box() { _min.set_max(); _max.set_min(); }

    Box(const vec2 &min, const vec2 &max) : _min(min), _max(max) {}
    
    void reset() { _min.set_max(); _max.set_min(); }

    template <class T>
    void add(const T &v) 
    { 
        if (v.x < _min.x)  _min.x = v.x;
        if (v.y < _min.y)  _min.y = v.y;

        if (v.x > _max.x)  _max.x = v.x;
        if (v.y > _max.y)  _max.y = v.y;
    }

    void add(const Box &o)
    {
        this->add<vec2>(o._min);
        this->add<vec2>(o._max);
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

    friend Box intersect(const Box &a, const Box &b)
    {
        Box res;

        res._min.x = std::max(a._min.x, b._min.x);
        res._min.y = std::max(a._min.y, b._min.y);
        res._max.x = std::min(a._max.x, b._max.x);
        res._max.y = std::min(a._max.y, b._max.y);
        return res;
    }

    bool empty()
    {
        return width() <= 0 || height() <= 0;
    }

    const vec2 &min() const { return _min; }
    const vec2 &max() const { return _max; }

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
class TargetX
{
public:
    TargetX(const Rect &r) :  _min(r.lx, r.ly), _max(r.hx, r.hy) { }

    bool check_y(float y) const { return y >= _min.y && y <= _max.y; }

    vec2     _min; 
    vec2     _max;
};

// ------------------------------------------------------------------------------
class TargetY
{
public:
    TargetY(const Rect &r) :  _min(r.ly, r.lx), _max(r.hy, r.hx) { }

    bool check_y(float y) const { return y >= _min.y && y <= _max.y; }

    vec2     _min; 
    vec2     _max;
};

// ------------------------------------------------------------------------------
class TargetBase
{
public:
    TargetBase() {}

    TargetBase(const Rect &r, const uint32_t count) : 
        _min(r.lx, r.ly), _max(r.hx, r.hy),
        _count(count)
    {
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

    const vec2 &min() const { return _min; }
    const vec2 &max() const { return _max; }

protected:
    vec2     _min; 
    vec2     _max;
    uint32_t _count;
};

// -------------------------- Wide ---------------------------------------------
class TargetW : public TargetBase
{
public:
    TargetW(const Rect &r, const uint32_t count) :
        TargetBase(r, count) {}

    inline bool contains(const Pt &pt) const
    {
        return pt.y >= _min.y && pt.y <= _max.y &&         // test y first
               pt.x >= _min.x && pt.x <= _max.x ;
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
    TargetT(const Rect &r, const uint32_t count) : 
        TargetBase(r, count) {}

    inline bool contains(const Pt &pt) const
    {
        return pt.x >= _min.x && pt.x <= _max.x &&        // test x first
               pt.y >= _min.y && pt.y <= _max.y;
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
class PointSetBase
{
public:
    PointSetBase() :  _count(20), _array_size(0), _point_array(0)
    {
        reset(_count);
    }

    PointSetBase(const PointSetBase &o) = delete; // no copy constructor
    PointSetBase& operator=(const PointSetBase &o) = delete;

    ~PointSetBase()
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

    int32_t operator[](int i) const { return _begin[i]; }

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

    const uint32_t& size() const { return _size; }

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
    uint32_t  _count;

private:
    uint32_t  _size;
    int32_t  *_begin;
    int32_t  *_point_array;
    uint32_t  _array_size;
};    

// ------------------------------------------------------------------------------
//                         P O I N T S E T
// ------------------------------------------------------------------------------
class PointSet : public PointSetBase
{
public:
    PointSet() {}

    PointSet(const PointSet &) = delete; // no copy constructor

    // -----------------------------------------------------------------------
    template <class Target>
    int add_multiple_t(const Pt *beg, const Target &target)
    { 
        if (size())
        {
            if (size() < _count)
            {
                for (; beg->r < INT_MAX; ++beg)
                {
                    if (target.contains(*beg))
                    {
                        if (_insert(beg->r) != INT_MAX)
                        {
                            // we reached size() == _count
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
            // size() == 0
            for (; beg->r < INT_MAX; ++beg)
            {
                if (target.contains(*beg))
                {
                    _push_back(beg->r); 
                    if (size() == _count)
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

        if (size() < _count)
        {
            if (size())
            {
                num_pts = min(num_pts, _count - size());
                _insert(beg, num_pts);
                beg += num_pts;
                rank = nth_rank();
                // no return, need to look at further points
            }
            else
            {
                // size() == 0
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

    int add_multiple(const Pt *beg)
    {
        int rank = nth_rank();
        for (; beg->r < rank; ++beg)
            rank = _insert(beg->r);
        return rank;
    }

    int add_multiple(const int32_t *beg)
    {
        int rank = nth_rank();
        for (; *beg < rank; ++beg)
            rank = _insert(*beg);
        return rank;
    }
};

class Node;

// ------------------------------------------------------------------------------
//                        S E A R C H C O N T E X T
// ------------------------------------------------------------------------------
struct SearchContext 
{
public:
    // ------------------------------------------------------------------------------
    struct Partial 
    {
        Partial() : _node(nullptr) {}
        Partial(const Node *node, float ia) : _node(node), _ia(ia) {} 

        const Node  *_node;
        float        _ia;    // intersection area
    };

    PointSet &pointset()  { return _pointset; }

    void add_partial(const Node *node, float ia) { _partial.emplace_back(Partial(node, ia));  }

protected:
    PointSet           _pointset;               // contains only points which are within target    
    vector<Partial>    _partial;      // partial quadtrees to be processed in a second pass
};

// ------------------------------------------------------------------------------
// ------------------------------------------------------------------------------
class Leaf
{
public:
    Leaf(Pt *first, uint32_t num_points) : 
        _num_points(num_points) 
    {
        assert(num_points <= s_n_leaf);

        for (uint32_t i=0; i<num_points; ++i)
        {
            _points[i] = first[i];
            _box.add(first[i].pos());
        }
        _points[num_points] = Pt(INT_MAX);
    }

    const Box &box() const { return _box; }
    vec2       pos() const { return center(_box); }

    const int32_t *  ranks() const 
    {
        static int32_t rank[s_count+1];
        uint32_t num_r = min(_num_points, s_count);

        for (uint32_t i=0; i<num_r; ++i)
            rank[i] = _points[i].r;
        rank[num_r] = INT_MAX;

        return &rank[0];
    }

    int32_t min_rank() const { return _points[0].r; }

    // ------------------------------------------------------------------------
    template <class Target>
    int add_points(Search *ctx, const Target &target, int rank) const
    {
        if (_points[0].r < rank)
        {
            intersect_code ic = target.intersect(_box);

            switch (ic)
            {
            case ic_inside:
                return ctx->pointset().add_multiple(&_points[0]);

            case ic_partial:
                return ctx->pointset().add_multiple_t(&_points[0], target);

            case ic_outside:
                break;
            }
        }

        return rank;
    }

private:
    Box       _box;
    Pt        _points[s_n_leaf + 1];     // sorted by rank, with terminating INT_MAX point
    uint32_t  _num_points;
};

static PointSet s_pointset;

// ------------------------------------------------------------------------------
// ------------------------------------------------------------------------------
class Node
{
public:
    Node(Node *first, uint32_t num_nodes) : 
        _first(first), _num_nodes(num_nodes), _has_leaves(false)
    {
        init(first, num_nodes);
    }

    Node(Leaf *first, uint32_t num_nodes) : 
        _first((Node *)first), _num_nodes(num_nodes), _has_leaves(true)
    {
        init(first, num_nodes);
    }

    template <class N>
    void init(N *first, uint32_t num_nodes)
    {
        s_pointset.reset(s_count);

        for (uint32_t i=0; i<num_nodes; ++i)
        {
            _box.add(first[i].box());
            s_pointset.add_multiple(first[i].ranks());
        }

        uint32_t num_r = min(s_pointset.size(), s_count);
        
        for (uint32_t i=0; i<num_r; ++i)
            _rank[i] = s_pointset[i];
        _rank[num_r] = INT_MAX;
    }

    // --------------------------------
    template <class Target>
    inline int check(Search *ctx, const Target &target, int rank, vector<const Node *> &out) const
    {
        if (_rank[0] < rank)
        {
            intersect_code ic = target.intersect(_box);

            switch (ic)
            {
            case ic_inside:
                return ctx->pointset().add_multiple(ranks());

            case ic_partial:
                if (!_has_leaves)
                    out.push_back(this);
                else
                {
                    Leaf *leaf = (Leaf *)_first;
            
                    for (uint32_t i=0; i<_num_nodes; ++i)
                        rank = leaf[i].add_points(ctx, target, rank);
                }
                break;
            }
        }

        return rank;
    }

    // -------------------------------- *this is a partial node which does not contain leaves
    template <class Target>
    int search(Search *ctx, const Target &target, int rank, vector<const Node *> &out) const
    {
        if (_rank[0] < rank)    // check rank again as it may have been lowered
            for (uint32_t i=0; i<_num_nodes; ++i)
                rank = _first[i].check(ctx, target, rank, out);
        return rank;
    }

    // ------------------------------------------------------------------------
    template <class Target>
    int search(Search *ctx, const Target &target, uint32_t depth, int rank) const
    {
        if (_rank[0] < rank)
        {
            intersect_code ic = target.intersect(_box);

            switch (ic)
            {
            case ic_inside:
                return ctx->pointset().add_multiple(ranks());

            case ic_partial:
                if (depth == s_depth_threshold)
                    ctx->add_partial(this, target.intersection_area(_box));
                else 
                    return search_partial(ctx, target, rank);
                break;
            
            case ic_outside:
                break;
            }
        }
        return rank;
    }

    // ------------------------------------------------------------------------
    template <class Target>
    int search_partial(Search *ctx, const Target &target, int rank) const
    {
        if (!_has_leaves)
        {
            for (uint32_t i=0; i<_num_nodes; ++i)
                rank = _first[i].search(ctx, target, s_depth_threshold + 1, rank);
        }
        else
        {
            Leaf *leaf = (Leaf *)_first;

            for (uint32_t i=0; i<_num_nodes; ++i)
                rank = leaf[i].add_points(ctx, target, rank);
        }

        return rank;
    }

    const Box &      box() const      { return _box; }
    vec2             pos() const      { return center(_box); }
    const int32_t *  ranks() const    { return &_rank[0]; }
    int32_t          min_rank() const { return _rank[0]; }

private:
    Box      _box;
    Node    *_first;
    uint32_t _num_nodes;
    int32_t  _rank[s_count + 1];
    bool     _has_leaves; // means that _first points to array of Leaf not Nodes
};


// ------------------------------------------------------------------------------
// ------------------------------------------------------------------------------
template <class Compare1, class Compare2, class Compare3, class P, class NODE>
uint32_t str(P *points, uint32_t num_t, vector<NODE> &nodes, uint32_t n, bool sort3, 
             float ratio = 1.0f)
{
    uint32_t num_nodes = 0;
    uint32_t capacity = (uint32_t)nodes.capacity();

    if (num_t <= n)
    {
        nodes.emplace_back(points, num_t);
        return 1;
    }

    sort(points, points + num_t, Compare1());

    float expected_num_nodes = (float)num_t / n;
    float along_x = sqrt(expected_num_nodes);
    along_x *= ratio;

    if (along_x < 1.0f)
        along_x = 1.0f;
    else if (along_x > expected_num_nodes)
        along_x = expected_num_nodes;

    uint32_t s_x   = (uint32_t)iround(along_x); // number of slices along x
    uint32_t num_x = iceil((float)num_t / s_x);

    for (uint32_t i=0; i<s_x; ++i)
    {
        uint32_t start_x = i * num_x;
        if (start_x >= num_t)
            break;

        uint32_t sz_x    = (i < s_x - 1) ? num_x : num_t - start_x;

        sort(&points[start_x], &points[start_x + sz_x], Compare2());

        uint32_t s_y = iceil((float)sz_x / n);
        for (uint32_t j=0; j<s_y; ++j)
        {
            uint32_t start_y = start_x + j * n;
            uint32_t sz_y    = (j < s_y - 1) ? n : sz_x - j * n;
            
            if (sort3)
                sort(&points[start_y], &points[start_y + sz_y], Compare3());

            nodes.emplace_back(&points[start_y], sz_y);
            num_nodes++;
        }
    }

    if ((uint32_t)nodes.capacity() != capacity)
        abort();

    return num_nodes;
}

// ------------------------------------------------------------------------------
// ------------------------------------------------------------------------------
class Rtree
{
public:
    Rtree() : _top(nullptr) {}
    
    void build(const vector<Leaf> &leaves, float factor)
    {
        _leaves = leaves;
        uint32_t num_leaves = (uint32_t)_leaves.size();

        _nodes.reserve((size_t)((float)num_leaves * 2.0f / s_n_node));
        uint32_t total_num_nodes = 0;
        uint32_t num_nodes  = 
            str<CompX<Leaf>, CompY<Leaf>, CompX<Leaf>, Leaf, Node>
            (&_leaves[0], num_leaves, _nodes, s_n_node, false, factor);

        while (num_nodes > 1)
        {
            uint32_t num_new_nodes = str<CompX<Node>, CompY<Node>, CompX<Node>, Node, Node>
                (&_nodes[total_num_nodes], num_nodes,  _nodes, s_n_node, false, 1.0);
            total_num_nodes += num_nodes;
            num_nodes = num_new_nodes;
            uint32_t next_size = total_num_nodes + iceil((float)total_num_nodes / s_n_node);
            if (_nodes.capacity() < next_size)
                _nodes.reserve(next_size);
        }
        
        total_num_nodes += num_nodes;
        assert(total_num_nodes == _nodes.size());

        _top = &_nodes.back();
    }

    vector<Node> _nodes;
    vector<Leaf> _leaves;

    Node *       _top;
};

// ------------------------------------------------------------------------------
// Single-threaded Search 
// ------------------------------------------------------------------------------
class Search : public SearchContext
{
public:

    // ------------------------------------------------------------------------
    Search(const Point *points_begin, const Point *points_end) :
        _num_points((uint32_t)(points_end - points_begin)),
        _median_leaf_cell_area(0.0f),
        _search_idx(0), _count(20),
        _low(0), _high(0), _small(0), _small_x(0), _small_y(0), _large(0), _super_large(0)
    {
        vector<Pt> points;

        _fillpoints(points, points_begin, points_end, true);

        _cloud_dims  = _box.dimensions();          // without crazy points
        _cloud_area  = _box.width() * _box.height();
        _cloud_ratio = _box.width() / _box.height();
        _cloud_log2r = log2(_cloud_ratio);        
        
        vector<Leaf> leaves;
        leaves.reserve(_num_points / s_n_leaf + 10 * s_n_leaf);
        uint32_t num_leaves = 
            str<CompareX<Pt>, CompareY<Pt>, CompareR<Pt>, Pt, Leaf>
            (&points[0], (uint32_t)points.size(), leaves, s_n_leaf, true);

        _rtree[0].build(leaves, 4.0f);
        _rtree[1].build(leaves, 1.0f);
        _rtree[2].build(leaves, 1.0f / 4.0f);

        _partial_nodes[0].reserve(10000);
        _partial_nodes[1].reserve(10000);
    }

    // ------------------------------------------------------------------------
    template <class Target>
    void _search(const Target &target)
    {
        Box box_int = intersect(_box, Box(target.min(), target.max()));

        float target_log2r = box_int.empty() ? log2(target.width() / target.height()) :
            log2(box_int.width() / box_int.height());
        float area_ratio = target.area() / _cloud_area;
        
        const Node *node;

        float log2r_diff = target_log2r - _cloud_log2r; // >0 means target wider than PC
        if (abs(log2r_diff) < 1.5f)
            node = _rtree[1]._top;
        else
        {
            if (log2r_diff < 0)
                node = _rtree[0]._top;
            else
                node = _rtree[2]._top;
        }

#if 1
        vector<const Node *> *next_out = &_partial_nodes[0];;
        vector<const Node *> *out      = &_partial_nodes[1];

        out->resize(0);
        int rank = node->search(this, target, INT_MAX, *out);
        size_t num_out = out->size();

        while (num_out)
        {
            next_out->resize(0);
            for (const Node *n : *out)
                rank = n->search(this, target, rank, *next_out);
            std::swap(out, next_out);
            num_out = out->size();
        }
            
#else
        int rank = node->search(this, target, 0, INT_MAX);

        size_t num_partial = _partial.size();
        if (num_partial > 1)
            std::sort(_partial.begin(), _partial.end(), 
                      [](const Partial &a, const Partial &b) -> bool { return a._ia > b._ia; });

        for (size_t i=0; i<num_partial; ++i)
            rank = _partial[i]._node->search_partial(this, target, rank);
#endif
    }

    ~Search()
    {
        printf("skinny=%d(%d/%d), small=%d, large=%d, _super_large=%d\n",
               _small_x + _small_y, _small_x, _small_y, _small, _large, _super_large);
    }

    // ------------------------------------------------------------------------
    int32_t search(const Rect rect, const uint32_t count, Point* out_points)
    {
        _target_dims = vec2(rect.hx - rect.lx, rect.hy - rect.ly);

#ifdef _DEBUG
        if (_target_dims.x < 0 || _target_dims.y < 0)
            return 0;
#endif
        
        _count = count;

        reset();
        
#ifdef _DEBUG
        _search_idx++;
#endif
        // actually do the search
        // ----------------------
        vec2 dims_percent = _target_dims / _cloud_dims;
        
        if (_target_dims.x > _target_dims.y)
            _search(TargetW(rect, count));
        else
            _search(TargetT(rect, count));

        // fill out the result buffer
        // --------------------------
        uint32_t num_points = _pointset.size();
        if (num_points > count)
            num_points = count;

        for (uint32_t i=0; i<num_points; ++i)
        {
            out_points[i].rank = _pointset[i];
            out_points[i].id = s_ids[_pointset[i]]; 
        }

        return num_points;
    }

    // ------------------------------------------------------------------------
    void _fillpoints(vector<Pt> &points, const Point *points_begin, const Point *points_end, bool skip_far)
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

        _box.reset();
        
        // prepare sorted vector of points for subdivision
        // -----------------------------------------------
        for (uint32_t i=0; i<_num_points; ++i)
        {
            Point_ *p = &points_[i];

            if (!skip_far || (abs(p->x) < 1.0e6 && abs(p->y) < 1.0e6))
            {
                _box.add(vec2(p->x, p->y));
                points.emplace_back(*p);
            }
            s_ids.emplace_back(p->id);
        }
    }

    void reset()
    {
        _partial.resize(0);
        _pointset.reset(_count);
    }

    uintptr_t num_points() const { return _num_points; }

protected:
    uint32_t           _count;
    uint32_t           _num_points;
    
    Rtree              _rtree[3];

    vector<const Node *> _partial_nodes[2];
               
    vector<Leaf>       _leaves;

    float              _median_leaf_cell_area;
    float              _center_average_area;
    uint32_t           _search_idx;
    uint32_t           _low;
    uint32_t           _high;
    uint32_t           _small, _small_x, _small_y;
    uint32_t           _large, _super_large;

    vec2               _target_dims;

    Box                _box;
    float              _cloud_area;
    float              _cloud_ratio;
    vec2               _cloud_dims;
    float              _cloud_log2r;
    float              _ratios[2];
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

    uint32_t vec_pc = 0;

    get_from_environment(s_depth_threshold,   "depth_threshold");
    get_from_environment(s_cell_size_main_l,  "cell_size_main_l");
    get_from_environment(s_cell_size_main_s,  "cell_size_main_s");
    get_from_environment(s_cell_size_side,    "cell_size_side");
    get_from_environment(s_vec_threshold,     "vec_threshold");
    get_from_environment(vec_pc,              "vec_percent");
    get_from_environment(s_area_threshold,    "area_threshold");
    get_from_environment(s_filter,            "filter");
    get_from_environment(s_area_filter_lb,    "area_filter_lb");
    get_from_environment(s_area_filter_hb,    "area_filter_hb");

    if (vec_pc)
        s_vec_percent = (float)vec_pc / 10000.0f;
    
    uintptr_t num_points = points_end - points_begin;

    return new Search(points_begin, points_end);
}

extern "C" __declspec(dllexport) int32_t search(SearchContext* sc, const Rect rect, 
                                                const int32_t count, Point* out_points)
{
    if (!sc)
        return 0;

#ifdef _DEBUG
    if (count <= 0 || !out_points)
        return 0;
#endif

    Search *s = static_cast<Search *>(sc);
    return s->search(rect, (uint32_t)count, out_points);
}

extern "C" __declspec(dllexport) SearchContext* __stdcall destroy(SearchContext* sc)
{
    if (!sc)
        return nullptr;

    Search *s = static_cast<Search *>(sc);
    delete s;

    return nullptr;
}
