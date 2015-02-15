#include <cstdio>

#include <algorithm>
#include <atomic>
#include <vector>
#include <queue>
#include <limits>

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "tree.h"
//#include "k_merge_tree.h"

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
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

struct SearchContext {
	PointTree tree;

	// Search
	std::vector<Point> r;
	//sway::bounded_priority_queue<Point> a;
	//kmerge_tree_c<Point, 
};

const int WORKER_IDLE = 0;
const int WORKER_EXIT = 1;
const int WORKER_QUERY = 2;

static SearchContext* g_WorkerCtx;

static std::atomic<int> g_WorkerState;

static std::atomic<Rect*> g_QueryRect;
static std::atomic<bool> g_ResultsReady;

DWORD WINAPI WorkerThread(LPVOID lpdwThreadParam) {
	auto ctx = (SearchContext *)lpdwThreadParam;

	while(true) {
		auto state = g_WorkerState.load(std::memory_order_acquire);

		if (state == WORKER_EXIT)
			break;

		if (state == WORKER_QUERY) {
			auto rect = g_QueryRect.load(std::memory_order_acquire);

			if (rect != nullptr) {
				ctx->tree.queryPoints(*rect, 20, ctx->r);

				g_ResultsReady.store(true, std::memory_order_release);
			}

			g_WorkerState.store(WORKER_IDLE, std::memory_order_release);
		}
	}

	//printf("Done!!\n");

	delete ctx;
	return 0;
}

extern "C"
{
	__declspec(dllexport) SearchContext* __stdcall create(const Point* points_begin, const Point* points_end)
	{
		SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
		SetThreadAffinityMask(GetCurrentThread(), 1);
		size_t count = points_end - points_begin;

		std::vector<Point> pts, worker_pts;

		pts.insert(pts.end(), points_begin, points_end);
		//std::sort(pts.begin(), pts.end(), [](Point& i, Point& j) -> bool { return i.x < j.x; });

		//worker_pts.insert(worker_pts.end(), pts.begin() + count / 2, pts.end());
		//pts.erase(pts.begin() + count / 2, pts.end());
		//pts.clear();

		auto ctx = new SearchContext();

		ctx->r.reserve(256);
		ctx->tree.build(pts);

		//volatile void* wow = new int[10000000];

		//auto ctx2 = new SearchContext();

		//ctx2->r.reserve(256);
		//ctx2->tree.build(worker_pts);

		//g_WorkerCtx = ctx2;

		//g_WorkerState.store(WORKER_IDLE, std::memory_order_release);

		//DWORD workerId;
		//CreateThread(NULL, 0, &WorkerThread, g_WorkerCtx, 0, &workerId);

		//HANDLE workerHandle = OpenThread(THREAD_ALL_ACCESS, false, workerId);
		//SetThreadAffinityMask(workerHandle, 4);

		//g_WorkerState.store(WORKER_QUERY, std::memory_order_release);

		return ctx;
	}

	__declspec(dllexport) int32_t __stdcall search(SearchContext* ctx, const Rect rect, const int32_t count, Point* out_points)
	{
		//printf("Searching %d pts - (%.4f, %.4f) -> (%.4f, %.4f)\n", count, rect.lx, rect.ly, rect.hx, rect.hy);
		
		//g_ResultsReady.store(false, std::memory_order_seq_cst);
		//g_QueryRect.store((Rect *) &rect, std::memory_order_seq_cst);
		//g_WorkerState.store(WORKER_QUERY, std::memory_order_seq_cst);

		ctx->tree.queryPoints(rect, count, ctx->r);

		// Busy wait for results
		//while (g_ResultsReady.load(std::memory_order_acquire) == false);

		//mergePoints(ctx->r, count, INT_MAX, &g_WorkerCtx->r[0], g_WorkerCtx->r.size(), &rect);

		if (ctx->r.size() > 0)
			memcpy(out_points, &ctx->r[0], ctx->r.size() * sizeof(Point));

		return ctx->r.size();
	}

	__declspec(dllexport) SearchContext* __stdcall destroy(SearchContext* ctx)
	{
		//g_WorkerState.store(WORKER_EXIT, std::memory_order_release);

		delete ctx;
		return nullptr;
	}
}