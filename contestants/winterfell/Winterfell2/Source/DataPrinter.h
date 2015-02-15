#pragma once

struct Point;
struct Rect;

namespace DataPrinter
{
	void DeleteFiles();
	void PrintPoints(const Point* inStart, const Point* inEnd);
	void PrintPoints(const Point* inStart, const Point* inEnd, const char* inFilename);
	void AppendRect(const Rect& inRect);
	void AppendRect(const Rect& inRect, const char* inFilename);
	void AppendPointAllocatorCount(int inCount);
}