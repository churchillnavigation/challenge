//_____________________________________________________________________________________________________________________________
#include "point_search.h"
#include <cstdio>
#include <ctime>
#include <algorithm>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <psapi.h>
//_____________________________________________________________________________________________________________________________
// Helper classes and functions
//.............................................................................................................................
static void LOG(const char* fmt, ...)
{
	static FILE* out = nullptr;
	if( out == nullptr )
	{
		::fopen_s(&out, "log.txt", "wt");
	}
	va_list args;
	va_start(args, fmt);
	::vfprintf(out, fmt, args);
	::vfprintf(stdout, fmt, args);
	::fflush(out);
	va_end(args);
}
//.............................................................................................................................
static uint64_t get_process_commit_size()
{
	::PROCESS_MEMORY_COUNTERS_EX mem;
	::GetProcessMemoryInfo(::GetCurrentProcess(), reinterpret_cast<::PROCESS_MEMORY_COUNTERS*>(&mem), sizeof(::PROCESS_MEMORY_COUNTERS_EX));
	return mem.PrivateUsage;
}
//.............................................................................................................................
class Clock
{
public:
	::LARGE_INTEGER freq;
	::LARGE_INTEGER begin;
	Clock()
	{
		::QueryPerformanceFrequency(&this->freq);
	}
	void start()
	{
		::QueryPerformanceCounter(&this->begin);
	}
	double stop()
	{
		::LARGE_INTEGER end;
		::QueryPerformanceCounter(&end);
		const auto delta_tick = end.QuadPart - this->begin.QuadPart;
		const auto delta_usec = (delta_tick * 1000000) / this->freq.QuadPart;
		const double delta_ms = delta_usec / 1000.0;
		return delta_ms;
	}
};
//.............................................................................................................................
static Clock clk;
typedef unsigned int uint;
static const float pi = 3.141592653589793238462643383279502884197169399375105820974944f;
static float sqr(const float f) { return f * f; }
static inline uint l(const uint t, const uint k) { return t ^ (t << k); }
static inline uint r(const uint t, const uint k) { return t ^ (t >> k); }
struct S_seed { uint x, y, z, w, v; };
static S_seed seed = { 123456789, 362436069, 521288629, 88675123, 886756453, }; 
//.............................................................................................................................
static void randomize_seed()
{
	FILETIME t[1];
	::GetSystemTimeAsFileTime(t);
	seed.x = seed.z ^ t->dwLowDateTime;
	seed.y = seed.y ^ t->dwLowDateTime;
	seed.z = seed.z ^ t->dwLowDateTime;
	seed.w = seed.w ^ t->dwLowDateTime;
	seed.v = seed.v ^ t->dwLowDateTime;
}
//.............................................................................................................................
static uint random() 
{
	const uint n=l(seed.v,6)^l(r(seed.x,7),13);
	seed.x=seed.y; seed.y=seed.z; seed.z=seed.w; seed.w=seed.v; seed.v=n;
	return (seed.y+seed.y+1)*seed.v;
}
//.............................................................................................................................
static float random(const float l, const float h)
{
	const float t = static_cast<float>(random()) / 0x100000000; // [0,+1)
	return l + t * (h - l);
}
//.............................................................................................................................
static Rect random(const float lx, const float hx, const float ly, const float hy)
{
	const float cx = ::random(lx, hx);
	const float cy = ::random(ly, hy);
	const float sx = (hx - lx) / 2.0f * ::sqr(::random(0.0, 1.0));
	const float sy = (hy - ly) / 2.0f * ::sqr(::random(0.0, 1.0));
	Rect r = { cx - sx, cy - sy, cx + sx, cy + sy };
	return r;
}
//.............................................................................................................................
struct S_algorithm
{
	enum { PATH_SIZE = 256 };
	char full_path[PATH_SIZE];
	int64_t local_path;
	HMODULE module;
	SearchContext* context;
	T_create create;
	T_search search;
	T_destroy destroy;
	double time_create;
	double time_search;
	uint64_t commit_size;

	bool operator< (const S_algorithm& a) const
	{
		return this->time_search < a.time_search;
	}

	bool load(const char* s)
	{
		this->time_search = -1;
		if (s == 0) { return false; }
		LOG("Loading %s... ", s);
		char* file_path = nullptr;
		const DWORD path_len = ::SearchPathA(".\\", s, 0, S_algorithm::PATH_SIZE, this->full_path, &file_path);
		if (path_len == 0) { LOG("Not found.\n"); return false; }
		if (path_len > S_algorithm::PATH_SIZE) { LOG("Path too long.\n"); return false; }
		this->local_path = file_path - this->full_path;
		this->module = ::LoadLibraryExA(this->full_path, 0, 0);
		if (this->module == 0) { LOG("Not a valid module.\n"); return false; }
		this->create = reinterpret_cast<T_create>(::GetProcAddress(this->module, "create"));
		this->search = reinterpret_cast<T_search>(::GetProcAddress(this->module, "search"));
		this->destroy = reinterpret_cast<T_destroy>(::GetProcAddress(this->module, "destroy"));
		if (this->create == 0 || this->search == 0 || this->destroy == 0) { LOG("Missing requested functions.\n"); return false; }
		LOG("Success.\n");
		return true;
	}

	double do_create(const Point* xpoints_begin, const Point* xpoints_end)
	{
		__try
		{
			clk.start();
			this->context = this->create(xpoints_begin, xpoints_end);
			return clk.stop();
		}
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
			return -1;
		}
	}

	double do_create(const Point* points, const int point_count)
	{
		Point* xpoints = new Point[point_count];
		memcpy(xpoints, points, sizeof(Point) * point_count); // make copy
		const auto time = this->do_create(xpoints, xpoints + point_count);
		memset(xpoints, 0, sizeof(Point) * point_count); // erase copy
		delete [] xpoints;
		return time;
	}
};
//_____________________________________________________________________________________________________________________________
static int _main(int argc, char** argv)
{
	// Default settings
	const char title[] = "--- Churchill Navigation Point Search Challenge ---\n";
	int point_count = 10000000;
	int query_count = 1000;
	int result_count = 20;

	// If no parameters are given, then print help
	if( argc == 1 )
	{
		// extract executable file path
		enum { PATH_SIZE = 256, };
		char executable_full_path[PATH_SIZE];
		char* executable_file_path = nullptr;
		::GetFullPathNameA(argv[0], PATH_SIZE, executable_full_path, &executable_file_path);

		LOG
		(
			"%s"
			"Description: Given [point count] ranked points on a plane, find the [result count] most important points inside [query count] rectangles. "
			"You can specify a list of modules that solve this problem, and their results and performance will be compared!\n"
			"Usage:\n"
			"\t%s module_paths [-pN] [-qN] [-rN] [-s]\n"
			"Options:\n"
			"\t-pN: point count (default: %d)\n"
			"\t-qN: query count (default: %d)\n"
			"\t-rN: result count (default: %d)\n"
			"\t-sX: specify seed (default: random)\n"
			"Example:\n"
			"\t%s reference.dll acme.dll -p10000000 -q100000 -r20 -s642E3E98-6EA650C8-642E3E98-7E75161E-4FE6D148\n",
			title,
			executable_file_path,
			point_count, query_count, result_count,
			executable_file_path
		);

		return 0;
	}

	::randomize_seed();

	// Optionally update default settings, based on command line arguments
	for(int i_arg = 1; i_arg < argc; ++i_arg)
	{
		const char* s = argv[i_arg];
		if( s[0] == '-' )
		{
			switch( s[1] )
			{
				case 's': { ::sscanf_s(s + 2, "%X-%X-%X-%X-%X", &seed.x, &seed.y, &seed.z, &seed.w, &seed.v); break; }
				case 'p': { point_count = ::atoi(s + 2); break; }
				case 'q': { query_count = ::atoi(s + 2); break; }
				case 'r': { result_count = ::atoi(s + 2); break; }
			}
		}
	}

	// Print settings
	LOG
	(
		"%s"
		"Point count  : %d\n"
		"Query count  : %d\n"
		"Result count : %d\n"
		"Random seed  : %X-%X-%X-%X-%X\n"
		"\n",
		title,
		point_count,
		query_count,
		result_count,
		seed.x, seed.y, seed.z, seed.w, seed.v
	);

	// Load algorithms
	LOG("Loading algorithms:\n");
	S_algorithm* algorithms = new S_algorithm[argc];
	int n_algorithm = 0;
	for(int i_arg = 1; i_arg < argc; ++i_arg)
	{
		const char* s = argv[i_arg];
		if( s[0] != '-' )
		{
			if( algorithms[n_algorithm].load(s) )
			{
				++n_algorithm;
			}
		}
	}

	// Exit, if no algorithms loaded
	if( n_algorithm == 0 )
	{
		LOG("No algorithms loaded.\n");
		return 0;
	}

	LOG("%d algorithms loaded.\n", n_algorithm);

	// Prepare points
	LOG("\n");
	LOG("Preparing %d random points...", point_count);
	clk.start();
	Point* points = new Point[point_count];
	Point* points_begin = points;
	Point* points_end = points + point_count;

	const float r = 1e3;
	Rect main_rect = ::random(-r, +r, -r, +r);

	int rank = 0;
	Point* i = points_begin;
	for(;i != points_end;)
	{
		// compute origin
		const uint cluster_n = ::random() & 0xffff;
		const float cluster_ox = ::random(main_rect.lx, main_rect.hx);
		const float cluster_oy = ::random(main_rect.ly, main_rect.hy);
		float rr[] = { cluster_ox - main_rect.lx, main_rect.hx - cluster_ox, cluster_oy - main_rect.ly, main_rect.hy - cluster_oy, };
		std::sort(rr, rr + 4);
		const float r = rr[0];
		Point* cluster_end = i + cluster_n;
		cluster_end = cluster_end > points_end ? points_end : cluster_end;
		for(; i != cluster_end; ++i)
		{
			const float angle = ::random(0.0, pi);
			const float range = ::random(0.0, r);

			i->id = static_cast<int8_t>(::random());
			i->rank = rank++;
			i->x = cluster_ox + ::cos(angle) * range;
			i->y = cluster_oy + ::sin(angle) * range;
		}
	}

	// modify the last four points to stretch the coordinate range, so that it's even harder to normalize or use a grid efficiently
	if( point_count >= 4 )
	{
		const float r = 1e10;
		points_end[-1].x = -r;
		points_end[-2].x = +r;
		points_end[-3].y = -r;
		points_end[-4].y = +r;
	}

	// creating a random permutation of ranks:
	for(int i = 0; i < point_count; ++i)
	{
		const int j = i + random() % (point_count - i);
		std::swap(points[i].rank, points[j].rank);
	}

	LOG("done (%.4fms).\n", clk.stop());

	// Prepare queries
	LOG("Preparing %d random queries...", query_count);
	clk.start();
	Rect* rects = new Rect[query_count];
	Rect* rects_begin = rects;
	Rect* rects_end = rects + query_count;
	for(Rect* i = rects_begin; i != rects_end; ++i)
	{
		*i = ::random(main_rect.lx, main_rect.hx, main_rect.ly, main_rect.hy);
	}
	LOG("done (%.4fms).\n", clk.stop());
	LOG("\n");

	// Prepare result buffers
	Point* out_points0 = new Point[result_count];
	Point* out_points1 = new Point[result_count];

	// For each algorithm, load the points and make queries
	for(int i_algorithm = 0; i_algorithm < n_algorithm; ++i_algorithm)
	{
		for(;;)
		{
			S_algorithm* algorithm = &algorithms[i_algorithm];
			LOG("Testing algorithm #%d (%s):\n", i_algorithm, algorithm->full_path + algorithm->local_path);

			const auto commit0 = get_process_commit_size();

			// Robustness testing:
			LOG("Robustness check...");
			__try { algorithm->context = algorithm->create(nullptr, nullptr); }
			__except( EXCEPTION_EXECUTE_HANDLER ) { LOG("CRASHED IN CREATE!\n\n"); break; }
			__try { algorithm->search(algorithm->context, rects[0], result_count, out_points0); }
			__except( EXCEPTION_EXECUTE_HANDLER ) { LOG("CRASHED IN SEARCH!\n\n"); break; }
			__try { algorithm->context = algorithm->destroy(algorithm->context); }
			__except( EXCEPTION_EXECUTE_HANDLER ) { LOG("CRASHED IN DESTROY!\n\n"); break; }
			LOG("done.\n");

			// Load points
			LOG("Loading points...");
			algorithm->time_create = algorithm->do_create(points, point_count);
			if (algorithm->time_create < 0)
			{
				LOG("CRASHED!\n\n");
				break;
			}
			else
			{
				LOG("done (%.4fms).\n", algorithm->time_create);
			}

			// Search points
			LOG("Making queries...", query_count);
			__try
			{
				clk.start();
				for (int i = 0; i < query_count; ++i)
				{
					algorithm->search(algorithm->context, rects[i], result_count, out_points0);
				}
				algorithm->time_search = clk.stop();
				LOG("done (%.4fms, avg %.4fms/query).\n", algorithm->time_search, algorithm->time_search / query_count);
			}
			__except( EXCEPTION_EXECUTE_HANDLER )
			{
				LOG("CRASHED!\n\n");
				break;
			}

			const auto commit1 = get_process_commit_size();
			algorithm->commit_size = (commit1 - commit0) >> 20;

			LOG("Release points...");
			__try
			{
				algorithm->context = algorithm->destroy(algorithm->context);
				if (algorithm->context)
				{
					LOG("failed");
				}
				else
				{
					LOG("done");
				}
				LOG(" (memory used: %luMB).", algorithm->commit_size);
			}
			__except (EXCEPTION_EXECUTE_HANDLER)
			{
				LOG("CRASHED!\n\n");
				break;
			}

			LOG("\n\n");
			break;
		}
	}

	// Compare the results of each algorithm
	LOG("Comparing the results of algorithms: ");
	if (n_algorithm > 1)
	{
		LOG("\n");
		for (int i_algorithm0 = 0; i_algorithm0 < n_algorithm; ++i_algorithm0)
		{
			S_algorithm* algorithm0 = &algorithms[i_algorithm0];
			for (int i_algorithm1 = i_algorithm0 + 1; i_algorithm1 < n_algorithm; ++i_algorithm1)
			{
				S_algorithm* algorithm1 = &algorithms[i_algorithm1];
				LOG("#%d (%s) and #%d (%s): ",
					i_algorithm0, algorithm0->full_path + algorithm0->local_path,
					i_algorithm1, algorithm1->full_path + algorithm1->local_path
				);
				__try
				{
					algorithm0->do_create(points, point_count);
					algorithm1->do_create(points, point_count);
					bool differ = false;
					for (int i_round = 0; i_round < query_count; ++i_round)
					{
						const int result_count0 = algorithm0->search(algorithm0->context, rects[i_round], result_count, out_points0);
						const int result_count1 = algorithm1->search(algorithm1->context, rects[i_round], result_count, out_points1);

						if (result_count0 != result_count1)
						{
							differ = true;
							goto label_differ;
						}

						for (int i_result = 0; i_result < result_count0; ++i_result)
						{
							const auto p0 = out_points0[i_result];
							const auto p1 = out_points1[i_result];
							if (p0.rank != p1.rank || p0.id != p1.id)
							{
								differ = true;
								goto label_differ;
							}
						}
					}
				label_differ:
					if (differ)
					{
						LOG("differ.\n");
					}
					else
					{
						LOG("match.\n");
					}
					algorithm0->context = algorithm0->destroy(algorithm0->context);
					algorithm1->context = algorithm1->destroy(algorithm1->context);
				}
				__except (EXCEPTION_EXECUTE_HANDLER)
				{
					LOG("CRASHED!\n");
				}
			}
		}
		LOG("\n");

		LOG("Scoreboard:\n");
		std::sort(algorithms, algorithms + n_algorithm);
		for (int i_algorithm = 0; i_algorithm < n_algorithm; ++i_algorithm)
		{
			S_algorithm* a = algorithms + i_algorithm;
			LOG("#%d: %.4lfms %s\n", i_algorithm, a->time_search, a->full_path + a->local_path);
		}
		LOG("\n");
	}
	else
	{
		LOG("There's nothing to compare to, load more DLLs!\n");
	}

	// Release resources (memory, dlls)
	LOG("Cleaning up resources...");
	delete [] points;
	delete [] out_points0;
	delete [] out_points1;
	delete [] rects;
	for(int i_algorithm = 0; i_algorithm < n_algorithm; ++i_algorithm)
	{
		S_algorithm* algorithm = &algorithms[i_algorithm];
		::FreeLibrary(algorithm->module);
	}

	// Exit
	LOG("done.\n");
	return 0;
}
//_____________________________________________________________________________________________________________________________
static LONG WINAPI exception_filter(const LPEXCEPTION_POINTERS /*ep*/)
{
	::TerminateProcess(::GetCurrentProcess(), ~0U); // This call is asynchronous and returns immediately
	::Sleep(INFINITE); // Wait to be killed
	return EXCEPTION_CONTINUE_SEARCH;
}
//_____________________________________________________________________________________________________________________________
int main(int argc, char** argv)
{
	__try
	{
		return _main(argc, argv);
	}
	__except( exception_filter(GetExceptionInformation()) )
	{
		return -1;
	}
}
//_____________________________________________________________________________________________________________________________
