#include "stdafx.h"

#include "tree.h"

namespace cc
{

	struct GridElement
	{
		float lx, ly, hx, hy;
		const Point * begin;
		const Point * end;

		GridElement(float lx, float ly, float hx, float hy, const Point * begin, const Point * end)
			: lx(lx), ly(ly), hx(hx), hy(hy), begin(begin), end(end)
		{
		}
	};

	SearchContext::SearchContext(std::vector<Point> && in_pts)
		: pts(std::move(in_pts))
	{

		float lx, ly, hx, hy;
		Point * begin, * end;

		if (pts.empty())
		{
			lx = ly = -1;
			hx = hy = 1;
			begin = end = nullptr;
		}
		else
		{
			// calculate bounding
			lx = hx = pts[0].x;
			ly = hy = pts[0].y;

			for (const Point & p : pts)
			{
				if (p.x < lx)
					lx = p.x;
				if (p.x > hx)
					hx = p.x;
				if (p.y < ly)
					ly = p.y;
				if (p.y > hy)
					hy = p.y;
			}

			hx = std::nextafterf(hx, std::numeric_limits<float>::infinity());
			hy = std::nextafterf(hy, std::numeric_limits<float>::infinity());

			begin = &pts.front();
			end = &pts.back() + 1;
		}

		const unsigned width = 28;
		const unsigned height = 28;

		std::vector<GridElement> tempElements;

		auto sortPredX = [](const Point & p1, const Point & p2)
		{
			return p1.x < p2.x;
		};
		auto sortPredY = [](const Point & p1, const Point & p2)
		{
			return p1.y < p2.y;
		};
		auto searchPredX = [](const Point & p1, float f)
		{
			return p1.x < f;
		};
		auto searchPredY = [](const Point & p1, float f)
		{
			return p1.y < f;
		};
		const float stepX = (hx - lx) / width;
		const float stepY = (hy - ly) / height;

		std::sort(begin, end, sortPredX);
		float nextLx = lx;
		float nextHx = lx + stepX;
		Point * splitXBegin = begin;
		for (unsigned i = 0; i < width; ++i)
		{
			Point * splitXEnd = i < width - 1 ? std::lower_bound(splitXBegin, end, nextHx, searchPredX) : end;

			// now between splitXBegin and splitXEnd are the points for the first column;
			std::sort(splitXBegin, splitXEnd, sortPredY);
			float nextLy = ly;
			float nextHy = ly + stepY;
			Point * splitYBegin = splitXBegin;
			for (unsigned j = 0; j < height; ++j)
			{
				Point * splitYEnd = j < height - 1 ? std::lower_bound(splitYBegin, splitXEnd, nextHy, searchPredY) : splitXEnd;

				// now between splitYBegin and splitYEnd are the points with x and y coordinates falling into the node;
				tempElements.emplace_back(nextLx, nextLy, nextHx, nextHy, splitYBegin, splitYEnd);

				std::sort(splitYBegin, splitYEnd, [](const Point & pt1, const Point & pt2)
				{
					return pt1.rank < pt2.rank;
				});

				splitYBegin = splitYEnd;
				nextLy = nextHy;
				nextHy += stepY;
			}

			splitXBegin = splitXEnd;
			nextLx = nextHx;
			nextHx += stepX;
		}

		// remove empty grid cells
		auto newEnd = std::remove_if(tempElements.begin(), tempElements.end(), [](const GridElement & e)
		{
			return e.begin == e.end;
		});
		tempElements.erase(newEnd, tempElements.end());
		cellCount = tempElements.size();

		// insert separators between pts runs
		std::vector<Point> separatedPts;
		separatedPts.reserve(pts.size() + cellCount);
		Point separator;
		uint32_t v = -1;
		separator.x = *reinterpret_cast<float *>(&v);
		separator.y = *reinterpret_cast<float *>(&v);
		separator.rank = -1;
		for (GridElement & ge : tempElements)
		{
			const Point * const newBegin = separatedPts.data() + separatedPts.size();
			separatedPts.insert(separatedPts.end(), ge.begin, ge.end);
			const Point * const newEnd = separatedPts.data() + separatedPts.size();
			separatedPts.push_back(separator);
			ge.begin = newBegin;
			ge.end = newEnd;
		}
		pts.swap(separatedPts);

		// sort tempElements
		std::sort(tempElements.begin(), tempElements.end(), [](const GridElement & e1, const GridElement & e2)
		{
			return e1.begin->rank < e2.begin->rank;
		});

		// extract temp elements to native arrays
		for (const GridElement & ge : tempElements)
		{
			// calculate new bounding
			float lx, ly, hx, hy;
			lx = hx = ge.begin->x;
			ly = hy = ge.begin->y;
			for (const Point * pt = ge.begin; pt != ge.end; ++pt)
			{
				if (pt->x < lx)
					lx = pt->x;
				if (pt->x > hx)
					hx = pt->x;
				if (pt->y < ly)
					ly = pt->y;
				if (pt->y > hy)
					hy = pt->y;
			}

			gridParams.push_back(hx);
			gridParams.push_back(hy);
			gridParams.push_back(-lx);
			gridParams.push_back(-ly);
			gridPoints.push_back(ge.begin - &pts.front());
		}

		ptsParamsMemBase = new float[pts.size()*4 + 16];
		const intptr_t ptsParamsOffset = ((intptr_t)ptsParamsMemBase) % 16;
		ptsParams = (float *)((char*)ptsParamsMemBase + (16 - ptsParamsOffset));

		ptsRanks = new int32_t[pts.size()];
		size_t i = 0;
		for (const Point & pt : pts)
		{
			ptsParams[i * 4 + 0] = pt.x;
			ptsParams[i * 4 + 1] = pt.y;
			ptsParams[i * 4 + 2] = -pt.x;
			ptsParams[i * 4 + 3] = -pt.y;
			ptsRanks[i] = pt.rank;
			i++;
		}

		heapMemBase = new HeapItem[cellCount + 65];
		intptr_t heap1Offset = intptr_t(heapMemBase + 1) % 64;
		if (heap1Offset != 0)
		{
			heap = (HeapItem*)((char*)heapMemBase + (64 - heap1Offset));
		}
		else
		{
			heap = heapMemBase;
		}

	}

	SearchContext::~SearchContext()
	{

		delete[] heapMemBase;
		delete[] ptsParamsMemBase;
		delete[] ptsRanks;

	}

	unsigned SearchContext::ProcessGrid()
	{
		unsigned heapHead = 0;
		__m128 searchWindow = _mm_load_ps(this->searchWindow);
		for (unsigned i = 0; i < cellCount; ++i)
		{
			__m128 gridWindow = _mm_load_ps(&gridParams[i * 4]);
			// searchWindow: shx, slx, shy, sly
			// gridWindow:   lx,  hx,  ly,  hy
			__m128 result = _mm_cmplt_ps(gridWindow, searchWindow);
			const int intersect = _mm_testz_ps(result, result);
			if (intersect)
			{
				heap[heapHead].begin = gridPoints[i];
				heapHead++;
				// heap[heapHead].rank is set by WindHeapItem
			}
		}
		return heapHead;
	}

	unsigned SearchContext::FixHeap(unsigned heapHead)
	{
		unsigned newHeapHead = 0;
		for (unsigned i = 0; i < heapHead; ++i)
		{
			newHeapHead += WindHeapItem(heap[i]);;
		}
		return newHeapHead;
	}


	void SearchContext::BuildHeap(unsigned heapHead)
	{
		for (size_t i = heapHead / 8 + 8; i != -1; --i)
		{
			RecursiveFixHeapItem(i, heapHead);
		}
	}

	void SearchContext::FixEmptyRoot(unsigned & heapHead)
	{
		heap[0] = heap[--heapHead];
		RecursiveFixHeapItem(0, heapHead);

		//size_t idx = 1;
		//for (;;)
		//{
		//	const size_t idxLeft = 2 * idx;
		//	const size_t idxRight = 2 * idx + 1;

		//	if (idxRight < heapHead)
		//	{
		//		if (heap_less(heap[idxLeft], heap[idxRight]))
		//		{
		//			heap[idx] = heap[idxLeft];
		//			idx = idxLeft;
		//			continue;
		//		}
		//		else
		//		{
		//			heap[idx] = heap[idxRight];
		//			idx = idxRight;
		//			continue;
		//		}
		//	}
		//	else if (idxLeft < heapHead)
		//	{
		//		heap[idx] = heap[idxLeft];
		//		idx = idxLeft;
		//		continue;
		//	}
		//	else
		//	{
		//		// TODO FIX heap[idx] = heap[--heapHead];
		//		// ez utan meg javitani kell...
		//	}
		//	break;
		//}
	}

	void SearchContext::RecursiveFixHeapItem(unsigned idx, unsigned heapHead)
	{
		for (;;)
		{
			const unsigned firstChild = idx * 8 + 1;
			const unsigned lastChild = idx * 8 + 8;

			if (firstChild >= heapHead)
			{
				break;
			}

			unsigned minIdx = firstChild;
			int32_t minValue = heap[firstChild].rank;
			for (unsigned i = firstChild + 1; i < heapHead && i <= lastChild; ++i)
			{
				const int32_t currValue = heap[i].rank;
				if (currValue < minValue)
				{
					minIdx = i;
					minValue = currValue;
				}
			}

			if (minValue < heap[idx].rank)
			{
				std::swap(heap[idx], heap[minIdx]);
				idx = minIdx;
				continue;
			}

			break;
		}
	}

	bool SearchContext::WindHeapItem(HeapItem & hi)
	{
		unsigned begin = hi.begin;
		__m128 searchWindow = _mm_load_ps(this->searchWindow);
		for (;;)
		{
			__m128 ptParam = _mm_load_ps(ptsParams + begin * 4);
			if (ptParam.m128_u32[0] == uint32_t(-1))
			{
				break;
			}
			// ptSearchWindow: slx, sly, -shx, -shy
			// ptParam:         x, y, -x, -y
			__m128 result = _mm_cmplt_ps(ptParam, searchWindow); // inside search window if all false
			const int inside = _mm_testz_ps(result, result);
			if (inside)
			{
				hi.begin = begin;
				hi.rank = ptsRanks[begin];
				return true;
			}
			begin++;
		}
		hi.begin = begin;
		hi.rank = std::numeric_limits<int32_t>::max();
		return false;
	}

	const Point * SearchContext::PopHeapRoot(unsigned & heapHead)
	{

		const Point * ret = &pts[heap[0].begin];
		if (ret->rank != -1)
		{
			heap[0].begin++;
		}
		if (!WindHeapItem(heap[0]))
		{
			FixEmptyRoot(heapHead);
		}
		else
		{
			RecursiveFixHeapItem(0, heapHead);
		}
		
		return ret;
	}

	int SearchContext::Search(float searchLx, float searchLy, float searchHx, float searchHy, const int32_t count, Point* out_points)
	{
		this->searchWindow[0] = searchLx;
		this->searchWindow[1] = searchLy;
		this->searchWindow[2] = -searchHx;
		this->searchWindow[3] = -searchHy;

		unsigned heapHead = ProcessGrid();
		//return 0;
		unsigned newHeapHead = FixHeap(heapHead);
		//return 0;
		BuildHeap(heapHead);
		//heapHead = newHeapHead; // TODO keeping even past-the-end heap items.. requires additional checking, so not optimal.

		int found = 0;

		while (heapHead != 0)
		{
			const Point * pt = PopHeapRoot(heapHead);

			if (pt->rank != -1)
			{
				*out_points++ = *pt;
				if (++found == count)
				{
					break;
				}
			}
		}

		return found;
	}

}
