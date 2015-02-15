#include "stdafx.h"

#include "tree.h"

//#define NOINLINE _declspec(noinline)
#define NOINLINE 

namespace cc
{

	template<typename T>
	T * alignedAlloc(T *& alignedArray, size_t count, size_t byteAlignment)
	{
		T * memBase = new T[count + byteAlignment / sizeof(T) + 1];
		intptr_t offset = intptr_t(memBase) % byteAlignment;
		if (offset != 0)
		{
			alignedArray = (T*)((char*)memBase + (byteAlignment - offset));
		}
		else
		{
			alignedArray = memBase;
		}
		return memBase;
	}

	struct GridElement
	{
		float lx, ly, hx, hy; // full grid bounding
		float plx, ply, phx, phy; // bounding of points
		const Point * begin;
		const Point * end;
		unsigned rawIndex;
		unsigned childIndexBegin;
		unsigned childIndexEnd;

		GridElement(float lx, float ly, float hx, float hy, const Point * begin, const Point * end, unsigned childIndexBegin, unsigned childIndexEnd, unsigned rawIndex)
			: lx(lx), ly(ly), hx(hx), hy(hy), begin(begin), end(end), rawIndex(rawIndex), childIndexBegin(childIndexBegin), childIndexEnd(childIndexEnd)
		{
		}
		GridElement() {}
	};

	NOINLINE int SearchContext::MapFloatToInt(float value)
	{
		uint32_t val = *reinterpret_cast<uint32_t*>(&value);
		val ^= -int32_t(val >> 31) & ~0x80000000;
		return val;
	}

	static unsigned windCalls;
	static unsigned windSteps;

	SearchContext::SearchContext(const Point * b, const Point * e)
		: pts(b, e)
	{

		windCalls = 0;
		windSteps = 0;

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
		auto sortPredRank = [](const Point & p1, const Point & p2)
		{
			return p1.rank < p2.rank;
		};

		std::sort(pts.begin(), pts.end(), sortPredRank);

		std::vector<GridElement> tempElements;
		std::vector<std::pair<unsigned, unsigned> > levelsRange;

		const unsigned baseWidth = 2;
		const unsigned baseHeight = 2;
		unsigned width = 1;
		unsigned height = 1;
		const unsigned childCount = baseWidth * baseHeight;
		unsigned int ptCountOnLevel = 80;
		Point * ptsOnLevelBegin = begin;
		for (levels = 0; ptsOnLevelBegin != end; levels++)
		{

			unsigned levelGridBegin = tempElements.size();

			Point * ptsOnLevelEnd = ptsOnLevelBegin + ptCountOnLevel;
			if (ptsOnLevelEnd > end)
			{
				ptsOnLevelEnd = end;
			}

			if (end - ptsOnLevelEnd < ptsOnLevelEnd - ptsOnLevelBegin)
			{
				ptsOnLevelEnd = end;
			}

			const unsigned cellsOnThisLevel = width * height;
			const unsigned levelBase = tempElements.size();
			tempElements.resize(levelBase + cellsOnThisLevel);

			const float stepX = (hx - lx) / width;
			const float stepY = (hy - ly) / height;

			std::sort(ptsOnLevelBegin, ptsOnLevelEnd, sortPredX);
			float nextLx = lx;
			float nextHx = lx + stepX;
			Point * splitXBegin = ptsOnLevelBegin;
			for (unsigned i = 0; i < width; ++i)
			{
				Point * splitXEnd = i < width - 1 ? std::lower_bound(splitXBegin, ptsOnLevelEnd, nextHx, searchPredX) : ptsOnLevelEnd;

				// now between splitXBegin and splitXEnd are the points for the first column;
				std::sort(splitXBegin, splitXEnd, sortPredY);
				float nextLy = ly;
				float nextHy = ly + stepY;
				Point * splitYBegin = splitXBegin;
				for (unsigned j = 0; j < height; ++j)
				{
					Point * splitYEnd = j < height - 1 ? std::lower_bound(splitYBegin, splitXEnd, nextHy, searchPredY) : splitXEnd;

					// now between splitYBegin and splitYEnd are the points with x and y coordinates falling into the node;
					unsigned relIndex = 0;
					unsigned placement1 = 1;
					unsigned placement2 = baseHeight;
					unsigned value1 = j;
					unsigned value2 = i;
					while (value1 > 0 || value2 > 0)
					{
						unsigned value = value1 % baseHeight;
						value1 /= baseHeight;
						relIndex += placement1 * value;
						placement1 *= baseHeight * baseWidth;
						value = value2 % baseWidth;
						value2 /= baseWidth;
						relIndex += placement2 * value;
						placement2 *= baseWidth * baseHeight;
					}

					const unsigned index = levelBase + relIndex;
					tempElements[index] = GridElement(nextLx, nextLy, nextHx, nextHy, splitYBegin, splitYEnd, index * childCount + 1, index * childCount + childCount + 1, index);

					std::sort(splitYBegin, splitYEnd, sortPredRank);

					splitYBegin = splitYEnd;
					nextLy = nextHy;
					nextHy += stepY;
				}

				splitXBegin = splitXEnd;
				nextLx = nextHx;
				nextHx += stepX;
			}

			ptsOnLevelBegin = ptsOnLevelEnd;

			unsigned levelGridEnd = tempElements.size();
			levelsRange.emplace_back(levelGridBegin, levelGridEnd);

			width *= baseWidth;
			height *= baseHeight;
			ptCountOnLevel *= childCount;
		}

		// update child grid index reference, where no child grid exists
		for (GridElement & ge : tempElements)
		{
			if (ge.childIndexBegin >= tempElements.size())
			{
				ge.childIndexBegin = 0;
				ge.childIndexEnd = 0;
			}
			if (ge.childIndexEnd > tempElements.size())
			{
				ge.childIndexEnd = tempElements.size();
			}
		}

		// remove all grid cells that have no children - level by level, botom up
		auto searchGridElementIndex = [](const GridElement & ge, unsigned index)
		{
			return ge.rawIndex < index;
		};
		for (unsigned level = levelsRange.size(); level > 0; --level)
		{
			const auto & range = levelsRange[level - 1];

			auto newEnd = std::remove_if(tempElements.begin() + range.first, tempElements.begin() + range.second, [&](const GridElement & ge) -> bool
			{
				auto newChildIdxBegin = std::lower_bound(tempElements.begin() + range.second, tempElements.end(), ge.childIndexBegin, searchGridElementIndex) - tempElements.begin();
				auto newChildIdxEnd = std::lower_bound(tempElements.begin() + range.second, tempElements.end(), ge.childIndexEnd, searchGridElementIndex) - tempElements.begin();
				return ge.begin == ge.end && newChildIdxBegin == newChildIdxEnd;
			});

			tempElements.erase(newEnd, tempElements.begin() + range.second);
		}

		// update child indices of grid elements - required because of the removed grid cells
		for (GridElement & ge : tempElements)
		{
			if (ge.childIndexBegin != ge.childIndexEnd)
			{
				auto newChildIdxBegin = std::lower_bound(tempElements.begin(), tempElements.end(), ge.childIndexBegin, searchGridElementIndex);
				auto newChildIdxEnd = std::lower_bound(tempElements.begin(), tempElements.end(), ge.childIndexEnd, searchGridElementIndex);
				ge.childIndexBegin = newChildIdxBegin - tempElements.begin();
				ge.childIndexEnd = newChildIdxEnd - tempElements.begin();
			}
		}
		// update indices in levelsRange as well
		for (auto & range : levelsRange)
		{
			auto begin = std::lower_bound(tempElements.begin(), tempElements.end(), range.first, searchGridElementIndex);
			auto end = std::lower_bound(tempElements.begin(), tempElements.end(), range.second, searchGridElementIndex);
			range.first = begin - tempElements.begin();
			range.second = end - tempElements.begin();
		}

		// statistics
		//for (size_t i = 0; i < levelsRange.size(); ++i)
		//{
		//	unsigned levelCellCount = levelsRange[i].second - levelsRange[i].first;
		//	float pointCountSum = 0;
		//	for (unsigned j = levelsRange[i].first; j < levelsRange[i].second; ++j)
		//	{
		//		pointCountSum += tempElements[j].end - tempElements[j].begin;
		//	}
		//	const float averagePoints = pointCountSum / levelCellCount;
		//	pointCountSum = 0;
		//	for (unsigned j = levelsRange[i].first; j < levelsRange[i].second; ++j)
		//	{
		//		float x = fabs(averagePoints - (tempElements[j].end - tempElements[j].begin));
		//		pointCountSum += x;
		//	}
		//	const float stDev = pointCountSum / levelCellCount;

		//	printf("Level %d, cells: %d, avg pts: %f, stdev: %f\n", i, levelCellCount, averagePoints, stDev);
		//}

		// insert separators between pts runs
		std::vector<Point> separatedPts;
		separatedPts.reserve(pts.size() + tempElements.size());
		Point separator;
		separator.x = std::numeric_limits<float>::min();
		separator.y = std::numeric_limits<float>::min();
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

		// now all child references points to true indices - and no empty grid exists
		for (auto it = tempElements.rbegin(); it != tempElements.rend(); ++it)
		{
			GridElement & ge = *it;


			if (ge.begin != ge.end)
			{
				// calculate bounding for points
				float clx, cly, chx, chy;

				clx = chx = ge.begin->x;
				cly = chy = ge.begin->y;

				for (const Point * pt = ge.begin; pt != ge.end; ++pt)
				{
					if (pt->x < clx)
						clx = pt->x;
					if (pt->x > chx)
						chx = pt->x;
					if (pt->y < cly)
						cly = pt->y;
					if (pt->y > chy)
						chy = pt->y;
				}

				ge.plx = clx;
				ge.ply = cly;
				ge.phx = chx;
				ge.phy = chy;
			}
			else
			{
				ge.plx = -std::numeric_limits<float>::max();
				ge.ply = -std::numeric_limits<float>::max();
				ge.phx = -std::numeric_limits<float>::max();
				ge.phy = -std::numeric_limits<float>::max();
			}

			if (ge.childIndexBegin != ge.childIndexEnd)
			{
				// calculate bounding for children
				float clx, cly, chx, chy;

				const unsigned begin = ge.childIndexBegin;
				const unsigned end = ge.childIndexEnd;
				bool initialized = false;

				for (unsigned childIdx = begin; childIdx < end; ++childIdx)
				{
					const GridElement & child = tempElements[childIdx];

					if (!initialized && child.begin != child.end)
					{
						clx = child.plx;
						cly = child.ply;
						chx = child.phx;
						chy = child.phy;
						initialized = true;
					}

					if (!initialized && child.childIndexBegin != child.childIndexEnd)
					{
						clx = child.lx;
						cly = child.ly;
						chx = child.hx;
						chy = child.hy;
						initialized = true;
					}

					if (child.begin != child.end)
					{
						if (child.plx < clx)
							clx = child.plx;
						if (child.phx > chx)
							chx = child.phx;
						if (child.ply < cly)
							cly = child.ply;
						if (child.phy > chy)
							chy = child.phy;
					}

					if (child.childIndexBegin != child.childIndexEnd)
					{
						if (child.lx < clx)
							clx = child.lx;
						if (child.hx > chx)
							chx = child.hx;
						if (child.ly < cly)
							cly = child.ly;
						if (child.hy > chy)
							chy = child.hy;
					}
				}

				ge.lx = clx;
				ge.ly = cly;
				ge.hx = chx;
				ge.hy = chy;
			}
			else
			{
				ge.lx = -std::numeric_limits<float>::max();
				ge.ly = -std::numeric_limits<float>::max();
				ge.hx = -std::numeric_limits<float>::max();
				ge.hy = -std::numeric_limits<float>::max();
			}

		}

		gridParamsMemBase = alignedAlloc(gridParams, tempElements.size() * 8 + 4, 16);

		// extract temp elements to native arrays
		for (size_t i = 0; i < tempElements.size(); ++i)
		{
			const GridElement & ge = tempElements[i];

			gridParams[i * 8 + 0] = MapFloatToInt(ge.hx);
			gridParams[i * 8 + 1] = MapFloatToInt(ge.hy);
			gridParams[i * 8 + 2] = MapFloatToInt(-ge.lx);
			gridParams[i * 8 + 3] = MapFloatToInt(-ge.ly);
			gridParams[i * 8 + 4] = MapFloatToInt(ge.phx);
			gridParams[i * 8 + 5] = MapFloatToInt(ge.phy);
			gridParams[i * 8 + 6] = MapFloatToInt(-ge.plx);
			gridParams[i * 8 + 7] = MapFloatToInt(-ge.ply);

			gridPoints.push_back(ge.begin - &pts.front());
			gridPoints.push_back(ge.end - &pts.front());
			gridChildren.push_back(ge.childIndexBegin);
			gridChildren.push_back(ge.childIndexEnd);
		}

		childrenToCheck = new unsigned[tempElements.size()];

		ptsParamsMemBase = alignedAlloc(ptsParams, pts.size() * 4 + 32, 32);
		ptsRanks = new int32_t[pts.size()];
		size_t i = 0;
		size_t prevStart = 0;
		for (const Point & pt : pts)
		{
			ptsParams[i * 4 + 0] = MapFloatToInt(pt.x);
			ptsParams[i * 4 + 1] = MapFloatToInt(pt.y);
			ptsParams[i * 4 + 2] = MapFloatToInt(-pt.x);
			ptsParams[i * 4 + 3] = MapFloatToInt(-pt.y);
			ptsRanks[i] = pt.rank;
			i++;
		}

		heapMemBase = new HeapItem[tempElements.size() + 8];
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
		delete[] childrenToCheck;
		delete[] gridParamsMemBase;

		//printf("Wind calls: %u, wind steps: %u, average: %f\n", windCalls, windSteps, ((float)(windSteps))/windCalls);
	}

	NOINLINE unsigned * SearchContext::ProcessTopGrid(unsigned * childrenToCheckEnd, const int * sw, const int * gridParams, const unsigned * gridChildren)
	{
		__m128i searchWindow = _mm_load_si128((__m128i*)sw);
		__m128i gridWindow = _mm_load_si128((__m128i*)gridParams);
		// searchWindow: shx, slx, shy, sly
		// gridWindow:   lx,  hx,  ly,  hy
		__m128i result = _mm_cmplt_epi32(gridWindow, searchWindow);
		const int intersect = _mm_testz_ps(_mm_castsi128_ps(result), _mm_castsi128_ps(result));
		if (intersect)
		{
			*childrenToCheckEnd++ = gridChildren[0];
			*childrenToCheckEnd++ = gridChildren[1];
		}
		return childrenToCheckEnd;
	}

	NOINLINE unsigned * SearchContext::ProcessIntermediateGrid(unsigned * childrenToCheckBegin, unsigned * childrenToCheckEnd, const int * sw, const int * gridParams, const unsigned * gridChildren)
	{
		__m128i searchWindow = _mm_load_si128((__m128i*)sw);
		unsigned * tmp = childrenToCheckEnd;
		for (; childrenToCheckBegin != tmp; childrenToCheckBegin += 2)
		{
			unsigned begin = childrenToCheckBegin[0];
			unsigned end = childrenToCheckBegin[1];

			for (unsigned i = begin; i < end; ++i)
			{
				__m128i gridWindow = _mm_load_si128((__m128i*)(gridParams + i * 8));
				// searchWindow: shx, slx, shy, sly
				// gridWindow:   lx,  hx,  ly,  hy
				__m128i result = _mm_cmplt_epi32(gridWindow, searchWindow);
				const int intersect = _mm_testz_ps(_mm_castsi128_ps(result), _mm_castsi128_ps(result));
				if (intersect)
				{
					*childrenToCheckEnd++ = gridChildren[i * 2];
					*childrenToCheckEnd++ = gridChildren[i * 2 + 1];
				}
			}
		}
		return childrenToCheckEnd;
	}

	NOINLINE HeapItem * SearchContext::ProcessLowestGrid(const unsigned * childrenToCheckBegin, const unsigned * childrenToCheckEnd, HeapItem * heap, const int * sw, const int * gridParams, const unsigned * gridPoints)
	{
		HeapItem * heapHead = heap;
		__m128i searchWindow = _mm_load_si128((__m128i*)sw);
		for (; childrenToCheckBegin != childrenToCheckEnd; childrenToCheckBegin += 2)
		{
			unsigned begin = childrenToCheckBegin[0];
			unsigned end = childrenToCheckBegin[1];
			for (unsigned i = begin; i < end; ++i)
			{
				__m128i gridWindow = _mm_load_si128((__m128i*)(gridParams + i * 8));
				// searchWindow: shx, slx, shy, sly
				// gridWindow:   lx,  hx,  ly,  hy
				__m128i result = _mm_cmplt_epi32(gridWindow, searchWindow);
				const int intersect = _mm_testz_ps(_mm_castsi128_ps(result), _mm_castsi128_ps(result));
				if (intersect)
				{
					heapHead->begin = gridPoints[i * 2];
					heapHead->end = gridPoints[i * 2 + 1];
					heapHead++;
					// heap[heapHead].rank is set by WindHeapItem
				}
			}
		}
		return heapHead;
	}

	NOINLINE void SearchContext::FixHeap(HeapItem * begin, HeapItem *& end, const int * ptsParams, const int * searchWindow, const int32_t * ptsRanks)
	{
		for (HeapItem * item = begin; item != end; ++item)
		{
			WindHeapItem(*item, ptsParams, searchWindow, ptsRanks);
		}
		for (HeapItem * item = begin; item != end; ++item)
		{
			if (item->rank == -1)
			{
				*item-- = *(--end);
			}
		}
	}


	NOINLINE void SearchContext::BuildHeap(HeapItem * begin, HeapItem * end)
	{
		for (unsigned i = ((end - begin) >> 2); i != -1; --i)
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
			HeapItem * firstChild = begin + idx * 4 + 1;
			HeapItem * lastChild = begin + idx * 4 + 4;

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
			unsigned parentIdx = ((idx - 1) >> 2);
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
			HeapItem * firstChild = begin + idx * 4 + 1;
			HeapItem * lastChild = begin + idx * 4 + 4;

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

	NOINLINE bool SearchContext::WindHeapItem(HeapItem & hi, const int * ptsParams, const int * sw, const int32_t * ptsRanks)
	{
		//windCalls++;
		unsigned begin = hi.begin;
		const unsigned end = hi.end;
		__m128i searchWindow = _mm_load_si128((__m128i*)sw);
		for (; begin < end; begin++)
		{
			//windSteps++;
			__m128i ptParam = _mm_load_si128((__m128i*)(ptsParams + begin * 4));
			_mm_prefetch((char*)(ptsParams + begin * 4) + 64, _MM_HINT_T0);
			// ptSearchWindow: slx, sly, -shx, -shy
			// ptParam:         x, y, -x, -y
			__m128i result = _mm_cmplt_epi32(ptParam, searchWindow); // inside search window if all false
			const int inside = _mm_testz_ps(_mm_castsi128_ps(result), _mm_castsi128_ps(result));
			if (inside)
			{
				hi.begin = begin;
				hi.rank = ptsRanks[begin];
				return true;
			}
		}
		hi.begin = begin;
		hi.rank = -1;
		return false;
	}


	NOINLINE int SearchContext::FlatWind(HeapItem & hi, const int * ptsParams, const int * sw, const int32_t * ptsRanks, const Point * pts, int count, Point * out_points)
	{
		//windCalls++;
		unsigned begin = hi.begin;
		const unsigned end = hi.end;
		__m128i searchWindow = _mm_load_si128((__m128i*)sw);
		int found = 0;
		for (; begin < end; begin++)
		{
			//windSteps++;
			__m128i ptParam = _mm_load_si128((__m128i*)(ptsParams + begin * 4));
			_mm_prefetch((char*)(ptsParams + begin * 4) + 64, _MM_HINT_T0);
			// ptSearchWindow: slx, sly, -shx, -shy
			// ptParam:         x, y, -x, -y
			__m128i result = _mm_cmplt_epi32(ptParam, searchWindow); // inside search window if all false
			const int inside = _mm_testz_ps(_mm_castsi128_ps(result), _mm_castsi128_ps(result));
			if (inside)
			{
				*out_points++ = pts[begin];
				if (++found == count)
				{
					return found;
				}
			}
		}
		return found;
	}

	NOINLINE const Point * SearchContext::PopHeapRoot(HeapItem * begin, HeapItem *& end, const int * ptsParams, const int * searchWindow, const int32_t * ptsRanks, const Point * pts, bool last)
	{

		const Point * ret = pts + begin->begin;
		if (!last)
		{
			begin->begin++;

			if (!WindHeapItem(*begin, ptsParams, searchWindow, ptsRanks))
			{
				FixEmptyRoot(begin, end);
			}
			else
			{
				RecursiveFixHeapItem(0, begin, end);
			}
		}
		
		return ret;
	}

	NOINLINE int SearchContext::ProcessHeap(HeapItem * heap, HeapItem * heapEnd, const int * ptsParams, const int * searchWindow, const int32_t * ptsRanks, const Point * pts, int count, Point * out_points)
	{
		if (heapEnd - heap == 1)
		{
			return FlatWind(*heap, ptsParams, searchWindow, ptsRanks, pts, count, out_points);
		}

		FixHeap(heap, heapEnd, ptsParams, searchWindow, ptsRanks);

		if (heapEnd - heap == 1)
		{
			return FlatWind(*heap, ptsParams, searchWindow, ptsRanks, pts, count, out_points);
		}

		BuildHeap(heap, heapEnd);

		int found = 0;
		while (heap != heapEnd)
		{
			const Point * pt = PopHeapRoot(heap, heapEnd, ptsParams, searchWindow, ptsRanks, pts, found == count-1);

			*out_points++ = *pt;
			if (++found == count)
			{
				break;
			}
		}
		return found;
	}

	NOINLINE int SearchContext::Search(float searchLx, float searchLy, float searchHx, float searchHy, const int32_t count, Point* out_points) const
	{
		const Point * const pts = this->pts.data();
		const int * const ptsParams = this->ptsParams;
		const int * const gridParams = this->gridParams;
		const unsigned * const gridChildren = this->gridChildren.data();
		const unsigned * const gridPoints = this->gridPoints.data();
		unsigned * childrenToCheck = this->childrenToCheck;

		int const maxLevel = this->levels - 1;

		__declspec(align(16)) int searchWindow[4];
		searchWindow[0] = MapFloatToInt(searchLx);
		searchWindow[1] = MapFloatToInt(searchLy);
		searchWindow[2] = MapFloatToInt(-searchHx);
		searchWindow[3] = MapFloatToInt(-searchHy);

		int found = 0;

		unsigned * childrenToCheckBegin = childrenToCheck;
		unsigned * childrenToCheckEnd = childrenToCheckBegin;
		*childrenToCheckEnd++ = 0;
		*childrenToCheckEnd++ = 1;
		HeapItem * heapEnd = ProcessLowestGrid(childrenToCheckBegin, childrenToCheckEnd, heap, searchWindow, gridParams+4, gridPoints);
		found += ProcessHeap(heap, heapEnd, ptsParams, searchWindow, ptsRanks, pts, count - found, out_points + found);
		if (found == count)
		{
			return found;
		}
		unsigned * childrenToCheckBeginTmp = childrenToCheckEnd;
		childrenToCheckEnd = ProcessTopGrid(childrenToCheckEnd, searchWindow, gridParams, gridChildren);
		childrenToCheckBegin = childrenToCheckBeginTmp;

		for (int level = 1; level < maxLevel; ++level)
		{
			heapEnd = ProcessLowestGrid(childrenToCheckBegin, childrenToCheckEnd, heap, searchWindow, gridParams + 4, gridPoints);
			found += ProcessHeap(heap, heapEnd, ptsParams, searchWindow, ptsRanks, pts, count - found, out_points + found);
			if (found == count)
			{
				return found;
			}
			childrenToCheckBeginTmp = childrenToCheckEnd;
			childrenToCheckEnd = ProcessIntermediateGrid(childrenToCheckBegin, childrenToCheckEnd, searchWindow, gridParams, gridChildren);
			childrenToCheckBegin = childrenToCheckBeginTmp;
		}

		heapEnd = ProcessLowestGrid(childrenToCheckBegin, childrenToCheckEnd, heap, searchWindow, gridParams + 4, gridPoints);
		found += ProcessHeap(heap, heapEnd, ptsParams, searchWindow, ptsRanks, pts, count - found, out_points + found);

		return found;
	}

}
