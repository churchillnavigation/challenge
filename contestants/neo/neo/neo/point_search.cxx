#include "../point_search.h"
#include <vector>
#include <algorithm>
#include <cstring>
#include <cfloat>
#include <cassert>
#include <btree_set.h>

using namespace std;
using namespace btree;


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

    float x, y;
};

// ------------------------------------------------------------------------------
class vec2r : public vec2
{
public:
    vec2r() {}
    vec2r(float _x, float _y, int32_t _r) : vec2(_x, _y), r(_r) {}

    vec2r operator+(const vec2r& v) const { return vec2r(x + v.x, y + v.y, r + v.r); }
    vec2r operator-(const vec2r& v) const { return vec2r(x - v.x, y - v.y, r - v.r); }

    vec2r operator*(float f) const { return vec2r(x * f, y * f, iround(r * f)); }

    void set_min(bool set_r) { x = -FLT_MAX; y = -FLT_MAX; if (set_r) r = -INT_MAX; }
    void set_max(bool set_r) { x =  FLT_MAX; y =  FLT_MAX; if (set_r) r =  INT_MAX; }

    int32_t   r;       // rank
};

// ------------------------------------------------------------------------------
struct Pt
{
    Pt() {}
    Pt(const Point &p) : _id(p.id), _pos(p.x, p.y, p.rank) {}

    void copy(Point &p) const { p.id = _id; p.rank = _pos.r; p.x = _pos.x; p.y = _pos.y; }

    vec2r  _pos;
    int8_t _id;
};

// ------------------------------------------------------------------------------
struct PtCompare
{
    bool operator()(const Pt &a, const Pt &b) const { return a._pos.r < b._pos.r; }
};

// ------------------------------------------------------------------------------
class Box
{
public:
    Box() { _min.set_max(true); _max.set_min(true); }
    
    void reset(bool r) { _min.set_max(r); _max.set_min(r); }

    void add(const vec2r& v, bool set_r) 
    { 
        if (v.x < _min.x)  _min.x = v.x;
        if (v.y < _min.y)  _min.y = v.y;
        if (set_r && v.r < _min.r) _min.r = v.r;

        if (v.x > _max.x)  _max.x = v.x;
        if (v.y > _max.y)  _max.y = v.y;
        if (set_r && v.r > _max.r)  _max.r = v.r;
    }

    Box divide(unsigned char quadrant)
    {
        Box res = *this;
        vec2r halfsize = (_max - _min) * .5f;

        if (quadrant & 4)
            res._min.r += halfsize.r;
        else
            res._max.r -= halfsize.r;

        if (quadrant & 2)
            res._min.x += halfsize.x;
        else
            res._max.x -= halfsize.x;

        if (quadrant & 1)
            res._min.y += halfsize.y;
        else
            res._max.y -= halfsize.y;

        return res;
    }

    friend vec2r center(const Box &b)     { return (b._min + b._max) * 0.5f; }
    friend bool supertight(const Box &b)  { return almostEqual(b._min.x, b._max.x, 8) && 
                                                   almostEqual(b._min.y, b._max.y, 8); }
    friend class Box2d;
private:
    vec2r _min; 
    vec2r _max;   // actually half size (from center)
};

// ------------------------------------------------------------------------------
enum intersect_code
{
    ic_outside,
    ic_partial,
    ic_inside
};

// ------------------------------------------------------------------------------
class Box2d
{
public:
    Box2d(const Rect &r) : _min(r.lx, r.ly), _max(r.hx, r.hy) {}
    
    intersect_code intersect(const Box &b) const
    {
        if (b._min.x > _max.x || b._max.x < _min.x ||
            b._min.y > _max.y || b._max.y < _min.y)
            return ic_outside;
        
        if (b._min.x >= _min.x && b._max.x <= _max.x &&
            b._min.y >= _min.y && b._max.y <= _max.y)
            return ic_inside;

        return ic_partial;
    }

    bool contains(const Pt &p) const
    {
        return p._pos.x >= _min.x && p._pos.x <= _max.x &&
               p._pos.y >= _min.y && p._pos.y <= _max.y;
    }

private:
    vec2 _min; 
    vec2 _max;
};

// ------------------------------------------------------------------------------
class PointSet : public btree_set<Pt, PtCompare, std::allocator<Pt>, 512> 
{
public:
    PointSet(uint32_t count) : 
        _num_elems(0), _nth(INT_MAX), _high_bound(INT_MAX), _count(count) {}

    void add(const Pt &point)
    { 
        if (point._pos.r > _high_bound)
            return;

        _num_elems++;
        this->insert(point); 
        if (_nth < INT_MAX)
        {
            if (point._pos.r < _nth)
                _nth = INT_MAX; // needs to be recomputed
        } 
        else if (_high_bound == INT_MAX)
            (void)nth_rank();
    }

    intptr_t nth_rank() const
    {
        if (_nth == INT_MAX && _num_elems >= _count)
        {
            // need to find it
            auto iter = begin(); 

            uint32_t count = _count;
            while(--count)
                ++iter;

            _high_bound = _nth = iter->_pos.r;
        }
        return _nth;
    }

private:
    uint32_t        _count;
    uint32_t        _num_elems;
    mutable int32_t _nth;         // cache nth_rank. count is constant for a run.
    mutable int32_t _high_bound;  // never need to insert a point ranked higher than _high_bound
};

// ------------------------------------------------------------------------------
template <class Iter>
class Octtree
{
public:
    Octtree(Iter first, Iter last)
    {
        memset(_child, 0, sizeof(_child)); 

        uint32_t num_points = (uint32_t)(last - first);
        _points.reserve(num_points);
        
        // initially, all points are in the first cell. Compute bounding box
        for (Iter p=first; p!=last; ++p)
        {
            _points.push_back(*p);
            _box.add(_points.back()._pos, true);
        }
        assert(num_points == _points.size());
    }

    Octtree(const Box &box, uint32_t num_points)
    {
        memset(_child, 0, sizeof(_child)); 
        _box = box;
        _points.reserve(num_points);
    }

    ~Octtree()
    {
        for (int i=0; i<8; ++i)
            delete _child[i];
    }

    void add_point(const Pt &p)
    {
        _points.push_back(p);
    }
        
    int32_t subdivide(size_t threshold)
    {
        uint32_t num_points = (uint32_t)_points.size();

        if (num_points <= threshold)
            return _sort();
        
        vector<unsigned char> quadrant;
        quadrant.resize(num_points);

        uint32_t count[8];
        
        while (true)
        {
            memset(count, 0, sizeof(count));
        
            vec2r c = center(_box);

            for (size_t i=0; i<num_points; ++i)
            {
                unsigned char q = _quadrant(_points[i]._pos, c);
                quadrant[i] = q;
                count[q]++;
            }

            // see if we should tighten the box to the actual points. 
            // should we tighten only the 2D box?
            // -----------------------------------------------------
            int num_busy_quadrants = 0;

            for (unsigned char i=0; i<4; ++i)
            {
                if (count[i] || count[i+4])
                    num_busy_quadrants++;
            }

            assert(num_busy_quadrants);
            if (num_busy_quadrants > 1)
                break;

            // all points in one quadrant. recompute box as points may be in same position,
            // but keep rank range
            // ----------------------------------------------------------------------------
            _box.reset(false);

            for (size_t i=0; i<num_points; ++i)
                _box.add(_points[i]._pos, false);
            
            if (supertight(_box))
            {
                // points close enough (in 2D) we can't discriminate... stop subdividing
                return _sort();; 
            }
        };

        // create child octtrees, and  move points to child octtrees
        // -----------------------------------------------------------
        for (unsigned char i=0; i<8; ++i)
            if (count[i])
                _child[i] = new Octtree(_box.divide(i), count[i]);

        for (size_t i=0; i<num_points; ++i)
            _child[quadrant[i]]->add_point(_points[i]);
        
        vector<Pt>().swap(_points); // release memory

        // check if we need to subdivide further
        // -------------------------------------
        _min_rank = INT_MAX;
        for (unsigned char i=0; i<8; ++i)
        {
            if (_child[i])
            {
                int32_t mrank = _child[i]->subdivide(threshold);
                if (mrank < _min_rank)
                    _min_rank = mrank;
            }
        }
        return _min_rank;
    }

    // main search entry point. parameters were validated.
    // --------------------------------------------------
    int32_t search(const Box2d &box2d, const uint32_t count, Point* out_points) const
    {
        PointSet points(count); // contains only points which are within box2d
        
        _populate(points, box2d, count);
        
        uint32_t num_points = (uint32_t)points.size();
        if (num_points > count)
            num_points = count;

        auto it = points.begin();

        for (uint32_t i=0; i<num_points; ++i)
            (it++)->copy(out_points[i]);

        return num_points;
    }

private:
    // this one checks for inclusion in target rectangle box2d
    // -------------------------------------------------------
    uint32_t _populate(PointSet &points, const Box2d &box2d, const uint32_t count) const
    {
        if (points.nth_rank() < _min_rank)
            return 0;

        intersect_code ic = box2d.intersect(_box);

        switch (ic)
        {
        case ic_inside:
            // all points from this cell are inside the target rectangle...
            // we just need the "count" with smallest rank
            // ------------------------------------------------------------
            return _fill(points, count);

        case ic_partial:
        {
            // some points from this cell match
            const uint32_t num_points = (uint32_t)_points.size();
            uint32_t num_filled = 0;

            if (num_points)
            {
                // leaf cell... add points inside box2d
                // ------------------------------------
                for (uint32_t i=0; i<num_points; ++i)
                {
                    const Pt &point = _points[i];
                    if (box2d.contains(point))
                    {
                        points.add(point);
                        num_filled++;
                        if (num_filled >= count) // yeah, they are sorted!
                            break;
                    }
                }
            } 
            else
            {
                // first 4 cells always contain lower rank points than last 4
                // ----------------------------------------------------------
                for (unsigned char i=0; i<4; ++i)
                    if (_child[i])
                        num_filled += _child[i]->_populate(points, box2d, count);

                if (num_filled < count)
                {
                    for (unsigned char i=4; i<8; ++i)
                        if (_child[i])
                            num_filled += _child[i]->_populate(points, box2d, count);
                }
            }
            return num_filled;
        }

        case ic_outside:
            // no point to add from this cell
            // ------------------------------
            break;

        default:
            assert(0);
            break;
        }
        return 0;
    }

    // when quadrant fully in target box, add points without checking rectangle
    // ------------------------------------------------------------------------
    uint32_t _fill(PointSet &points, const uint32_t count) const
    {
        if (points.nth_rank() < _min_rank)
            return 0;

        const uint32_t num_points = (uint32_t)_points.size();
        uint32_t num_filled;

        if (num_points)
        {
            // leaf cell... add "count" first points (points are sorted by rank)
            // -----------------------------------------------------------------
            num_filled = min(num_points, count);

            for (size_t i=0; i<num_filled; ++i)
                points.add(_points[i]);
        }
        else
        {
            num_filled = 0;

            // first 4 always contain lower rank points than last 4
            // ----------------------------------------------------
            for (unsigned char i=0; i<4; ++i)
                if (_child[i])
                    num_filled += _child[i]->_fill(points, count);

            if (num_filled < count)
            {
                for (unsigned char i=4; i<8; ++i)
                    if (_child[i])
                        num_filled += _child[i]->_fill(points, count);
            }
        }
        return num_filled;
    }
        
    unsigned char _quadrant(const vec2r& v, const vec2r &center) const
    {
        return ((v.r > center.r) ? 4 : 0) + ((v.x > center.x) ? 2 : 0) + ((v.y > center.y) ? 1 : 0);
    }

    int32_t _sort()
    {
        std::sort(_points.begin(), _points.end(), PtCompare());
        _min_rank = _points[0]._pos.r;
        return _min_rank;
    }

    Octtree     *_child[8];
    Box          _box;
    vector<Pt>   _points;
    int32_t      _min_rank; // lowest rank that this cell contains
};


struct SearchContext {};

// ------------------------------------------------------------------------------
class Search : public SearchContext
{
public:
    Search(const Point* points_begin, const Point* points_end) :
        _ot(points_begin, points_end)
    {
        _ot.subdivide(24);
    }

    int32_t search(const Rect rect, const int32_t count, Point* out_points)
    {
        //printf("count=%d\n", count);
        if (count <= 0 || !out_points || rect.lx > rect.hx || rect.ly > rect.hy)
            return 0;

        return _ot.search(Box2d(rect), (uint32_t)count, out_points);
    }

private:
    Octtree<const Point*> _ot;
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
    Search *s = static_cast<Search *>(sc);
    delete s;
    return nullptr;
}
