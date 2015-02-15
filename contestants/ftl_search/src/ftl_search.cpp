#include <stdlib.h>
#include <stdio.h>

#include <vector>

#include <ftl_search/point_search.h>

#include <windows.h> 

int main(int argc, char *argv[])
{
    if( argc != 1 )
    {
        printf( "Usage: point_search point_file rect_file\n" );
        exit( 1 );
    }

	HINSTANCE dllHandle = LoadLibrary("libftl_search.dll");

	// If the handle is valid, try to get the function address. 
	if (NULL != dllHandle)
	{
		//Get pointer to our function using GetProcAddress:
		T_search s = (T_search) GetProcAddress(dllHandle, "search");
		T_create c = (T_create) GetProcAddress(dllHandle, "create");
		T_destroy d = (T_destroy) GetProcAddress(dllHandle, "destroy");
		if (s != NULL && c != NULL && d != NULL)
		{
			printf("Success!\n");
		}
		else
		{
			printf("Fail!\n");
		}
	}
	else
	{
		printf("Fail!");
	}

	exit( 1 );

    std::vector<Point> points;
    
    FILE *fp = fopen( argv[ 1 ], "r" );
    Point p;
    int id = 0;
    while( fscanf( fp, "%f %f %d", &p.x, &p.y, &p.rank ) == 3 )
    {
        p.id = id++ % 127 ;
        points.push_back( p );
    }
    fclose( fp );
    SearchContext *sc = create( &points[ 0 ], &points[ points.size( ) ] );

    Point *out = (Point *) malloc( sizeof( Point ) * 20 );
    fp = fopen( argv[ 2 ], "r" );
    Rect r;
    while( fscanf( fp, "%f %f %f %f", &r.lx, &r.hx, &r.ly, &r.hy ) == 4 )
    {
        int a = search( sc, r, 20, out );
        for(int i = 0; i < a; i++)
        {
            printf( "%d ", out[ i ].rank );
        }
        printf( "\n" );
    }
}
