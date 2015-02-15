// ChallengeDLL.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "ChallengeDLL.h"
#include <iterator>
#include <iostream>
#include <fstream>

#include <vector>
#include <queue>
#include <iostream>
#include <stdint.h>
#include <time.h>
#include <algorithm>
#include <numeric>

#include "priority_deque.hpp"
#include <array>
#include <stack>

#include "point_search.h"
#include <thread>
#include <mutex>
#include <atomic>


using namespace std;
using boost::container::priority_deque;
#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;								// current instance
TCHAR szTitle[MAX_LOADSTRING];					// The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];			// the main window class name

// Forward declarations of functions included in this code module:
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY _tWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPTSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	// TODO: Place code here.
	MSG msg;
	HACCEL hAccelTable;

	// Initialize global strings
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_CHALLENGEDLL, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// Perform application initialization:
	if (!InitInstance(hInstance, nCmdShow))
	{
		return FALSE;
	}

	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_CHALLENGEDLL));

	// Main message loop:
	while (GetMessage(&msg, NULL, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return (int)msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_CHALLENGEDLL));
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = MAKEINTRESOURCE(IDC_CHALLENGEDLL);
	wcex.lpszClassName = szWindowClass;
	wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassEx(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	HWND hWnd;

	hInst = hInstance; // Store instance handle in our global variable

	hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);

	if (!hWnd)
	{
		return FALSE;
	}

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND	- process the application menu
//  WM_PAINT	- Paint the main window
//  WM_DESTROY	- post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;

	switch (message)
	{
	case WM_COMMAND:
		wmId = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		// Parse the menu selections:
		switch (wmId)
		{
		case IDM_ABOUT:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
			break;
		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		// TODO: Add any drawing code here...
		EndPaint(hWnd, &ps);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}


SearchContext* create(const Point* points_begin, const Point* points_end)
{

	//Sleep(5000);
	//ofstream log;
	//log.open("dlllog.txt", ofstream::app);

	//log << "create " << (int)(points_end - points_begin) << "\n";



	PtrIterator<const Point> begin(points_begin);
	PtrIterator<const Point> end(points_end);

	Rect sensibleBounds(-100000000, -100000000, 100000000, 100000000);

	Rect bounds(FLT_MAX, FLT_MAX, -FLT_MAX, -FLT_MAX);
	if (begin != end){

		for (auto p = begin; p != end; ++p){
			if (sensibleBounds.contains(*p)){
				bounds.lx = min(bounds.lx, (*p).x);
				bounds.ly = min(bounds.ly, (*p).y);
				bounds.hx = max(bounds.hx, (*p).x);
				bounds.hy = max(bounds.hy, (*p).y);
			}
		}
		//auto minmaxx = std::minmax_element(begin, end, [](const Point& l, const Point& r) { return l.x < r.x; });
		//auto minmaxy = std::minmax_element(begin, end, [](const Point& l, const  Point& r) { return l.y < r.y; });

		//bounds=Rect((*minmaxx.first).x, (*minmaxy.first).y, (*minmaxx.second).x, (*minmaxy.second).y);
	}

	SearchContext *context = new SearchContext(10, bounds, 100);
	for (auto p = begin; p != end; ++p){

		if (sensibleBounds.contains(*p))
			context->insert(*p);
		else
			context->outlier(*p);
	}

	//context->dump(log);

	context->freeze();

	return context;
}

int32_t search(SearchContext* sc, const Rect rect, const int32_t count, Point* out_points)
{
	//ofstream log;
	//log.open("dlllog.txt", ofstream::app);


	sc->_ncontains = 0;
	sc->_ndescend = 0;
	sc->_nintersects = 0;
	sc->_tq1 = 0;
	sc->_tq2 = 0;
	sc->_tend = 0;
	sc->_ttotal = 0;


	//log << "search " << count << " : " << rect.lx << " " << rect.ly << " " << rect.hx << " " << rect.hy << "\n";

	auto result = sc->query(rect, count, out_points);

	//log << "_ncontains " << sc->_ncontains << " _nintersects " << sc->_nintersects << " _ndescend " << sc->_ndescend
	//	<< "_tq1 " << sc->_tq1 << "_tq2 " << sc->_tq2 << "_tend " << sc->_tend << "_ttotal " << sc->_ttotal << endl;

	//log << count << "," << rect.lx << "," << rect.ly << "," << rect.hx << "," << rect.hy
	//	<< "," << sc->_ncontains << "," << sc->_nintersects << "," << sc->_ndescend << "," << sc->_tq1 << "," << sc->_tq2 << "," << sc->_tend << "," << sc->_ttotal << endl;


	return result;
}

SearchContext* destroy(SearchContext* sc)
{
	if (sc != NULL){
		delete sc;
	}
	return NULL;
}

__inline int ipow(int x, int p) {
	int i = 1;
	for (int j = 1; j <= p; j++)  i *= x;
	return i;
}



SearchContext::SearchContext(int levels, Rect bounds, int nQuery)
{
	_levels = levels;
	_bounds = bounds;
	_bounds.normalise();
	_nQuery = nQuery;

	_nnodes = (ipow(4, levels) - 1) / (4 - 1);
	_pdeqs = new priority_deque<Point>*[_nnodes]();
	_toplists = new PackedVector<Point>*[_nnodes]();
	_toplistdata = NULL;

	_levelOfs = new int[_levels];
	_levelSize = new int[_levels];
	for (int l = 0; l < _levels; l++){
		_levelOfs[l] = (ipow(4, l) - 1) / (4 - 1);
		_levelSize[l] = ipow(2, l);
	}
}

SearchContext::~SearchContext(){

	for (int i = 0; i < _nnodes; i++){

		if (_pdeqs[i] != NULL)
			delete _pdeqs[i];
	}

	if (_toplistdata != NULL){
		delete _toplistdata;
	}

	delete _pdeqs;
	delete _toplists;
	delete _levelOfs;
	delete _levelSize;
}

void SearchContext::dump(ostream& f){

	f << "_levels=" << _levels << "\n";
	f << "_nQuery=" << _nQuery << "\n";
	f << "_nnodes=" << _nnodes << "\n";
	f << "_bounds=" << _bounds << "\n";

	f << "_levelOfs=[";
	for (int i = 0; i < _levels; i++)
		f << _levelOfs[i] << ",";
	f << "]\n";

	f << "_levelSize=[";
	for (int i = 0; i < _levels; i++)
		f << _levelSize[i] << ",";
	f << "]\n";


	for (int l = 0; l < _levels; l++){

		for (int iy = 0; iy < _levelSize[l]; iy++){

			for (int ix = 0; ix < _levelSize[l]; ix++){

				f << "{ " << l << "\t" << ix << "\t" << iy << "} ";

				auto pdeq = _pdeqs[nodeOffset(l, ix, iy)];

				if (pdeq == NULL)
					f << "NULL";
				else {

					priority_deque<Point> cpdeq = *pdeq;
					f << cpdeq.size() << " ";
					while (!cpdeq.empty()){
						f << cpdeq.minimum() << " ";
						cpdeq.pop_minimum();
					}
				}

				f << "\n";
			}
		}


	}


}

void SearchContext::insert(const Point& p){

	Rect nodeBounds = _bounds;
	int ix = 0, iy = 0;
	for (int l = 0; l < _levels; l++){

		auto result = insertPDeq(&(_pdeqs[nodeOffset(l, ix, iy)]), p, l == _levels - 1 ? INT_MAX : _nQuery);
		if (result == ptInserted){
			break;
		}
		else if (!(result == ptNotInserted)){
			insert(result);
			break;
		}

		float cx = nodeBounds.lx + (nodeBounds.hx - nodeBounds.lx) / 2;
		float cy = nodeBounds.ly + (nodeBounds.hy - nodeBounds.ly) / 2;

		ix *= 2;
		iy *= 2;

		if (p.x >= cx){
			ix++;
			nodeBounds.lx = cx;
		}
		else {
			nodeBounds.hx = cx;
		}

		if (p.y >= cy){
			iy++;
			nodeBounds.ly = cy;
		}
		else {
			nodeBounds.hy = cy;
		}
	}
}

void SearchContext::outlier(const Point& p){

	_outliers.push_back(p);
}


__inline int SearchContext::nodeOffset(int l, int ix, int iy) {
	return _levelOfs[l] + iy * _levelSize[l] + ix;
}

__inline Point SearchContext::insertPDeq(priority_deque<Point>** pdeq, const Point& item, int limit){

	if (*pdeq == NULL){
		*pdeq = new priority_deque<Point>();
	}
	if ((*pdeq)->size() >= limit && item.rank >= (*pdeq)->maximum().rank)
		return ptNotInserted;

	(*pdeq)->push(item);
	if ((*pdeq)->size() > limit){

		auto result = (*pdeq)->maximum();
		(*pdeq)->pop_maximum();
		return result;
	}
	return ptInserted;
}

template<typename Iterator >
void mergepts(vector<Point>*& results, vector<Point>*& buf, int nresults, Iterator toplist_begin, Iterator toplist_end, function<bool(const Point*)> match){

	buf->clear();
	buf->reserve(nresults);
	auto r = results->begin();
	auto p = toplist_begin;

	while (buf->size() < nresults){


		if (p == toplist_end){
			if (r == results->end())
				break;
			else
				buf->push_back(*r++);
		}
		else{
			if (!match(&(*p)))
				p++;
			else if (r == results->end())
				buf->push_back(*p++);
			else {
				if ((*r).rank <= (*p).rank)
				{
					if ((*r).rank == (*p).rank)
						p++;
					buf->push_back(*r++);
				}
				else
					buf->push_back(*p++);
			}
		}

	}
	swap(results, buf);
}


int SearchContext::query(Rect qrect, int count, Point* points){

	LARGE_INTEGER start, ElapsedMicroseconds;
	LARGE_INTEGER Frequency;

	QueryPerformanceFrequency(&Frequency);
	QueryPerformanceCounter(&start);

	qrect.normalise();

	vector<Point> buf1, buf2;
	vector<Point>* fresult = &(buf1);
	vector<Point>* fbuf = &(buf2);

	queryNode(0, 0, 0, _bounds, qrect, fresult, fbuf, count);


	LARGE_INTEGER q1;
	QueryPerformanceCounter(&q1);

	mergepts(fresult, fbuf, count, _outliers.begin(), _outliers.end(), [&](const Point* p) { return qrect.contains(*p); });


	int i;
	for (i = 0; i < fresult->size(); i++){
		points[i] = (*fresult)[i];
	}

	LARGE_INTEGER end;
	QueryPerformanceCounter(&end);

	_tq1+= q1.QuadPart - start.QuadPart;
	//_tq2 += q2.QuadPart - q1.QuadPart;
	//_tend += end.QuadPart - q2.QuadPart;
	_ttotal += end.QuadPart - start.QuadPart;

	return i;
}



void SearchContext::freeze(){

	size_t totalBytes = 0;
	for (int i = 0; i < _nnodes; i++){
		if (_pdeqs[i] != NULL){
			totalBytes += sizeof(PackedVector<Point>) + sizeof(Point)* _pdeqs[i]->size();
		}
	}

	_toplistdata = (PackedVector<Point>*)new uint8_t[totalBytes];
	auto next = _toplistdata;

	for (int i = 0; i < _nnodes; i++){
		if (_pdeqs[i] != NULL){
			_toplists[i] = next;
			for (_toplists[i]->numItems = 0; !_pdeqs[i]->empty(); _toplists[i]->numItems++){
				_toplists[i]->items[_toplists[i]->numItems] = _pdeqs[i]->minimum();
				_pdeqs[i]->pop_minimum();
			}
			delete _pdeqs[i];
			_pdeqs[i] = NULL;
			next = next->next();
		}
		else{
			_toplists[i] = NULL;
		}
	}

}
void SearchContext::queryNode(int l, int ix, int iy, const Rect& nodeBounds, const Rect& qrect, vector<Point>*& results, vector<Point>*& buf, int nresults){

	auto toplist = _toplists[nodeOffset(l, ix, iy)];

	if (toplist == NULL || toplist->numItems == 0)
		return;

	if (results->size() >= nresults && toplist->items[0].rank >= ((*results)[results->size() - 1].rank)) {
		return;
	}

	bool descend = false;
	if (qrect.contains(nodeBounds)){


		_ncontains++;
		descend = true;

		mergepts(results, buf, nresults, toplist->begin(), toplist->end(), [](const Point* p) { return true; });

	}
	else if (qrect.intersects(nodeBounds)){

		_nintersects++;
		descend = true;

		mergepts(results, buf, nresults, toplist->begin(), toplist->end(), [&](const Point* p) { return qrect.contains(*p); });
	}

	if (descend && l < _levels - 1){

			_ndescend++;

			queryNode(l + 1, ix * 2, iy * 2, nodeBounds.ll(), qrect, results, buf, nresults);
			queryNode(l + 1, ix * 2 + 1, iy * 2, nodeBounds.lr(), qrect, results, buf, nresults);
			queryNode(l + 1, ix * 2, iy * 2 + 1, nodeBounds.hl(), qrect, results, buf, nresults);
			queryNode(l + 1, ix * 2 + 1, iy * 2 + 1, nodeBounds.hr(), qrect, results, buf, nresults);
	}
}
//
//void mergeVec(vector<Point*> dest, vector<Point*> src, stack<Point*> temp, int limit){
//
//	if (dest.size() >= limit && (*src.begin())->rank >= (*(dest.end() - 1))->rank)
//		return;
//
//	for (auto i = dest.begin(); i != dest.end(); i++){
//
//		
//	}
//}
//
