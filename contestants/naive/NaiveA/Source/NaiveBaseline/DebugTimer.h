#include <windows.h>

double PCFreq = 0.0;
__int64 CounterStart = 0;

namespace DebugTimer
{
    void StartCounter()
    {
        LARGE_INTEGER li;

        QueryPerformanceFrequency(&li);

        PCFreq = double(li.QuadPart) / 1000.0;

        QueryPerformanceCounter(&li);
        CounterStart = li.QuadPart;
    }
    double GetCounter()
    {
        LARGE_INTEGER li;
        QueryPerformanceCounter(&li);
        return double(li.QuadPart - CounterStart) / PCFreq;
    }
}