#ifndef COMMON_H
#define COMMON_H

#include <vector>

#include "../../point_search.h"

#include "alignment_allocator.h"

typedef std::vector<float, AlignmentAllocator<float, 16>> Floats;
struct Coordinates
{
	Floats xs;
	Floats ys;
};

enum RectChecks
{
	CheckLx = 0x1,
	CheckHx = 0x2,
	CheckLy = 0x4,
	CheckHy = 0x8,
};

//#define INVISIBLE_WOMAN
//#define HUMAN_TORCH
#define THING
//#define MISTER_FANTASTIC

#ifdef INVISIBLE_WOMAN
const float UseBecauseAreaSpanned = 0.5f;
const int FirstDirection = 0; // 0 or 1
#endif
#ifdef HUMAN_TORCH
const float UseBecauseAreaSpanned = 0.5f;
const int FirstDirection = 1; // 0 or 1
#endif
#ifdef THING
const float UseBecauseAreaSpanned = 0.25f;
const int FirstDirection = 0; // 0 or 1
#endif
#ifdef MISTER_FANTASTIC
const float UseBecauseAreaSpanned = 0.25f;
const int FirstDirection = 1; // 0 or 1
#endif



const int PointCountThreshold = 10000;
const int MinimumSizeToUseMappingSort = 25000;
const float MaxGapFraction = 0.1f;

typedef std::vector<int8_t>     Ids;
typedef std::vector<int32_t>    Ranks;

struct Mappings
{
	Mappings(const Mappings& that) = delete;

	Mappings(int point_count, const Coordinates& coordinates, const Ids& ids)
		: points_count(point_count)
		, coordinates(coordinates)
		, ids(ids)
		, x_sorted_rank_idxes(point_count)
		, y_sorted_rank_idxes(point_count)
		, rank_idx_to_x_sorted_idxes(point_count)
		, rank_idx_to_y_sorted_idxes(point_count)
	{}

	int points_count;
	const Coordinates& coordinates;
	const Ids& ids;
	std::vector<int32_t> x_sorted_rank_idxes;
	std::vector<int32_t> y_sorted_rank_idxes;
	std::vector<int32_t> rank_idx_to_x_sorted_idxes;
	std::vector<int32_t> rank_idx_to_y_sorted_idxes;
};

static const int UNKNOWN_VALUE = INT_MAX - 1;
static const int END_OF_DATA = INT_MAX;

struct PointIterator
{
	int32_t best_next;
	virtual int32_t iterate() = 0;
};


#endif