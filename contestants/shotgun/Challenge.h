#ifndef _CHALLENGE_H_
#define _CHALLENGE_H_
#include <iostream>
#include <stdint.h>
 
#ifdef  DLL_EXPORT
#define DECLDIR __declspec(dllimport)
#else
#define DECLDIR __declspec(dllexport)
#endif

 extern "C"
{
	
	/* Defines a point in 2D space with some additional attributes like id and rank. 
	Uses a sensable 4 byte id value, so  the structs will align on 16 byte boundaries*/
	struct Point16
	{
		float x;
		float y;
		int32_t id;
		int32_t rank;
	};

	struct SearchContext;
	
	/* The following structs are packed with no padding. */
	#pragma pack(push, 1)

	/* Defines a point in 2D space with some additional attributes like id and rank. */
	struct Point
	{
		int8_t id;
		int32_t rank;
		float x;
		float y;
	};

	/* Defines a rectangle, where a point (x,y) is inside, if x is in [lx, hx] and y is in [ly, hy]. */
	struct Rect
	{
		float lx;
		float ly;
		float hx;
		float hy;
	};
	#pragma pack(pop)
	   
	__declspec(dllexport) SearchContext* create(const Point* points_begin, const Point* points_end);	
	__declspec(dllexport) int32_t search(SearchContext* sc, const Rect rect, const int32_t count, Point* out_points);	
	__declspec(dllexport) SearchContext* destroy(SearchContext* sc);

	
	void quickSort(Point16 a[], int first, int last);
	int pivot(Point16 a[], int first, int last);
	void swap(Point16& a, Point16& b);
}

 
#endif
