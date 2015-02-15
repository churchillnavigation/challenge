#pragma once

//#define _TRACE_FLOw
#ifdef _TRACE_FLOw
#include <cstdio>
#define LOG(format, ...) printf( format "\n", __VA_ARGS__ )
#else
#define LOG(format, ...)
#endif

