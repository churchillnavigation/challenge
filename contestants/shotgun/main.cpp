#include <iostream>
#include <windows.h>
#include <stdint.h>
#include <ctime>


/* Defines a point in 2D space with some additional attributes like id and rank. 
Uses a sensable 4 byte id value, so  the structs will align on 16 byte boundaries*/
struct Point16
{
	int32_t id;
	int32_t rank;
	float x;
	float y;
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


typedef SearchContext* (__stdcall* createFunc)(const Point* points_begin, const Point* points_end);
typedef int32_t (__stdcall* searchFunc)(SearchContext*, const Rect, const int32_t, Point*);
typedef SearchContext* (__stdcall* destroyFunc)(SearchContext* sc);
 

int main()
{
   createFunc _createFunc;
   searchFunc _searchFunc;
   destroyFunc _destroyFunc;
   HINSTANCE hInstLibrary = LoadLibrary(L"Shotgun.dll");
   //HINSTANCE hInstLibrary = LoadLibrary(L"reference.dll");
 
   if (hInstLibrary)
   {
	  _createFunc = (createFunc)GetProcAddress(hInstLibrary, "create");
	  _searchFunc = (searchFunc)GetProcAddress(hInstLibrary, "search");
	  _destroyFunc = (destroyFunc)GetProcAddress(hInstLibrary, "destroy");

      std::cout << "DLL Loaded" << std::endl;
	  
      if (_createFunc)
	  {
		 std::cout << "_createFunc Loaded" << std::endl;
	  }
	  
      if (_searchFunc)
	  {
		 std::cout << "_searchFunc Loaded" << std::endl;
	  }
	  
      if (_destroyFunc)
	  {
		 std::cout << "_destroyFunc Loaded" << std::endl;
	  }
	  
      if (_createFunc && _searchFunc && _destroyFunc)
      {
		  //Create an array of 10 million points, then pass those points to the T_create function and let it iterate
		  srand (static_cast <unsigned> (time(0)));

		  Point* samplePoints = (Point*)malloc(10000000 * sizeof(Point));

		  for (int i = 0; i < 10000000; i++)
		  {
			  Point temp;
			  temp.id = 1;
			  temp.rank = (static_cast <float> (rand()) / static_cast <float> (RAND_MAX)) * 9999999;  //Gives random rank numbers from 0 to 9999999
			  temp.x = ((static_cast <float> (rand()) / static_cast <float> (RAND_MAX)) * 2000) - 1000; //Gives random rank numbers from +- 1000
			  temp.y = ((static_cast <float> (rand()) / static_cast <float> (RAND_MAX)) * 2000) - 1000; //Gives random rank numbers from +- 1000

			  samplePoints[i] = temp;
		  }

		  //Call DLL's create function, which loads all the points
		  SearchContext* sc2 = _createFunc(samplePoints, samplePoints + 10000000);
		  free(samplePoints);

		  //Create a rectangle and a set of output points, then call the DLL's search function.
		  Rect rect;
		  rect.lx = ((static_cast <float> (rand()) / static_cast <float> (RAND_MAX)) * 1000) - 500;
		  rect.hx = ((static_cast <float> (rand()) / static_cast <float> (RAND_MAX)) * 1000) + 500;
		  rect.ly = ((static_cast <float> (rand()) / static_cast <float> (RAND_MAX)) * 1000) - 500;
		  rect.hy = ((static_cast <float> (rand()) / static_cast <float> (RAND_MAX)) * 1000) + 500;
		  Point* best = (Point*)malloc(20 * sizeof(Point));

		  _searchFunc(sc2, rect, 20, best);

		  for (int i = 0; i < 20; i++)
		  {			  
				std::cout << "outpoint " << i << " has rank = " << best[i].rank << ", x = " << best[i].x << ", and y = " << best[i].y << std::endl;
		  }

		  //Call the DLL's destroy function, to release its SearchContext object's memory
		  SearchContext* shouldBeNull = _destroyFunc(sc2);
		  free(best);

		  if (shouldBeNull == NULL) //Verify Release worked
		  {			  
				std::cout << "_destroyFunc Worked" << std::endl;
		  }

      }
	  else 
	  {
		 std::cout << "Not all functions loaded" << std::endl;
	  }
 
      FreeLibrary(hInstLibrary);
   }
   else
   {
      std::cout << "DLL Failed To Load!" << std::endl;
   }
 
   std::cin.get();
 
   return 0;
}
