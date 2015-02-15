// KrokedV1.cpp 
//
//        point_search reference.dll acme.dll -p10000000 -q100000 -r20 -s642E3E98-6EA650C8-642E3E98-7E75161E-4FE6D148

#include "stdafx.h"

#include <stdio.h>
#include <stdlib.h>
#include <algorithm>
#include <vector>
using namespace std;


#define CCHUNK  512
#define CCHUNK2 512
#include "point_search.h"
//#include "C:\Program Files (x86)\Microsoft Visual Studio 11.0\Team Tools\Performance Tools\x64\PerfSDK\VSPerf.h"


#define QUAD 1
#define DOYS 1
extern "C" __declspec(dllexport) SearchContext* __stdcall create(const Point* points_begin, const Point* points_end)
{
	

		if(!points_begin)
			return(NULL);
#define COUNTQ 20000
		std::vector<Foint> *foints = new std::vector<Foint>(COUNTQ);
		std::vector<Moint> *points = new std::vector<Moint>(points_end-points_begin);
		std::vector<Moint> *points2 = new std::vector<Moint>(points_end-points_begin);
		std::vector<Point> *ranks = new std::vector<Point>(points_end-points_begin);
		points->clear();
		points2->clear();
		ranks->clear();
		foints->clear();
		
		for (int idx=0;idx<points_end-points_begin;++idx)
		{
			Moint it;
			//it.id=points_begin[idx].id;
			it.x=points_begin[idx].x;
			it.y=points_begin[idx].y;
			it.rank=points_begin[idx].rank;
			points->emplace_back(it);
			points2->emplace_back(it);
			ranks->emplace_back(points_begin[idx]);
		}
#ifdef DOYS
		
		//std::copy(points_begin,points_end,points2->begin());
		
#endif	
		
		//std::copy(points_begin,points_end,ranks->begin());
		std::sort(ranks->begin(),ranks->end(), [] (Point p1,Point p2){return p1.rank<p2.rank;})	;

		for (int idx=0;idx<COUNTQ;++idx)
		{
			Foint fit;
			fit.x = ranks->at(idx).x;
			fit.y = ranks->at(idx).y;
			foints->emplace_back(fit);
		}
#if 0
		int ii=0;
		for (std::vector<Point>::iterator it=ranks->begin(); it!=ranks->end(); ++it)
		{
			if(it->rank != ii)
				printf("FAILRANKTEST\n");
			++ii;
		}
#endif		
		int64_t count=points_end-points_begin;

		//std::stable_sort(points->begin(),points->end(), [] (Moint p1,Moint p2){return p1.rank<p2.rank;});	
		std::sort(points->begin(),points->end(), [] (Moint p1,Moint p2){return p1.y<p2.y;})	;
		//std::stable_sort(points->begin(),points->end(), [] (Moint p1,Moint p2){return p1.x<p2.x;});	
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
			std::vector<Moint>::iterator thisbegin=points->begin()+(chunk);
			std::vector<Moint>::iterator thisend;
			if(distance(thisbegin,points->end())<chunksize)
				thisend=points->end();
			else
				thisend=points->begin()+(chunk+chunksize);
			yrlow.emplace_back(thisbegin->y);
			yrhigh.emplace_back((thisend-1)->y);
			std::stable_sort(thisbegin,thisend, [] (Moint p1,Moint p2){return p1.x<p2.x;});
		}
#else
		std::stable_sort(points->begin(),points->end(), [] (Moint p1,Moint p2){return p1.x<p2.x;});	
#endif
		
#ifdef DOYS
		//std::stable_sort(points2->begin(),points2->end(), [] (Moint p1,Moint p2){return p1.rank<p2.rank;});	
		std::sort(points2->begin(),points2->end(), [] (Moint p1,Moint p2){return p1.x<p2.x;});
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
			std::vector<Moint>::iterator thisbegin2=points2->begin()+(chunk2);
			std::vector<Moint>::iterator thisend2;
			if(distance(thisbegin2,points2->end())<chunksize2)

				thisend2=points2->end();
			else
				thisend2=points2->begin()+(chunk2+chunksize2);
			xrlow.emplace_back(thisbegin2->x);
			xrhigh.emplace_back((thisend2-1)->x);
			std::stable_sort(thisbegin2,thisend2, [] (Moint p1,Moint p2){return p1.y<p2.y;});
		}
#else
		std::stable_sort(points2->begin(),points2->end(), [] (Moint p1,Moint p2){return p1.y<p2.y;});
#endif
#endif

#if 0
		  for (std::vector<Moint>::iterator it=points->begin(); it!=points->end(); ++it)
			printf("x=%f,y=%f,rank=%d\n",it->x,it->y,it->rank);
		printf("count=%d, \n",count);
#endif	
		SearchContext *sc=new SearchContext;
		sc->count = count;
		sc->points= (Moint *) &points->begin()[0];
		sc->vpoints=points;
#ifdef DOYS
		sc->vpoints2=points2;
#endif
		sc->scratch= new std::vector<Moint>();
		sc->scratch->reserve(points_end-points_begin);
		sc->yrlow = yrlow;
		sc->yrhigh = yrhigh;
		sc->xrlow=xrlow;
		sc->xrhigh=xrhigh;
		sc->ranks=ranks;
		sc->foints=foints;
		
		return((SearchContext *)sc);
}

extern "C" __declspec(dllexport) int32_t search(const SearchContext* sc, const Rect rect, const int32_t count, Point* out_points)
{
	if(!sc)
		return(0);
//StartProfile(PROFILE_GLOBALLEVEL, PROFILE_CURRENTID);
	const std::vector<Foint> *foints= sc->foints;
	const std::vector<Point> *ranks= sc->ranks;

	int idx=0;
	for (int i=0;i<COUNTQ;++i)
	{
		if((foints->at(i).x>=rect.lx) && (foints->at(i).x<=rect.hx) && (foints->at(i).y>=rect.ly) && (foints->at(i).y<=rect.hy))
		{
			out_points[idx]=ranks->at(i);

			++idx;
			if(idx>=count)
			{
				return(idx);
			}
			
		}
	}
	
	const std::vector<Moint> *points = sc->vpoints;
	const std::vector<Moint> *points2 = sc->vpoints2;
	std::vector<Moint> *vlist = sc->scratch;
	
	std::vector<Moint>::const_iterator sclow[CCHUNK+1];
	std::vector<Moint>::const_iterator schigh[CCHUNK+1];
	std::vector<Moint>::const_iterator sclow2[CCHUNK2+1];
	std::vector<Moint>::const_iterator schigh2[CCHUNK2+1];

	
	int32_t lrank=MAXINT32;
	int iv=0;
	//int ov=0;
	//int pv=0;
	int64_t yblockcount=0;
	int64_t xblockcount=0;
	//int txcount=0;
	//int tycount=0;
#ifdef DOYS
	{
	size_t chunk2;
	size_t chunksize2;
	std::vector<float>::const_iterator xrlow=sc->xrlow.begin();
	std::vector<float>::const_iterator xrhigh=sc->xrhigh.begin();

	chunksize2=CCHUNK2;
	if(points2->size() > CCHUNK2*10)
		chunksize2 = points2->size()/CCHUNK2;

	for(int n=0,chunk2=0;chunk2<points2->size();chunk2+=chunksize2,n++)
	{
		float thischunkhigh=*xrhigh;
		++xrhigh;
		if(rect.lx > thischunkhigh)
			continue;

		float thischunklow=*xrlow;
		++xrlow;
		if(rect.hx < thischunklow)
			continue;
		std::vector<Moint>::const_iterator thisbegin2=points2->begin()+(chunk2);
		std::vector<Moint>::const_iterator thisend2;
		if(distance(thisbegin2,points2->end())<chunksize2)
			thisend2=points2->end();
		else
			thisend2=points2->begin()+(chunk2+chunksize2);
			
		std::vector<Moint>::const_iterator low2,high2;
		Moint Foo2,Bar2;
		Foo2.y=rect.ly;
		Bar2.y=rect.hy;
		low2=std::lower_bound (thisbegin2, thisend2, Moint(Foo2),[](Moint p1,Moint p2){return p1.y < p2.y;});
		high2=std::upper_bound (low2, thisend2, Moint(Bar2),[](Moint p1,Moint p2){return p1.y < p2.y;});
		sclow2[n]=low2;
		schigh2[n]=high2;
	
		yblockcount+=distance(low2,high2);
	}
	}
	{
	size_t chunk;
	size_t chunksize;
	std::vector<float>::const_iterator yrlow=sc->yrlow.begin();
	std::vector<float>::const_iterator yrhigh=sc->yrhigh.begin();

	chunksize=CCHUNK;
	if(points->size() > CCHUNK*10)
		chunksize = points->size()/CCHUNK;

	for(int n=0,chunk=0;chunk<points->size();chunk+=chunksize,n++)
	{
		float thischunkhigh=*yrhigh;
		++yrhigh;
		if(rect.ly > thischunkhigh)
			continue;

		float thischunklow=*yrlow;
		++yrlow;
		if(rect.hy < thischunklow)
			continue;
		std::vector<Moint>::const_iterator thisbegin=points->begin()+(chunk);
		std::vector<Moint>::const_iterator thisend;
		if(distance(thisbegin,points->end())<chunksize)
			thisend=points->end();
		else
			thisend=points->begin()+(chunk+chunksize);
				
		std::vector<Moint>::const_iterator low,high;
		Moint Foo,Bar;
		Foo.x=rect.lx;
		Bar.x=rect.hx;
		low=std::lower_bound (thisbegin, thisend, Moint(Foo),[](Moint p1,Moint p2){return p1.x < p2.x;});
		high=std::upper_bound (low, thisend, Moint(Bar),[](Moint p1,Moint p2){return p1.x < p2.x;});
	
		sclow[n]=low;
		schigh[n]=high;
		xblockcount+=distance(low,high);
	}
	}
	if(min(xblockcount,yblockcount)==0)
		return(0);
	//printf("yblock=%d,xblock=%d\t min=%d\n",yblockcount,xblockcount,min(xblockcount,yblockcount));
	if(yblockcount<xblockcount)
{

	size_t chunk2;
	size_t chunksize2;
	std::vector<float>::const_iterator xrlow=sc->xrlow.begin();
	std::vector<float>::const_iterator xrhigh=sc->xrhigh.begin();

	chunksize2=CCHUNK2;
	if(points2->size() > CCHUNK2*10)
		chunksize2 = points2->size()/CCHUNK2;

	for(int n=0,chunk2=0;chunk2<points2->size();chunk2+=chunksize2,n++)
	{
		float thischunkhigh=*xrhigh;
		++xrhigh;
		if(rect.lx > thischunkhigh)
			continue;

		float thischunklow=*xrlow;
		++xrlow;
		if(rect.hx < thischunklow)
			continue;
#if 0
//++tycount;
		std::vector<Moint>::const_iterator thisbegin2=points2->begin()+(chunk2);
		std::vector<Moint>::const_iterator thisend2;
		if(distance(thisbegin2,points2->end())<chunksize2)
			thisend2=points2->end();
		else
			thisend2=points2->begin()+(chunk2+chunksize2);
				
	std::vector<Moint>::const_iterator low2,high2;
	Moint Foo2,Bar2;
	Foo2.y=rect.ly;
	Bar2.y=rect.hy;
	low2=std::lower_bound (thisbegin2, thisend2, Moint(Foo2),[](Moint p1,Moint p2){return p1.y < p2.y;});
	high2=std::upper_bound (low2, thisend2, Moint(Bar2),[](Moint p1,Moint p2){return p1.y < p2.y;});
#endif
	std::vector<Moint>::const_iterator low2,high2;
	low2=sclow2[n];
	high2=schigh2[n];

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
						std::partial_sort(vlist->begin(),vlist->begin()+min(iv,count),vlist->end(),[] (Moint p1,Moint p2){return p1.rank<p2.rank;});
						vlist->resize(count);
						lrank=vlist->at(count-1).rank;
#if 0
						if(lrank < chunksize2-1) //bugbug -1?
						{
							iv=0;
							vlist->clear();
							for (std::vector<Point>::const_iterator it=ranks->begin(); it!=ranks->begin()+lrank+1; ++it)
							{
									if((it->x>=rect.lx) && (it->x<=rect.hx) && (it->y>=rect.ly) && (it->y<=rect.hy))
									{
										Moint tit;
										tit.x=it->x;
										tit.y=it->y;
										tit.rank=it->rank;
										vlist->emplace_back(tit);
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

	size_t chunk;
	size_t chunksize;
	std::vector<float>::const_iterator yrlow=sc->yrlow.begin();
	std::vector<float>::const_iterator yrhigh=sc->yrhigh.begin();

	chunksize=CCHUNK;
	if(points->size() > CCHUNK*10)
		chunksize = points->size()/CCHUNK;
	for(int n=0,chunk=0;chunk<points->size();chunk+=chunksize,n++)
	{
		float thischunkhigh=*yrhigh;
		++yrhigh;
		if(rect.ly > thischunkhigh)
			continue;

		float thischunklow=*yrlow;
		++yrlow;
		if(rect.hy < thischunklow)
			continue;
#if 0
//++txcount;
		std::vector<Moint>::const_iterator thisbegin=points->begin()+(chunk);
		std::vector<Moint>::const_iterator thisend;
		if(distance(thisbegin,points->end())<chunksize)
			thisend=points->end();
		else
			thisend=points->begin()+(chunk+chunksize);
		
		

	std::vector<Moint>::const_iterator low,high;
	Moint Foo,Bar;
	Foo.x=rect.lx;
	Bar.x=rect.hx;
	low=std::lower_bound (thisbegin, thisend, Moint(Foo),[](Moint p1,Moint p2){return p1.x < p2.x;});
	high=std::upper_bound (low, thisend, Moint(Bar),[](Moint p1,Moint p2){return p1.x < p2.x;});
#endif
	std::vector<Moint>::const_iterator low,high;
	low=sclow[n];
	high=schigh[n];
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
						std::partial_sort(vlist->begin(),vlist->begin()+min(iv,count),vlist->end(),[] (Moint p1,Moint p2){return p1.rank<p2.rank;});
						vlist->resize(count);
						lrank=vlist->at(count-1).rank;
				
					}
#if 0
						if(lrank < chunksize-1) //bugbug -1?
						{
							iv=0;
							vlist->clear();
							for (std::vector<Point>::iterator it=ranks->begin(); it!=ranks->begin()+lrank+1; ++it)
							{
									if((it->x>=rect.lx) && (it->x<=rect.hx) && (it->y>=rect.ly) && (it->y<=rect.hy))
									{
										Moint tit;
										tit.x=it->x;
										tit.y=it->y;
										tit.rank=it->rank;
										vlist->emplace_back(tit);
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
		std::partial_sort(vlist->begin(),vlist->begin()+min(iv,count),vlist->end(),[] (Moint p1,Moint p2){return p1.rank<p2.rank;});
		//std::copy( vlist->begin(), vlist->begin()+min(iv,count), out_points);
		for (int idx=0;idx<min(iv,count);idx++)
		{
				//out_points[idx].id=vlist->at(idx).id;
			    int32_t rank=vlist->at(idx).rank;
				out_points[idx].id=ranks->at(rank).id;
				out_points[idx].x=vlist->at(idx).x;
				out_points[idx].y=vlist->at(idx).y;
				out_points[idx].rank=vlist->at(idx).rank;
		}
							
		vlist->clear();
		//printf("txcount=%d,tycount=%d\n",txcount,tycount);
		//printf("ov=%d,pv=%d,iv=%d\n",ov,pv,iv);
	}
	//StopProfile(PROFILE_GLOBALLEVEL, PROFILE_CURRENTID);
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
	
	return (NULL);
}