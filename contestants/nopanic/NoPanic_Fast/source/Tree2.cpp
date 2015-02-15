#include "Tree2.h"
#include <assert.h>
#include <cmath>
#include <iostream>
#include <cstring>
#include <queue>
#include <limits>
#include "Profiler.h"

#if defined(__SSSE3__) || defined(_MSC_VER)
#  include <tmmintrin.h>
#else
#  include <emmintrin.h>

// Helper, assuming that a == b actually
inline __m128i _mm_hadd_epi32(__m128i a, __m128i b)
{
	__m128i l = _mm_unpacklo_epi32(a, b); // a0 b0 a1 b1
	__m128i h = _mm_unpackhi_epi32(a, b); // a2 b2 a3 b3

	__m128i r = _mm_add_epi32(l,h); // a0+a2; b0+b2; a1+a3; b1 + b3;
	return r;
}
#endif

using namespace std;

inline bool inside(Rect r, Point p)
{
	return (r.lx <= p.x) & (p.x <= r.hx)
	     & (r.ly <= p.y) & (p.y <= r.hy);
}

inline bool inside1(Rect r, float x, float y)
{
	return (r.lx <= x) & (x <= r.hx)
	     & (r.ly <= y) & (y <= r.hy);
}

inline void inside4(Rect r, float *x, float *y, uint32_t *res)
{
#if 0 // TODO: make compiler parallelize this
	res[0] = inside1(r, x[0], y[0]);
	res[1] = inside1(r, x[1], y[1]);
	res[2] = inside1(r, x[2], y[2]);
	res[3] = inside1(r, x[3], y[3]);
#else
#define LOAD(P) tmp = _mm_load_ss(&r.P); auto P = _mm_shuffle_ps(tmp, tmp, 0);
	// TODO: ensure alignment for loads?
	__m128 tmp;
	LOAD(lx)
	LOAD(ly)
	LOAD(hx)
	LOAD(hy)
	
	__m128 x_ = _mm_load_ps(x);
	__m128 y_ = _mm_load_ps(y);

	__m128 ret = _mm_cmple_ps(lx, x_);
	ret = _mm_and_ps(ret, _mm_cmple_ps(x_, hx));
	ret = _mm_and_ps(ret, _mm_cmple_ps(ly, y_));
	ret = _mm_and_ps(ret, _mm_cmple_ps(y_, hy));
	_mm_store_ps((float*) res, ret);

#undef LOAD
#endif
}

inline bool overlaps(Rect a, Rect b)
{
	// http://stackoverflow.com/questions/306316/determine-if-two-rectangles-overlap-each-other
	
	return a.lx < b.hx && a.hx > b.lx
	    && a.ly < b.hy && a.hy > b.ly;
}

Tree2::Tree2(Rect span_):
	Tree(span_)
{
}

void Tree2::ensure_children()
{
	if (children[0])
		return;
	
	float mx = 0.5f * (span.lx + span.hx);
	float my = 0.5f * (span.ly + span.hy);
	
	float mx_ = nextafter(mx, span.hx);
	float my_ = nextafter(my, span.hy);
	
	Rect s1{ span.lx, span.ly, mx, my };
	Rect s2{ mx_, span.ly, span.hx, my };
	Rect s3{ span.lx, my_, mx, span.hy };
	Rect s4{ mx_, my_, span.hx, span.hy };

	children[0] = new Tree2(s1);
	children[1] = new Tree2(s2);
	children[2] = new Tree2(s3);
	children[3] = new Tree2(s4);
}

void Tree2::prepare()
{
	size_t target_size = (points.size() % 4 == 0) 
	                   ? points.size()
	                   : (points.size() + 4 - (points.size() % 4)); 
	for (size_t i = points.size(); i < target_size; i++)
	{
		Point p;
		p.rank = numeric_limits<int32_t>::max();
		
		points.push_back(p);
	}
	
	x.reserve(points.size());
	y.reserve(points.size());
	ranks.reserve(points.size());
	for (unsigned i = 0; i < points.size(); i++)
	{
		x.push_back(points[i].x);
		y.push_back(points[i].y);
		ranks.push_back(points[i].rank);
	}

	ranks4.reserve(points.size()/4);
	for (unsigned i = 0; i < points.size(); i+=4)
	{
		int32_t rank = numeric_limits<int32_t>::max();
		for (int j = 0; j < 4; j++)
		{
			if (points[i+j].rank == numeric_limits<int32_t>::max())
				break;
			
			rank = min(rank, points[i+j].rank);
		}
		ranks4.push_back(rank);
	}	

	if (children[0])
	{
		static_cast<Tree2*>(children[0])->prepare();
		static_cast<Tree2*>(children[1])->prepare();
		static_cast<Tree2*>(children[2])->prepare();
		static_cast<Tree2*>(children[3])->prepare();
	}
}

void Tree2::query2(Query2 &q)
{
	PROFILE(ctx->Queries ++)

	q.count = 0;
	q.min_rank = numeric_limits<int32_t>::min();

	q.points.reserve(256);
#if defined(USE_MORE_SIMD) || defined(Q2_INDIRECT)
	q.ranks.reserve(256);
#endif
#ifdef UPDATE_MIN_RANK
	q.node_min_rank.reserve(256);
	q.node_points.reserve(256);
#endif

	vector<Tree*> bfs;
	bfs.reserve(64);
	bfs.push_back(this);
	
	unsigned it = 0;
	
	const bool root_overlaps = overlaps(span, q.rect);
	
	while(it < bfs.size())
	{
		Tree2 *t = static_cast<Tree2*>(bfs[it++]);
		
		PROFILE(ctx->NodesVisited ++)
#ifdef USE_POINT_OVERLAP
		if (t->overlap.overlaps(q.rect))
			t->query_node(q);
#else
		t->query_node(q);
#endif
		
		if (t->children[0] && root_overlaps)
		{
			float x = 0.5f * (t->span.lx + t->span.hx);
			float y = 0.5f * (t->span.ly + t->span.hy);
		
			bool c1 = q.rect.lx <= x;
			bool c2 = q.rect.ly <= y;
			bool c1n = q.rect.hx >= x;
			bool c2n = q.rect.hy >= y;
			bool chk[] = {c1 && c2, c1n && c2, c1 && c2n, c1n && c2n};
		
			for (int i = 0; i < 4; i++)
			{
				if (chk[i] && (q.count < q.request || q.min_rank > t->children[i]->min_rank))
				{
					bfs.push_back(t->children[i]);
				}
				else if (!chk[i])
				{
					PROFILE(ctx->OverlapFailed++)
				}
			}
		}

	}
	PROFILE(ctx->PointsOutputted += q.points.size())
	PROFILE(ctx->PointsFound += q.count)
	PROFILE(if (q.points.size() > 70) ctx->SlowQueries++)
}

#ifdef Q2_INDIRECT
static 
//static __attribute__((noinline)) 
int process(Query2 &q, Point *points, Point **points_out, float *x, float *y, int32_t *rank, int32_t *rank_out)
#else
static
//static __attribute__((noinline))
int process(Query2 &q, Point *points, float *x, float *y, int32_t *rank)
#endif
{
#if !defined(Q2_INDIRECT) && defined(USE_SIMD)
	Point *points_out = points;
#endif

#ifdef USE_SIMD
	uint32_t res[4];
	inside4(q.rect, x, y, res);
	
#ifdef USE_MORE_SIMD
	// TODO: ensure alignment for loads?
	
	__m128i ones = _mm_set1_epi32(~0);
	
	__m128i rank_ = _mm_loadu_si128((__m128i*) rank);
	__m128i mask = _mm_loadu_si128((__m128i*) res);
	__m128i imask = _mm_xor_si128(mask, ones);

	__m128i max_rank = _mm_set1_epi32(numeric_limits<int32_t>::min());
	max_rank = _mm_and_si128(max_rank, imask);
	
	rank_ = _mm_and_si128(rank_, mask);
	rank_ = _mm_or_si128(rank_, max_rank);
	
	_mm_storeu_si128((__m128i*) rank_out, rank_);

#else	
	if (!res[0] && !res[1] && !res[2] && !res[3])
		return 0;
	

#ifdef Q2_INDIRECT
	rank_out[0] = res[0] ? rank[0] : numeric_limits<int32_t>::min();
	rank_out[1] = res[1] ? rank[1] : numeric_limits<int32_t>::min();
	rank_out[2] = res[2] ? rank[2] : numeric_limits<int32_t>::min();
	rank_out[3] = res[3] ? rank[3] : numeric_limits<int32_t>::min();
#else
	if (!res[0]) points[0].rank = numeric_limits<int32_t>::min();
	if (!res[1]) points[1].rank = numeric_limits<int32_t>::min();
	if (!res[2]) points[2].rank = numeric_limits<int32_t>::min();
	if (!res[3]) points[3].rank = numeric_limits<int32_t>::min();
#endif
#endif	
	
#ifdef USE_MORE_SIMD
	__m128i count = _mm_loadu_si128((__m128i*) res);
	count = _mm_srli_epi32(count, 31);
	count = _mm_hadd_epi32(count, count);
	count = _mm_hadd_epi32(count, count);
	
	union
	{
		uint64_t v;
		uint32_t l,h;
	} cnt;
	_mm_storel_epi64((__m128i*) &cnt.v, count);
	
	q.count += cnt.l;
	PROFILE(ctx->tmp1 += cnt.l)
	
	if (cnt.l <= 1)
	{
		__m128i ids = _mm_set_epi32(3, 2, 1, 0);
		ids = _mm_and_si128(ids, mask);

		// TODO: perhaps HADD will be faster?

		__m128i l = _mm_unpacklo_epi32(ids, ids); // 0 0 1 1
		__m128i h = _mm_unpackhi_epi32(ids, ids); // 2 2 3 3

		__m128i r = _mm_or_si128(l,h); // 0|2 0|2 1|3 1|3
		
		
		l = _mm_unpacklo_epi32(r, r); // 0|2 0|2 0|2 0|2
		h = _mm_unpackhi_epi32(r, r); // 1|3 1|3 1|3 1|3
		r = _mm_or_si128(l,h);
		
		union
		{
			uint64_t v;
			uint32_t l,h;
		} id;
		_mm_storel_epi64((__m128i*) &id.v, r);
		
		points_out[0] = &points[id.l];
		rank_out[0] = rank_out[id.l];
		
#ifndef UPDATE_MIN_RANK
		if (q.count <= q.request)
			q.min_rank = max(q.min_rank, rank_out[0]);
#endif
		
		return cnt.l;
	}
#else
	int n = 0;
	int s = 0;
	if (res[0]) {n ++; s = 0;}
	if (res[1]) {n ++; s = 1;}
	if (res[2]) {n ++; s = 2;}
	if (res[3]) {n ++; s = 3;}

	q.count += n;
	PROFILE(ctx->tmp1 += n)
#endif
	
#ifndef UPDATE_MIN_RANK
#if defined(USE_MORE_SIMD) || defined(Q2_INDIRECT)
	int32_t min_rank = max(rank_out[0], rank_out[1]);
	min_rank = max(min_rank, max(rank_out[2], rank_out[3]));
#else
	int32_t min_rank = max(points[0].rank, points[1].rank);
	min_rank = max(min_rank, max(points[2].rank, points[3].rank));
#endif
	
	if (q.count - 4 <= q.request)
		q.min_rank = max(q.min_rank, min_rank);
#endif
	
#ifndef USE_MORE_SIMD
	if (n == 1 && s != 0)
	{
#ifdef Q2_INDIRECT
		points_out[0] = &points[s];
		rank_out[0]   = rank_out[s];
#else
		points_out[0] = points[s];
#endif
		return 1;
	}
#endif	

#ifdef Q2_INDIRECT
	memcpy(points_out, points, sizeof(Point*) * 4);
	for (int i = 0; i < 4; i++)
		points_out[i] = &points[i];
#endif
	
	if (res[3]) return 4;
	if (res[2]) return 3;
	if (res[1]) return 2; 
	/*if (res[0])*/ return 1;  // TODO: this wont be reached actually
		
#else
	int ret = 0;
	int n   = 0;

	for (int i = 0; i < 4; i++)
	{
		if (inside1(q.rect, x[i], y[i]))
		{
			PROFILE(ctx->tmp1++)
			q.count ++;
			n ++;
			ret = i + 1;
			
#ifndef UPDATE_MIN_RANK
			if (q.count <= q.request)
				q.min_rank = max(q.min_rank, points[i].rank);
#endif
		}
		else
			points[i].rank = numeric_limits<int32_t>::max();
	}
	
	if (n == 1 && ret != 1)
	{
		points[0] = points[ret - 1];
		return 1;
	}
	
	return ret;
#endif
}

#ifdef UPDATE_MIN_RANK
// XXX: MSVC doesn't support noinline.
static //__attribute__((noinline)) 
void update_min_rank(Query2 &q, unsigned begin, unsigned end)
{
	int32_t min_rank = numeric_limits<int32_t>::min();
	unsigned n_points = 0;
	for (unsigned i = begin; i != end; i++)
	{
#ifndef Q2_INDIRECT
		Point p = q.points[i];
#else
		Point p = *q.points[i];
		p.rank  = q.ranks[i];
#endif

		if (p.rank == numeric_limits<int32_t>::max())
			continue;
#ifdef USE_SIMD
		if (p.rank == numeric_limits<int32_t>::min())
			continue;
#endif
	
		min_rank = p.rank;
		n_points ++;
	}
	
	auto pos = lower_bound(q.node_min_rank.begin(), q.node_min_rank.end(), min_rank) - q.node_min_rank.begin();
	
	q.node_min_rank.insert(q.node_min_rank.begin() + pos, min_rank);
	q.node_points.insert(q.node_points.begin() + pos, n_points);
	
	const auto size = q.node_min_rank.size();
	int n = 0;
	for (unsigned i = 0; i < size; i++)
	{
		q.min_rank = q.node_min_rank[i];
		n += q.node_points[i];
		
		if (n >= q.request)
			break;
	}
}
#endif

void Tree2::query_node(Query2 &q)
{
	PROFILE(ctx->tmp2 = 0)
	if (min_rank > q.min_rank && q.count >= q.request)
		return;
	
#ifdef UPDATE_MIN_RANK
	auto offset0 = q.points.size();
#endif

	auto offset  = q.points.size();
	q.points.resize(offset + PER_NODE);
#if defined(USE_MORE_SIMD) || defined(Q2_INDIRECT)
	q.ranks.resize(offset + PER_NODE);
#endif

	const size_t count = points.size();
	for (unsigned i = 0; i < count; i += 4)
	{
		PROFILE(ctx->TotalTests++)
		PROFILE(ctx->tmp1 = 0)

		if (q.count >= q.request && ranks4[i/4] > q.min_rank)
		{
			PROFILE(ctx->SkippedTests++)
			PROFILE(ctx->TotalTests   += (count - i) / 4 - 1)
			PROFILE(ctx->SkippedTests += (count - i) / 4 - 1)
			break;
		}
	
#ifndef Q2_INDIRECT
		PROFILE(ctx->BytesCopied += (sizeof(Point) * 4))
		memcpy(&q.points[offset], &points[i], sizeof(Point) * 4);
#endif
		int r;
#if defined(USE_MORE_SIMD) || defined(Q2_INDIRECT)
		r = process(q, &points[i], &q.points[offset], &x[i], &y[i], &ranks[i], &q.ranks[offset]);
#else
		r = process(q, &q.points[offset], &x[i], &y[i], NULL);
#endif
		offset += r;

		PROFILE(ctx->tmp2 ++)
		PROFILE(ctx->TestReturned += ctx->tmp1)
		PROFILE(if (r==0) ctx->EmptyTests++)
		PROFILE(if (ctx->tmp1 == 1) ctx->OneReturned++)
	}
	
#ifdef UPDATE_MIN_RANK
	if (offset0 != offset)
	{
		update_min_rank(q, offset0, offset);
	}
	else
	{
		PROFILE(ctx->EmptyNodes ++)
		PROFILE(ctx->EmptyNodeTests += ctx->tmp2)
	}
#endif
	
	q.points.resize(offset);

#if defined(USE_MORE_SIMD) || defined(Q2_INDIRECT)
	q.ranks.resize(offset);
#endif
}

