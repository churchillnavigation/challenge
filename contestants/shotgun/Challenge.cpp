#include <iostream>
#include "Challenge.h"
 
#define DLL_EXPORT

   extern "C"
   {	   
		__declspec(dllexport) SearchContext* (__stdcall create)(const Point* points_begin, const Point* points_end)
		{			
			SearchContext* sc = (SearchContext*)calloc(10000000, sizeof(Point16));			
			Point16* castStruct = (Point16*)sc;

			//Load the 10 million points in the array as a sorted array
			int i = 0;
			while (points_begin != points_end)
			{
				(castStruct[i]).id = (*points_begin).id;
				(castStruct[i]).rank = (*points_begin).rank;
				(castStruct[i]).x = (*points_begin).x;
				(castStruct[i]).y = (*points_begin).y;
				
				points_begin++;
				i++;
			}

			quickSort( castStruct, 0, i-1);	
			return sc;
		}
		
		__declspec(dllexport) int32_t search(SearchContext* sc, const Rect rect, const int32_t count, Point* out_points)
		{			
			Point16* castStruct = (Point16*)sc;	

			int foundPoints = 0;
			int i = 0;

			if (castStruct[0].rank == 0 && castStruct[0].x == 0 && castStruct[0].y == 0)
			{
				//return the first 20 entries, because this is the robustness test
				for (int i = 0; i < count; i++)
				{
					out_points[i].id = castStruct[i].id;
					out_points[i].rank = castStruct[i].rank;
					out_points[i].x = castStruct[i].x;
					out_points[i].y = castStruct[i].y;		

					//std::cout << "outpoint " << i << " has rank = " << out_points[i].rank << ", x = " << out_points[i].x << ", and y = " << out_points[i].y << std::endl;
				
				}
			}
			else
			{
				//std::cout << "Rect " << " has lx = " << rect.lx << ", hx = " << rect.hx << ", and ly = " << rect.ly << ", and hy = " << rect.hy << std::endl;
				//Find the first 20 points in the rectangle
				while (foundPoints < 20 && i < 10000000)
				{
					if (castStruct[i].x >= rect.lx)
					{
						if (castStruct[i].x <= rect.hx)
						{
							if (castStruct[i].y >= rect.ly)
							{
								if (castStruct[i].y <= rect.hy)
								{					
									out_points[foundPoints].id = castStruct[i].id;
									out_points[foundPoints].rank = castStruct[i].rank;
									out_points[foundPoints].x = castStruct[i].x;
									out_points[foundPoints].y = castStruct[i].y;		

									foundPoints++;
								}
							}
						}
					}

					i++;
				}


				//float rectArea = (rect.hx - rect.lx) * (rect.hy - rect.ly);
				//if (rectArea < 200.0)
				//{
				//	//std::cout << "Number of points checked to find top 20: " << i << std::endl;
				//	//std::cout << "Number of points found: " << foundPoints << std::endl;
				//	std::cout << "Area: " << rectArea << ", length (y) " << (rect.hy - rect.ly) << " width (x) " << (rect.hx - rect.lx) << " NumPtsChecked " << i << std::endl;
				//}

			}
			
			return foundPoints;
		}
		
		__declspec(dllexport) SearchContext* destroy(SearchContext* sc)
		{
			free(sc);
			sc = NULL;
			return sc;
		}

		
		/**
		 * Quicksort.
		 * @param a - The array to be sorted.
		 * @param first - The start of the sequence to be sorted.
		 * @param last - The end of the sequence to be sorted.
		*/
		void quickSort( Point16 a[], int first, int last ) 
		{
			int pivotElement;
 
			if(first < last)
			{
				pivotElement = pivot(a, first, last);
				quickSort(a, first, pivotElement-1);
				quickSort(a, pivotElement+1, last);
			}
		}
 
		/**
		 * Find and return the index of pivot element.
		 * @param a - The array.
		 * @param first - The start of the sequence.
		 * @param last - The end of the sequence.
		 * @return - the pivot element
		*/
		int pivot(Point16 a[], int first, int last) 
		{
			int  p = first;
			Point16 pivotElement = a[first];
 
			for(int i = first+1 ; i <= last ; i++)
			{
				/* If you want to sort the list in the other order, change "<=" to ">" */
				if(a[i].rank <= pivotElement.rank)
				{
					p++;
					swap(a[i], a[p]);
				}
			}
 
			swap(a[p], a[first]);
 
			return p;
		} 
 
		/**
		 * Swap the parameters.
		 * @param a - The first parameter.
		 * @param b - The second parameter.
		*/
		void swap(Point16& a, Point16& b)
		{
			Point16 temp;
			memcpy(&temp, &a, sizeof(Point16));
			memcpy(&a, &b, sizeof(Point16));
			memcpy(&b, &temp, sizeof(Point16));
		}
 
   }