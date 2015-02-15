#include "stdafx.h"

#include "tree.h"

//#define NOINLINE _declspec(noinline)
#define NOINLINE 

namespace cc
{

	CompoundSearchContext::CompoundSearchContext(std::vector<Point> & pts)
	{
		std::sort(pts.begin(), pts.end(), [](const Point & pt1, const Point & pt2)
		{
			return pt1.rank < pt2.rank;
		});
		std::vector<unsigned> blocks = { 100, 1000, 10000, 100000, 1000000, 10000000 };
		std::vector<unsigned> widths = { 2,   3,    2,     2,      2,      2 };
		std::vector<unsigned> levels = { 1,   2,    4,     6,      7,      8 };
		unsigned i = 0;
		Point * blockBegin = pts.data();
		Point * end = pts.data() + pts.size();
		while (blockBegin != end)
		{
			Point * blockEnd = pts.data() + blocks[i];
			if (blockEnd > end)
			{
				blockEnd = end;
			}
			ctxs.emplace_back(blockBegin, blockEnd, widths[i], widths[i], levels[i]);
			blockBegin = blockEnd;
			i++;
		}
	}
	int CompoundSearchContext::Search(float searchLx, float searchLy, float searchHx, float searchHy, const int32_t count, Point* out_points)
	{
		int found = 0;
		int i = 0;
		for (SearchContext & ctx : ctxs)
		{
			i++;
			found += ctx.Search(searchLx, searchLy, searchHx, searchHy, count - found, out_points + found);
			if (found == count)
			{
				break;
			}
			//if (i == 4)
			//{
			//	break;
			//}
		}
		//iterCnt[i]++;
		//if (found > 0)
		//{
		//Point * lastPt = out_points + found - 1;
		//std::ofstream out("count.txt", std::ios_base::app);
		//out << i << std::endl;
		//}
		return found;
	}

	CompoundSearchContext::~CompoundSearchContext()
	{
		//std::ofstream out("iterCounts.txt", std::ios_base::app);
		//out << std::endl;
		//for (auto it = iterCnt.cbegin(); it != iterCnt.cend(); ++it)
		//{
		//	out << it->first << ": " << it->second << std::endl;
		//}
	}

	struct GridElement
	{
		float lx, ly, hx, hy;
		const Point * begin;
		const Point * end;
		bool lowest;
		unsigned rawIndex;
		unsigned childIndexBegin;
		unsigned childIndexEnd;

		GridElement(unsigned rawIndex, unsigned childIndexBegin, unsigned childIndexEnd)
			: lowest(false), rawIndex(rawIndex), childIndexBegin(childIndexBegin), childIndexEnd(childIndexEnd)
		{
		}
		GridElement(float lx, float ly, float hx, float hy, const Point * begin, const Point * end, unsigned rawIndex)
			: lx(lx), ly(ly), hx(hx), hy(hy), begin(begin), end(end), rawIndex(rawIndex), lowest(true)
		{
		}
	};

	SearchContext::SearchContext(const Point * b, const Point * e, unsigned baseWidth, unsigned baseHeight, unsigned levels)
		: pts(b, e), levels(levels)
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

		std::vector<GridElement> tempElements;
		std::vector<std::pair<unsigned, unsigned> > levelsRange;

		unsigned width = 1;
		unsigned height = 1;
		unsigned childCount = baseWidth * baseHeight;
		for (unsigned level = 0; level < levels - 1; level++)
		{
			unsigned begin = tempElements.size();

			// create temp elements for higher level grids
			for (unsigned i = 0; i < width; i++)
			{
				for (unsigned j = 0; j < height; j++)
				{
					unsigned index = tempElements.size();
					tempElements.emplace_back(index, index * childCount + 1, index * childCount + childCount + 1);
				}
			}

			unsigned end = tempElements.size();
			levelsRange.emplace_back(begin, end);
			childrenToCheck.push_back(new unsigned[(end - begin)*2+1]);

			width *= baseWidth;
			height *= baseHeight;
		}

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
				tempElements.emplace_back(nextLx, nextLy, nextHx, nextHy, splitYBegin, splitYEnd, (unsigned)tempElements.size());

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
			return e.lowest && e.begin == e.end;
		});
		tempElements.erase(newEnd, tempElements.end());
		const unsigned cellCount = std::count_if(tempElements.begin(), tempElements.end(), [](const GridElement & e)
		{
			return e.lowest;
		});

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
			if (ge.lowest)
			{
				const Point * const newBegin = separatedPts.data() + separatedPts.size();
				separatedPts.insert(separatedPts.end(), ge.begin, ge.end);
				const Point * const newEnd = separatedPts.data() + separatedPts.size();
				separatedPts.push_back(separator);
				ge.begin = newBegin;
				ge.end = newEnd;
			}
		}
		pts.swap(separatedPts);

		// shrink bounding boxes
		for (GridElement & ge : tempElements)
		{
			if (ge.lowest)
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

				ge.lx = lx;
				ge.ly = ly;
				ge.hx = hx;
				ge.hy = hy;
			}
		}

		// update child grid references, and apply boundings of upper level grid
		auto searchGridElementIndex = [](const GridElement & ge, unsigned index)
		{
			return ge.rawIndex < index;
		};

		// remove all grid cells that has no children - level by level
		for (unsigned level = levelsRange.size(); level > 0; --level)
		{
			const auto & range = levelsRange[level - 1];

			auto newEnd = std::remove_if(tempElements.begin() + range.first, tempElements.end(), [&](const GridElement & ge) -> bool
			{
				auto begin = std::lower_bound(tempElements.begin() + range.second, tempElements.end(), ge.childIndexBegin, searchGridElementIndex);
				auto end = std::lower_bound(tempElements.begin() + range.second, tempElements.end(), ge.childIndexEnd, searchGridElementIndex);
				return !ge.lowest && begin == end;
			});

			tempElements.erase(newEnd, tempElements.end());
		}
		
		// update child indices
		for (GridElement & ge : tempElements)
		{
			if (!ge.lowest)
			{
				auto begin = std::lower_bound(tempElements.begin(), tempElements.end(), ge.childIndexBegin, searchGridElementIndex);
				auto end = std::lower_bound(tempElements.begin(), tempElements.end(), ge.childIndexEnd, searchGridElementIndex);
				ge.childIndexBegin = begin - tempElements.begin();
				ge.childIndexEnd = end - tempElements.begin();
			}
		}
		for (auto & range : levelsRange)
		{
			auto begin = std::lower_bound(tempElements.begin(), tempElements.end(), range.first, searchGridElementIndex);
			auto end = std::lower_bound(tempElements.begin(), tempElements.end(), range.second, searchGridElementIndex);
			range.first = begin - tempElements.begin();
			range.second = end - tempElements.begin();
		}

		// now all child references points to true indices - and no empty grid exists
		// update bounding boxes
		for (unsigned level = levelsRange.size(); level > 0; --level)
		{
			const auto range = levelsRange[level - 1];

			for (unsigned idx = range.first; idx < range.second; ++idx)
			{
				GridElement & ge = tempElements[idx];
				unsigned begin = ge.childIndexBegin;
				unsigned end = ge.childIndexEnd;

				float & lx = ge.lx;
				float & ly = ge.ly;
				float & hx = ge.hx;
				float & hy = ge.hy;
				lx = tempElements[begin].lx;
				ly = tempElements[begin].ly;
				hx = tempElements[begin].hx;
				hy = tempElements[begin].hy;
				for (unsigned childIdx = begin; childIdx < end; ++childIdx)
				{
					const GridElement & child = tempElements[childIdx];
					if (child.lx < lx)
						lx = child.lx;
					if (child.hx > hx)
						hx = child.hx;
					if (child.ly < ly)
						ly = child.ly;
					if (child.hy > hy)
						hy = child.hy;
				}
			}
		}

		// extract temp elements to native arrays
		for (const GridElement & ge : tempElements)
		{
			gridParams.push_back(ge.hx);
			gridParams.push_back(ge.hy);
			gridParams.push_back(-ge.lx);
			gridParams.push_back(-ge.ly);
			if (ge.lowest)
			{
				gridPoints.push_back(ge.begin - &pts.front());
			}
			else
			{
				gridPoints.push_back(-1);
				gridChildren.push_back(ge.childIndexBegin);
				gridChildren.push_back(ge.childIndexEnd);
			}
		}

		ptsParamsMemBase = new float[pts.size() * 4 + 16];
		const intptr_t ptsParamsOffset = ((intptr_t)ptsParamsMemBase) % 16;
		if (ptsParamsOffset == 0)
		{
			ptsParams = ptsParamsMemBase;
		}
		else
		{
			ptsParams = (float *)((char*)ptsParamsMemBase + (16 - ptsParamsOffset));
		}

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

		heapMemBase = new HeapItem[cellCount + 100];
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
		for (unsigned *& ptr : childrenToCheck)
		{
			delete[] ptr;
		}
	}

	NOINLINE void SearchContext::ProcessTopGrid(unsigned * out)
	{
		__m128 searchWindow = _mm_load_ps(this->searchWindow);
		__m128 gridWindow = _mm_load_ps(&gridParams[0]);
		// searchWindow: shx, slx, shy, sly
		// gridWindow:   lx,  hx,  ly,  hy
		__m128 result = _mm_cmplt_ps(gridWindow, searchWindow);
		const int intersect = _mm_testz_ps(result, result);
		if (intersect)
		{
			*out++ = gridChildren[0];
			*out++ = gridChildren[1];
		}
		*out = -1;
	}

	NOINLINE void SearchContext::ProcessIntermediateGrid(const unsigned * in, unsigned * out)
	{
		__m128 searchWindow = _mm_load_ps(this->searchWindow);
		while (*in != unsigned(-1))
		{
			unsigned begin = *in++;
			unsigned end = *in++;
			for (unsigned i = begin; i < end; ++i)
			{
				__m128 gridWindow = _mm_load_ps(&gridParams[i * 4]);
				// searchWindow: shx, slx, shy, sly
				// gridWindow:   lx,  hx,  ly,  hy
				__m128 result = _mm_cmplt_ps(gridWindow, searchWindow);
				const int intersect = _mm_testz_ps(result, result);
				if (intersect)
				{
					*out++ = gridChildren[i * 2];
					*out++ = gridChildren[i * 2 + 1];
				}
			}
		}
		*out = -1;
	}

	NOINLINE HeapItem * SearchContext::ProcessLowestGrid(const unsigned * in, HeapItem * heap)
	{
		HeapItem * heapHead = heap;
		__m128 searchWindow = _mm_load_ps(this->searchWindow);
		while (*in != unsigned(-1))
		{
			unsigned begin = *in++;
			unsigned end = *in++;
			for (unsigned i = begin; i < end; ++i)
			{
				__m128 gridWindow = _mm_load_ps(&gridParams[i * 4]);
				// searchWindow: shx, slx, shy, sly
				// gridWindow:   lx,  hx,  ly,  hy
				__m128 result = _mm_cmplt_ps(gridWindow, searchWindow);
				const int intersect = _mm_testz_ps(result, result);
				if (intersect)
				{
					heapHead->begin = gridPoints[i];
					heapHead++;
					// heap[heapHead].rank is set by WindHeapItem
				}
			}
		}
		return heapHead;
	}

	NOINLINE void SearchContext::FixHeap(HeapItem * begin, HeapItem *& end)
	{
		for (HeapItem * item = begin; item != end; ++item)
		{
			WindHeapItem(*item);
			//if (!WindHeapItem(*item))
			//{
			//	*item-- = *(--end);
			//}
		}
		for (HeapItem * item = begin; item != end; ++item)
		{
			if (item->rank == std::numeric_limits<int32_t>::max())
			{
				*item-- = *(--end);
			}
		}
	}


	NOINLINE void SearchContext::BuildHeap(HeapItem * begin, HeapItem * end)
	{
		for (unsigned i = ((end - begin) >> 3) + 8; i != -1; --i)
		{
			RecursiveFixHeapItem(i, begin, end);
		}
	}

	NOINLINE void SearchContext::FixEmptyRoot(HeapItem * begin, HeapItem *& end)
	{
		HeapItem * item = begin;
		unsigned idx = 0;
		for (;;)
		{
			HeapItem * firstChild = begin + idx * 8 + 1;
			HeapItem * lastChild = begin + idx * 8 + 8;

			if (firstChild >= end)
			{
				break;
			}

			HeapItem * minItem = firstChild;
			int32_t minValue = firstChild->rank;
			for (HeapItem * child = firstChild + 1; child < end && child <= lastChild; ++child)
			{
				const int32_t currValue = child->rank;
				if (currValue < minValue)
				{
					minItem = child;
					minValue = currValue;
				}
			}

			*item = *minItem;
			item = minItem;
			idx = minItem - begin;
			continue;
		}

		// now *item is an empty item, replace it with the last item of the heap
		*item = *(--end);

		while (idx > 0)
		{
			unsigned parentIdx = ((idx - 1) >> 3);
			HeapItem * parent = begin + parentIdx;
			if (item->rank < parent->rank)
			{
				std::swap(*item, *parent);
				item = parent;
				idx = parentIdx;
			}
			else
			{
				break;
			}
		}
	}

	NOINLINE void SearchContext::RecursiveFixHeapItem(unsigned idx, HeapItem * begin, HeapItem * end)
	{
		HeapItem * item = begin + idx;
		for (;;)
		{
			HeapItem * firstChild = begin + idx * 8 + 1;
			HeapItem * lastChild = begin + idx * 8 + 8;

			if (firstChild >= end)
			{
				break;
			}

			HeapItem * minItem = firstChild;
			int32_t minValue = firstChild->rank;
			for (HeapItem * child = firstChild + 1; child < end && child <= lastChild; ++child)
			{
				const int32_t currValue = child->rank;
				if (currValue < minValue)
				{
					minItem = child;
					minValue = currValue;
				}
			}

			if (minValue < item->rank)
			{
				std::swap(*item, *minItem);
				item = minItem;
				idx = minItem - begin;
				continue;
			}

			break;
		}
	}

	NOINLINE bool SearchContext::WindHeapItem(HeapItem & hi)
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

	NOINLINE const Point * SearchContext::PopHeapRoot(HeapItem * begin, HeapItem *& end)
	{

		const Point * ret = &pts[begin->begin];
		begin->begin++;

		if (!WindHeapItem(*begin))
		{
			FixEmptyRoot(begin, end);
		}
		else
		{
			RecursiveFixHeapItem(0, begin, end);
		}
		
		return ret;
	}

	NOINLINE int SearchContext::Search(float searchLx, float searchLy, float searchHx, float searchHy, const int32_t count, Point* out_points)
	{
		this->searchWindow[0] = searchLx;
		this->searchWindow[1] = searchLy;
		this->searchWindow[2] = -searchHx;
		this->searchWindow[3] = -searchHy;

		HeapItem * heapEnd;
		if (levels > 1)
		{
			ProcessTopGrid(childrenToCheck[0]);
			for (unsigned i = 1; i < levels - 1; ++i)
			{
				ProcessIntermediateGrid(childrenToCheck[i - 1], childrenToCheck[i]);
			}
			heapEnd = ProcessLowestGrid(childrenToCheck[levels - 2], heap);
		}
		else
		{
			static const unsigned top[] = { 0, 1, -1 };
			heapEnd = ProcessLowestGrid(top, heap);
		}

		FixHeap(heap, heapEnd);
		BuildHeap(heap, heapEnd);

		int found = 0;

		while (heap != heapEnd)
		{
			const Point * pt = PopHeapRoot(heap, heapEnd);

			*out_points++ = *pt;
			if (++found == count)
			{
				break;
			}
		}

		return found;
	}

}
