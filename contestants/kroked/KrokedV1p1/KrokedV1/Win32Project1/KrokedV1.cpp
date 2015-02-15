// KrokedV1.cpp 
//
//        point_search reference.dll acme.dll -p10000000 -q100000 -r20 -s642E3E98-6EA650C8-642E3E98-7E75161E-4FE6D148

#include "stdafx.h"

#include <stdio.h>
#include <stdlib.h>
#include <algorithm>
#include <vector>
using namespace std;
#include "point_search.h"
//#include "C:\Program Files (x86)\Microsoft Visual Studio 11.0\Team Tools\Performance Tools\x64\PerfSDK\VSPerf.h"

#define CCHUNK  512
#define CCHUNK2 512

#define QUAD 1
#define DOYS 1
extern "C" __declspec(dllexport) SearchContext* __stdcall create(const Point* points_begin, const Point* points_end)
{
	

		if(!points_begin)
			return(NULL);

		std::vector<Point> *points = new std::vector<Point>(points_end-points_begin);
		std::copy(points_begin,points_end,points->begin());
#ifdef DOYS
		std::vector<Point> *points2 = new std::vector<Point>(points_end-points_begin);
		std::copy(points_begin,points_end,points2->begin());
#endif	
		std::vector<Point> *ranks = new std::vector<Point>(points_end-points_begin);
		std::copy(points_begin,points_end,ranks->begin());
		std::sort(ranks->begin(),ranks->end(), [] (Point p1,Point p2){return p1.rank<p2.rank;})	;
		
		
		int64_t count=points_end-points_begin;

		//std::stable_sort(points->begin(),points->end(), [] (Point p1,Point p2){return p1.rank<p2.rank;});	
		std::sort(points->begin(),points->end(), [] (Point p1,Point p2){return p1.y<p2.y;})	;
		//std::stable_sort(points->begin(),points->end(), [] (Point p1,Point p2){return p1.x<p2.x;});	
#ifdef QUAD
		size_t chunk;
		size_t chunksize;

		std::vector<float> yrlow;
		std::vector<float> yrhigh;

		chunksize=CCHUNK;
		if(points->size() > CCHUNK*10)
			chunksize = points->size()/CCHUNK;
		for(chunk=0;chunk<points->size();chunk+=chunksize)
		{
			std::vector<Point>::iterator thisbegin=points->begin()+(chunk);
			std::vector<Point>::iterator thisend;
			if(distance(thisbegin,points->end())<chunksize)
				thisend=points->end();
			else
				thisend=points->begin()+(chunk+chunksize);
			yrlow.emplace_back(thisbegin->y);
			yrhigh.emplace_back((thisend-1)->y);
			std::stable_sort(thisbegin,thisend, [] (Point p1,Point p2){return p1.x<p2.x;});
		}
#else
		std::stable_sort(points->begin(),points->end(), [] (Point p1,Point p2){return p1.x<p2.x;});	
#endif
		
#ifdef DOYS
		//std::stable_sort(points2->begin(),points2->end(), [] (Point p1,Point p2){return p1.rank<p2.rank;});	
		std::sort(points2->begin(),points2->end(), [] (Point p1,Point p2){return p1.x<p2.x;});
#ifdef QUAD
		size_t chunk2;
		size_t chunksize2;

		std::vector<float> xrlow;
		std::vector<float> xrhigh;

		chunksize2=CCHUNK2;
		if(points2->size() > CCHUNK2*10)
			chunksize2 = points2->size()/CCHUNK2;
		for(chunk2=0;chunk2<points2->size();chunk2+=chunksize2)
		{
			std::vector<Point>::iterator thisbegin2=points2->begin()+(chunk2);
			std::vector<Point>::iterator thisend2;
			if(distance(thisbegin2,points2->end())<chunksize2)

				thisend2=points2->end();
			else
				thisend2=points2->begin()+(chunk2+chunksize2);
			xrlow.emplace_back(thisbegin2->x);
			xrhigh.emplace_back((thisend2-1)->x);
			std::stable_sort(thisbegin2,thisend2, [] (Point p1,Point p2){return p1.y<p2.y;});
		}
#else
		std::stable_sort(points2->begin(),points2->end(), [] (Point p1,Point p2){return p1.y<p2.y;});
#endif
#endif

#if 0
		  for (std::vector<Point>::iterator it=points->begin(); it!=points->end(); ++it)
			printf("x=%f,y=%f,rank=%d\n",it->x,it->y,it->rank);
		printf("count=%d, \n",count);
#endif	
		SearchContext *sc=new SearchContext;
		sc->count = count;
		sc->points= (Point *) &points->begin()[0];
		sc->vpoints=points;
#ifdef DOYS
		sc->vpoints2=points2;
#endif
		sc->scratch= new std::vector<Point>();
		sc->scratch->reserve(points_end-points_begin);
		sc->yrlow = yrlow;
		sc->yrhigh = yrhigh;
		sc->xrlow=xrlow;
		sc->xrhigh=xrhigh;
		sc->ranks=ranks;
		
		//StartProfile(PROFILE_GLOBALLEVEL, PROFILE_CURRENTID);
		return((SearchContext *)sc);
}

extern "C" __declspec(dllexport) int32_t search(SearchContext* sc, const Rect rect, const int32_t count, Point* out_points)
{
	if(!sc)
		return(0);

	Point *list= out_points;
	
	std::vector<Point> *points = sc->vpoints;
	std::vector<Point> *points2 = sc->vpoints2;
	std::vector<Point> *vlist = sc->scratch;
	std::vector<Point> *ranks= sc->ranks;

	

	
	int32_t lrank=MAXINT32;
	int iv=0;
	//int ov=0;
	//int pv=0;
	int yblockcount=0;
	int xblockcount=0;
	//int txcount=0;
	//int tycount=0;
#ifdef DOYS
	{
	size_t chunk2;
	size_t chunksize2;
	std::vector<float>::iterator xrlow=sc->xrlow.begin();
	std::vector<float>::iterator xrhigh=sc->xrhigh.begin();

	chunksize2=CCHUNK2;
	if(points2->size() > CCHUNK2*10)
		chunksize2 = points2->size()/CCHUNK2;
	for(chunk2=0;chunk2<points2->size();chunk2+=chunksize2)
	{
		float thischunkhigh=*xrhigh;
		++xrhigh;
		if(rect.lx > thischunkhigh)
			continue;

		float thischunklow=*xrlow;
		++xrlow;
		if(rect.hx < thischunklow)
			continue;
		++yblockcount;
	}
	}
	{
	size_t chunk;
	size_t chunksize;
	std::vector<float>::iterator yrlow=sc->yrlow.begin();
	std::vector<float>::iterator yrhigh=sc->yrhigh.begin();

	chunksize=CCHUNK;
	if(points->size() > CCHUNK*10)
		chunksize = points->size()/CCHUNK;
	for(chunk=0;chunk<points->size();chunk+=chunksize)
	{
		float thischunkhigh=*yrhigh;
		++yrhigh;
		if(rect.ly > thischunkhigh)
			continue;

		float thischunklow=*yrlow;
		++yrlow;
		if(rect.hy < thischunklow)
			continue;
		++xblockcount;
	}
	}
	//printf("yblock=%d,xblock=%d\n",yblockcount,xblockcount);
	//if(yblockcount>xblockcount)  //backwards check is faster..whaa??
	if(yblockcount>xblockcount)
{
	#ifdef QUAD 
	size_t chunk2;
	size_t chunksize2;
	std::vector<float>::iterator xrlow=sc->xrlow.begin();
	std::vector<float>::iterator xrhigh=sc->xrhigh.begin();

	chunksize2=CCHUNK2;
	if(points2->size() > CCHUNK2*10)
		chunksize2 = points2->size()/CCHUNK2;
	for(chunk2=0;chunk2<points2->size();chunk2+=chunksize2)
	{
		float thischunkhigh=*xrhigh;
		++xrhigh;
		if(rect.lx > thischunkhigh)
			continue;

		float thischunklow=*xrlow;
		++xrlow;
		if(rect.hx < thischunklow)
			continue;

//++tycount;
		std::vector<Point>::iterator thisbegin2=points2->begin()+(chunk2);
		std::vector<Point>::iterator thisend2;
		if(distance(thisbegin2,points2->end())<chunksize2)
			thisend2=points2->end();
		else
			thisend2=points2->begin()+(chunk2+chunksize2);
				
#endif
	std::vector<Point>::iterator low2,high2;
	Point Foo2,Bar2;
	Foo2.y=rect.ly;
	Bar2.y=rect.hy;
	low2=std::lower_bound (thisbegin2, thisend2, Point(Foo2),[](Point p1,Point p2){return p1.y < p2.y;});
	high2=std::upper_bound (low2, thisend2, Point(Bar2),[](Point p1,Point p2){return p1.y < p2.y;});
	
	for(;low2!=high2;++low2)
	{	
//++ov;
			if(low2->rank < lrank)
			if((low2->x>=rect.lx) && (low2->x<=rect.hx))
			{
//++pv;
				//if(low2->rank < lrank)
				{
					vlist->emplace_back(*low2);
					++iv;
#if 1
#define MYMASK 0x3F
					if((iv&MYMASK)==0)
					{
						std::partial_sort(vlist->begin(),vlist->begin()+min(iv,count),vlist->end(),[] (Point p1,Point p2){return p1.rank<p2.rank;});
						vlist->resize(count);
						lrank=vlist->at(count-1).rank;
#if 1
						if(lrank < chunksize2-1) //bugbug -1?
						{
							iv=0;
							vlist->clear();
							for (std::vector<Point>::iterator it=ranks->begin(); it!=ranks->begin()+lrank+1; ++it)
							{
									if((it->x>=rect.lx) && (it->x<=rect.hx) && (it->y>=rect.ly) && (it->y<=rect.hy))
									{
										vlist->emplace_back(*it);
										++iv;
									}
									
							}	
							goto quickfinish;
						}
#endif
				
					}
#endif
				}

			}
	}
#ifdef QUAD
	}
#endif
	}
	else
	{
#endif
#ifdef QUAD 
	size_t chunk;
	size_t chunksize;
	std::vector<float>::iterator yrlow=sc->yrlow.begin();
	std::vector<float>::iterator yrhigh=sc->yrhigh.begin();

	chunksize=CCHUNK;
	if(points->size() > CCHUNK*10)
		chunksize = points->size()/CCHUNK;
	for(chunk=0;chunk<points->size();chunk+=chunksize)
	{
		float thischunkhigh=*yrhigh;
		++yrhigh;
		if(rect.ly > thischunkhigh)
			continue;

		float thischunklow=*yrlow;
		++yrlow;
		if(rect.hy < thischunklow)
			continue;

//++txcount;
		std::vector<Point>::iterator thisbegin=points->begin()+(chunk);
		std::vector<Point>::iterator thisend;
		if(distance(thisbegin,points->end())<chunksize)
			thisend=points->end();
		else
			thisend=points->begin()+(chunk+chunksize);
		
		
#endif
	std::vector<Point>::iterator low,high;
	Point Foo,Bar;
	Foo.x=rect.lx;
	Bar.x=rect.hx;
	low=std::lower_bound (thisbegin, thisend, Point(Foo),[](Point p1,Point p2){return p1.x < p2.x;});
	high=std::upper_bound (low, thisend, Point(Bar),[](Point p1,Point p2){return p1.x < p2.x;});
	
	for(;low!=high;++low)
	{	
//++ov;		
			if(low->rank < lrank)
			if((low->y>=rect.ly) && (low->y<=rect.hy))
			{
//++pv;
				//if(low->rank < lrank)
				{
					vlist->emplace_back(*low);
					++iv;
#if 1
					if((iv&MYMASK)==0)
					{
						std::partial_sort(vlist->begin(),vlist->begin()+min(iv,count),vlist->end(),[] (Point p1,Point p2){return p1.rank<p2.rank;});
						vlist->resize(count);
						lrank=vlist->at(count-1).rank;
				
					}
#if 1
						if(lrank < chunksize-1) //bugbug -1?
						{
							iv=0;
							vlist->clear();
							for (std::vector<Point>::iterator it=ranks->begin(); it!=ranks->begin()+lrank+1; ++it)
							{
									if((it->x>=rect.lx) && (it->x<=rect.hx) && (it->y>=rect.ly) && (it->y<=rect.hy))
									{
										vlist->emplace_back(*it);
										++iv;
									}
									
							}	
							goto quickfinish;
						}
#endif
#endif
				}
			}
	}
#ifdef QUAD
	}
#endif
	}
	if(iv)
	{
quickfinish:
		std::partial_sort(vlist->begin(),vlist->begin()+min(iv,count),vlist->end(),[] (Point p1,Point p2){return p1.rank<p2.rank;});
		std::copy( vlist->begin(), vlist->begin()+min(iv,count), out_points);
		vlist->clear();
		//printf("txcount=%d,tycount=%d\n",txcount,tycount);
		//printf("ov=%d,pv=%d,iv=%d\n",ov,pv,iv);
	}
	
	return(min(iv,count));
}

extern "C" __declspec(dllexport) SearchContext *destroy(SearchContext *sc)
{
	if(sc)
	{
		if(sc->points)
			free(sc->points);
		if(sc->scratch)
			free(sc->scratch);
		//if(sc->vpoints)
		//	delete sc->vpoints;
		//if(sc->vpoints2)
		//	delete sc->vpoints2;
		delete sc;
	}
	//StopProfile(PROFILE_GLOBALLEVEL, PROFILE_CURRENTID);
	return (NULL);
}


