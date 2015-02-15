#pragma once

#define _FORCE_ALIGNMENT

#ifdef _FORCE_ALIGNMENT
#define _ALIGN_TO( n ) __declspec(align(n))
#else
#define _ALIGN_TO( n )
#endif
