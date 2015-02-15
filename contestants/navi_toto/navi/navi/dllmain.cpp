// dllmain.cpp : Defines the entry point for the DLL application.
#include "stdafx.h"
#include "sc3.h"

//#define LOG_OUT

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}

	return TRUE;
}

#define DLLSPEC extern "C" __declspec(dllexport)

DLLSPEC SearchContext* __stdcall create(const Point* points_begin, const Point* points_end)
{
    SearchContext* sc = new SearchContext();

    sc->create(points_begin, points_end);

#if defined(LOG_OUT)
	//clear log no robobust
	std::fstream fs( "out.txt", (points_end > points_begin) ? std::ios_base::out|std::ios_base::app : std::ios_base::out );
    fs << "\n **** create ****";
	//fs << "bound(" << sc->m_bound.lx << "," << sc->m_bound.ly << "," << sc->m_bound.hx << "," << sc->m_bound.hy << ")";
#endif

    return sc;
}

DLLSPEC int32_t __stdcall search(SearchContext* sc, const Rect rect, const int32_t count, Point* out_points)
{
#if defined(LOG_OUT)
	LARGE_INTEGER tf, t0, t1;
	QueryPerformanceFrequency(&tf);

	QueryPerformanceCounter(&t0);
    int32_t results = sc->search(rect, count, out_points);
	QueryPerformanceCounter(&t1);

	std::fstream fs( "out.txt", std::ios_base::out|std::ios_base::app );
	fs << "\n";
	fs << "results( " << results << " / " << (t1.QuadPart-t0.QuadPart) << "tiks ) ";
	fs << "rect(" << rect.lx << "," << rect.ly << "," << rect.hx << "," << rect.hy << "/ " << rect.hx-rect.lx << "," << rect.hy-rect.ly << ") ";
	for( int i = 0; i < results; i++ )
		fs << "(" << out_points[i].x << "," << out_points[i].y << ")";

	return results;
#else
	return sc->search(rect, count, out_points);
#endif
}

DLLSPEC SearchContext* __stdcall destroy(SearchContext* sc)
{
    delete sc;
    return 0;
}
