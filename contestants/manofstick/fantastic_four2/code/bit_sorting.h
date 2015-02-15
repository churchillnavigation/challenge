#ifndef BIT_SORTING_H
#define BIT_SORTING_H

#include "common.h"
#include "bitset.h"

static void sort_by_rank(int total_points_count, Ranks& rankCollection)
{
	BitSet ws(total_points_count);
	for (auto it : rankCollection)
		ws.set(it);

	int idx = 0;
	for (auto it = ws.get_values_iterator(); !it.at_end(); ++it)
		rankCollection[idx++] = *it;
}

static void sort_by_coordinate(int point_count, Ranks& rankCollection, const std::vector<int32_t>& rank_idx_to_coord_sorted_idxes, const std::vector<int32_t>& coord_sorted_rank_idxes)
{
	BitSet ws(point_count);
	for (auto it : rankCollection)
		ws.set(rank_idx_to_coord_sorted_idxes[it]);

	int idx = 0;
	for (auto it = ws.get_values_iterator(); !it.at_end(); ++it)
		rankCollection[idx++] = coord_sorted_rank_idxes[*it];
}

#endif