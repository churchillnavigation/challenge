#ifndef RANK_ITERATORS_H
#define RANK_ITERATORS_H

#include <memory>
#include "common.h"
#include "bit_sorting.h"

struct RanksIterator
	: PointIterator
{
	virtual void reset() = 0;
};

struct DirectArrayRanksIterator
	: RanksIterator
{
	int idx;
	Ranks points;
	int max_index_check;

	DirectArrayRanksIterator(Ranks& points)
		: points(points)
	{
		this->points.swap(points);
	}

	void reset()
	{
		idx = -1;
		max_index_check = (int)points.size() - 1;
	}

	int32_t best_next()
	{
		if (idx >= max_index_check)
			return std::numeric_limits<int>::max();
		return points[idx + 1];
	}

	int32_t iterate()
	{
		if (++idx > max_index_check)
			return END_OF_DATA;
		return points[idx];
	}
};

struct CountingIterator
	: RanksIterator
{
	const int point_count;
	
	int idx;

	CountingIterator(int point_count)
		: point_count(point_count)
	{}

	void reset()
	{
		idx = -1;
	}

	int32_t best_next()
	{
		return idx + 1;
	}

	int32_t iterate()
	{
		if (++idx >= point_count)
			return END_OF_DATA;
		return idx;
	}
};

struct BitSetIterator
	: RanksIterator
{
	BitSet bitset;
	BitSet::values_iterator values_iterator;

	int idx;

	BitSetIterator(int point_count, Ranks& points)
		: bitset(point_count)
		, values_iterator(bitset.get_values_iterator()) // TODO: Not nice from API perspective
	{
		for (auto rank : points)
			bitset.set(rank);
	}

	void reset()
	{
		values_iterator = bitset.get_values_iterator();
	}

	int32_t best_next()
	{
		if (values_iterator.at_end())
			return END_OF_DATA;
		return *values_iterator;
	}

	int32_t iterate()
	{
		if (values_iterator.at_end())
			return END_OF_DATA;
		auto result = *values_iterator;
		++values_iterator;
		return result;
	}
};


struct UCharIterator
	: RanksIterator
{
	std::vector<unsigned char> values;
	int first_value;

	std::vector<unsigned char>::iterator it;
	int next_value;

	UCharIterator(Ranks& points)
	{
		first_value = points.front();
		auto last_value = first_value;
		for (auto i = 1; i < points.size(); ++i)
		{
			auto diff = points[i] - last_value - 1;
			while (diff >= 0xFF)
			{
				values.push_back(0xFF);
				diff -= 0xFF;
			}
			values.push_back(diff);
			last_value = points[i];
		}
		values.shrink_to_fit();
	}

	void reset()
	{
		next_value = first_value;
		it = values.begin();
	}

	int32_t best_next()
	{
		return next_value;
	}

	int32_t iterate()
	{
		if (next_value == END_OF_DATA)
			return END_OF_DATA;

		auto result = next_value;
		if (it == values.end())
			next_value = END_OF_DATA;
		else
		{
			auto increment = 1;
			auto tmp = *it++;
			while (tmp == 0xFF)
			{
				increment += 0xFF;
				tmp = *it++;
			}
			increment += tmp;
			next_value += increment;
		}

		return result;
	}
};

struct UShortIterator
	: RanksIterator
{
	std::vector<unsigned short> values;
	int first_value;

	std::vector<unsigned short>::iterator it;
	int next_value;

	UShortIterator(Ranks& points)
	{
		first_value = points.front();
		auto last_value = first_value;
		for (auto i = 1; i < points.size(); ++i)
		{
			auto diff = points[i] - last_value - 1;
			while (diff >= 0xFFFF)
			{
				values.push_back(0xFFFF);
				diff -= 0xFFFF;
			}
			values.push_back(diff);
			last_value = points[i];
		}
		values.shrink_to_fit();
	}

	void reset()
	{
		next_value = first_value;
		it = values.begin();
	}

	int32_t best_next()
	{
		return next_value;
	}

	int32_t iterate()
	{
		if (next_value == END_OF_DATA)
			return END_OF_DATA;

		auto result = next_value;
		if (it == values.end())
			next_value = END_OF_DATA;
		else
		{
			auto increment = 1;
			auto tmp = *it++;
			while (tmp == 0xFFFF)
			{
				increment += 0xFFFF;
				tmp = *it++;
			}
			increment += tmp;
			next_value += increment;
		}

		return result;
	}
};


std::shared_ptr<RanksIterator> ranks_iterator_factory(int total_points_count, Ranks& points, int depth)
{
	if (points.size() > MinimumSizeToUseMappingSort)
		sort_by_rank(total_points_count, points);
	else
		concurrency::parallel_sort(points.begin(), points.end(), [](int32_t a, int32_t b){ return a < b; });

	if (depth == 0)
		return std::shared_ptr<RanksIterator>(new CountingIterator(total_points_count));

	if (depth <= 2)
		return std::shared_ptr<RanksIterator>(new BitSetIterator(total_points_count, points));

	if (depth < 7)
		return std::shared_ptr<RanksIterator>(new UCharIterator(points));

	if (depth < 15)
		return std::shared_ptr<RanksIterator>(new UShortIterator(points));

	return std::shared_ptr<RanksIterator>(new DirectArrayRanksIterator(points));
}


#endif