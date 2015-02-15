#include "DataPrinter.h"

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <stdio.h>
#include "point_search.h"


namespace DataPrinter
{
	static const char* kPointsFileName = "input.txt";
	static const char* kRectsFileName = "rects.txt";
	static const char* kStackAllocatorFileName = "pointStackAlloc.txt";

	void DeleteFiles()
	{
		remove(kPointsFileName);
		remove(kRectsFileName);
		remove(kStackAllocatorFileName);
	}

	void PrintPoints(const Point* inStart, const Point* inEnd)
	{
		PrintPoints(inStart, inEnd, kPointsFileName);
	}

	void PrintPoints(const Point* inStart, const Point* inEnd, const char* inFilename)
	{
		FILE* file = fopen(inFilename, "w");
		if (nullptr == file)
		{
			return;
		}

		const int kBufSize = 256;
		char buffer[kBufSize];

		for (auto iter = inStart; iter != inEnd; ++iter)
		{
			const Point& p = *iter;

			int count = _snprintf(buffer, kBufSize, "%.6f, %.6f, %i, %i\n", p.x, p.y, p.id, p.rank);
			int toWrite = (count < kBufSize) ? count : kBufSize;
			fwrite(buffer, 1, toWrite, file);
		}

		fclose(file);
	}

	void AppendRect(const Rect& inRect)
	{
		AppendRect(inRect, kRectsFileName);
	}

	void AppendRect(const Rect& inRect, const char* inFilename)
	{
		FILE* file = fopen(inFilename, "a");
		if (nullptr != file)
		{
			fseek(file, 0, SEEK_END);

			const int kBufSize = 256;
			char buffer[kBufSize];

			float xRange = inRect.hx - inRect.lx;
			float yRange = inRect.hy - inRect.ly;
			int count = _snprintf(buffer, kBufSize, "%.6f, %.6f, %.6f, %.6f (Size: %.6f, %.6f)\n", inRect.lx, inRect.ly, inRect.hx, inRect.hy, xRange, yRange);
			int toWrite = (count < kBufSize) ? count : kBufSize;
			fwrite(buffer, 1, toWrite, file);

			fclose(file);
		}
	}


	void AppendPointAllocatorCount(int inCount)
	{
		FILE* file = fopen(kStackAllocatorFileName, "a");
		if (nullptr != file)
		{
			fseek(file, 0, SEEK_END);

			const int kBufSize = 256;
			char buffer[kBufSize];

			int count = _snprintf(buffer, kBufSize, "%i\n", inCount);
			int toWrite = (count < kBufSize) ? count : kBufSize;
			fwrite(buffer, 1, toWrite, file);

			fclose(file);
		}
	}

}