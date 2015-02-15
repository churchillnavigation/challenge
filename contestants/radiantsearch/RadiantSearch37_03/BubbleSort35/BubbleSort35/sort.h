#pragma once

template <typename Data>
void QuickSortRecursive(Data *begin, Data *end)
{
	if (end > begin)
	{
		Data *pivot = begin;
		Data *left = begin + 1;
		Data *right = end;

		while (left < right)
		{
			if (*pivot < *left)
			{
				right--;
				std::swap(*left, *right);
			}
			else
			{
				left++;
			}
		}

		left--;

		std::swap(*begin, *left);
		QuickSortRecursive(begin, left);
		QuickSortRecursive(right, end);
	}
}

template <typename Data>
void QuickSort(Data *dataArray, int noof)
{
  QuickSortRecursive(dataArray, dataArray + noof);
}