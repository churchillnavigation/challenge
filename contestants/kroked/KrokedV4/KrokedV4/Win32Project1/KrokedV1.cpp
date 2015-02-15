// KrokedV1.cpp 
//
//        point_search reference.dll acme.dll -p10000000 -q100000 -r20 -s642E3E98-6EA650C8-642E3E98-7E75161E-4FE6D148

#include "stdafx.h"

#include <stdio.h>
#include <stdlib.h>
#include <algorithm>
#include <vector>
using namespace std;


#define CCHUNK 5
#define CCHUNK2 5
#define COUNTPLAT 33

//#define FLOATPOINT 1
//#define MUL 1.0

#define MUL 1000000.0
//#define MUL 10000000L
//#define FIXED64 1
#define FIXED32 1
//#define LIES (2147)
#define ptinrect(PT,RECT) ((PT.x>=rlx) && (PT.x<=rhx) && (PT.y>=rly) && (PT.y<=rhy))
#define fptinrect(PT,RECT) ((PT.fx>=frlx) && (PT.fx<=frhx) && (PT.fy>=frly) && (PT.fy<=frhy))
#include "point_search.h"

#define PROF 0
#if PROF
#include "C:\Program Files (x86)\Microsoft Visual Studio 11.0\Team Tools\Performance Tools\x64\PerfSDK\VSPerf.h"
#endif

#define COUNTQ 6000
bool nofixed=false;
#if 1
void splitx(std::vector<Moint> *points,Rect outterbounds,std::vector<fFoint> **inleft,std::vector<int32_t> **inleftrank,Rect *boundsleft,std::vector<fFoint> **inright,std::vector<int32_t> **inrightrank,Rect *boundsright,std::vector<Moint> **mmointsleft,std::vector<Moint> **mmointsright,int qlimit)
{
	std::vector<Moint> *spoints=new std::vector<Moint>(points->end()-points->begin());

	std::copy(points->begin(),points->end(),spoints->begin());

	std::sort(spoints->begin(),spoints->end(), [] (Moint p1,Moint p2){return p1.x<p2.x;});
	std::size_t const half_size = spoints->size() / 2;
	std::vector<Moint> *left;
	std::vector<Moint> *right;
	left = new std::vector<Moint>(spoints->begin(),spoints->begin()+half_size);
	right =new std::vector<Moint>(spoints->begin()+half_size,spoints->end());
	boundsleft->lx=outterbounds.lx;
	boundsleft->hx=left->back().x;
	boundsright->lx=right->begin()->x;
	boundsright->hx=outterbounds.hx;

	boundsleft->ly=outterbounds.ly;
	boundsleft->hy=outterbounds.hy;
	boundsright->ly=outterbounds.ly;
	boundsright->hy=outterbounds.hy;
	std::sort(left->begin(),left->end(), [] (Moint p1,Moint p2){return p1.rank<p2.rank;});
	std::sort(right->begin(),right->end(), [] (Moint p1,Moint p2){return p1.rank<p2.rank;});
	if(left->size()>qlimit)
		left->resize(qlimit);
	if(right->size()>qlimit)
		right->resize(qlimit);

	std::vector<fFoint> *leftout =new std::vector<fFoint>;
	std::vector<int32_t> *leftrankout = new std::vector<int32_t>;
	std::vector<fFoint> *rightout =new std::vector<fFoint>;
	std::vector<int32_t> *rightrankout = new std::vector<int32_t>;
	for(int i=0;i<left->size();++i)
	{
		fFoint it;
		if(!nofixed)
		{
			it.fx=left->at(i).x*MUL;
			it.fy=left->at(i).y*MUL;
		}
		else
		{
			it.x=left->at(i).x;
			it.y=left->at(i).y;
		}
			
		leftout->emplace_back(it);
		leftrankout->emplace_back(left->at(i).rank);
	}
	for(int i=0;i<right->size();++i)
	{
		fFoint it;
		if(!nofixed)
		{
			it.fx=right->at(i).x*MUL;
			it.fy=right->at(i).y*MUL;
		}
		else
		{
			it.x=right->at(i).x;
			it.y=right->at(i).y;
		}
		
		rightout->emplace_back(it);
		rightrankout->emplace_back(right->at(i).rank);
	}
	*inright=rightout;
	*inrightrank=rightrankout;
	*inleft=leftout;
	*inleftrank=leftrankout;
	*mmointsright=right;
	*mmointsleft=left;

	delete spoints;
}
#endif
#if 1
void splity(std::vector<Moint> *points,Rect outterbounds,std::vector<fFoint> **inbottom,std::vector<int32_t> **inbottomrank,Rect *boundsbottom,std::vector<fFoint> **intop,std::vector<int32_t> **intoprank,Rect *boundstop,std::vector<Moint> **mmointsbottom,std::vector<Moint> **mmointstop, int qlimit)
{
	std::vector<Moint> *spoints=new std::vector<Moint>(points->end()-points->begin());

	std::copy(points->begin(),points->end(),spoints->begin());

	std::sort(spoints->begin(),spoints->end(), [] (Moint p1,Moint p2){return p1.y<p2.y;});
	std::size_t const half_size = spoints->size() / 2;
	std::vector<Moint> *bottom;
	std::vector<Moint> *top;
	bottom =new std::vector<Moint>(spoints->begin(),spoints->begin()+half_size);
	top = new std::vector<Moint>(spoints->begin()+half_size,spoints->end());
	boundsbottom->ly=outterbounds.ly;
	boundsbottom->hy=bottom->back().y;
	boundstop->ly=top->begin()->y;
	boundstop->hy=outterbounds.hy;
	
	boundsbottom->lx=outterbounds.lx;
	boundsbottom->hx=outterbounds.hx;
	boundstop->lx=outterbounds.lx;
	boundstop->hx=outterbounds.hx;
	std::sort(bottom->begin(),bottom->end(), [] (Moint p1,Moint p2){return p1.rank<p2.rank;});
	std::sort(top->begin(),top->end(), [] (Moint p1,Moint p2){return p1.rank<p2.rank;});

	if(top->size()>qlimit)
		top->resize(qlimit);
	if(bottom->size()>qlimit)
		bottom->resize(qlimit);
	std::vector<fFoint> *bottomout =new std::vector<fFoint>;
	std::vector<int32_t> *bottomrankout = new std::vector<int32_t>;
	std::vector<fFoint> *topout =new std::vector<fFoint>;
	std::vector<int32_t> *toprankout = new std::vector<int32_t>;
	for(int i=0;i<bottom->size();++i)
	{
		fFoint it;
		if(!nofixed)
		{
			it.fx=bottom->at(i).x*MUL;
			it.fy=bottom->at(i).y*MUL;
		}
		else
		{
			it.x=bottom->at(i).x;
			it.y=bottom->at(i).y;
		}

		bottomout->emplace_back(it);
		bottomrankout->emplace_back(bottom->at(i).rank);
	}
	for(int i=0;i<top->size();++i)
	{
		fFoint it;
		if(!nofixed)
		{
			it.fx=top->at(i).x*MUL;
			it.fy=top->at(i).y*MUL;
		}
		else
		{
			it.x=top->at(i).x;
			it.y=top->at(i).y;
		}

		topout->emplace_back(it);
		toprankout->emplace_back(top->at(i).rank);
	}
	*inbottom=bottomout;
	*inbottomrank=bottomrankout;
	*intop=topout;
	*intoprank=toprankout;
	*mmointstop=top;
	*mmointsbottom=bottom;

	delete spoints;
}
#endif

#define contains(r1,r2) ((r1.lx>r2.lx) && (r1.hx<r2.hx) && (r1.ly>r2.ly) && (r1.hy<r2.hy))

extern "C" __declspec(dllexport) SearchContext* __stdcall create(const Point* points_begin, const Point* points_end)
{
		
		if(!points_begin)
			return(NULL);
		nofixed=false;
		std::vector<fFoint> *foints = new std::vector<fFoint>(COUNTQ);
		
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

			if(!nofixed)
			{
				if(abs(it.x)<10000000.0)
				{
					float bob=it.x*MUL;
					float bill=bob - (long)bob;
					if(bill!=0)
					{
						nofixed=TRUE;
						//printf("NOFIXED",bill);
					}
				}
				if(abs(it.y)<10000000.0)
				{
					float bob=it.y*MUL;
					float bill=bob - (long)bob;
					if(bill!=0)
					{
						nofixed=TRUE;
						//printf("NOFIXED",bill);
					}

				}
				
			}

#ifdef LIES			
			if(it.x>LIES)
				it.x=LIES;
			if(it.x< -LIES)
					it.x=-LIES;
			if(it.y>LIES)
				it.y=LIES;
			if(it.y< -LIES)
					it.y=-LIES;
#endif
			it.rank=points_begin[idx].rank;
			points->emplace_back(it);
			points2->emplace_back(it);
			ranks->emplace_back(points_begin[idx]);
		}
//		
#if 1
	std::vector<Moint> *tlist[COUNTPLAT];//bugbug free
	std::vector<fFoint> *rlist[COUNTPLAT];
	std::vector<int32_t> *rrank[COUNTPLAT];
	Rect bounds[COUNTPLAT];
	Rect outterbounds;
	outterbounds.lx=-10000000001.0;
	outterbounds.ly=-10000000001.0;
	outterbounds.hx= 10000000001.0;
	outterbounds.hy= 10000000001.0;
	splity(points,outterbounds,&rlist[1],&rrank[1],&bounds[1],&rlist[2],&rrank[2],&bounds[2],&tlist[1],&tlist[2],COUNTQ);
	delete tlist[1];delete tlist[2];
	splitx(points,outterbounds,&rlist[3],&rrank[3],&bounds[3],&rlist[4],&rrank[4],&bounds[4],&tlist[3],&tlist[4],COUNTQ);
		splity(tlist[3],bounds[3],&rlist[5],&rrank[5],&bounds[5],&rlist[6],&rrank[6],&bounds[6],&tlist[5],&tlist[6],COUNTQ);
		delete tlist[3];
		splity(tlist[4],bounds[4],&rlist[7],&rrank[7],&bounds[7],&rlist[8],&rrank[8],&bounds[8],&tlist[7],&tlist[8],COUNTQ);
		delete tlist[4];
			splitx(tlist[5],bounds[5],&rlist[9],&rrank[9],&bounds[9],&rlist[10],&rrank[10],&bounds[10],&tlist[9],&tlist[10],COUNTQ);
			delete tlist[5];
			splitx(tlist[6],bounds[6],&rlist[11],&rrank[11],&bounds[11],&rlist[12],&rrank[12],&bounds[12],&tlist[11],&tlist[12],COUNTQ);
			delete tlist[6];
			splitx(tlist[7],bounds[7],&rlist[13],&rrank[13],&bounds[13],&rlist[14],&rrank[14],&bounds[14],&tlist[13],&tlist[14],COUNTQ);
			delete tlist[7];
			splitx(tlist[8],bounds[8],&rlist[15],&rrank[15],&bounds[15],&rlist[16],&rrank[16],&bounds[16],&tlist[15],&tlist[16],COUNTQ);
			delete tlist[8];
				splity(tlist[9],bounds[9],&rlist[17],&rrank[17],&bounds[17],&rlist[18],&rrank[18],&bounds[18],&tlist[17],&tlist[18],COUNTQ);
				delete tlist[9];
				splity(tlist[10],bounds[10],&rlist[19],&rrank[19],&bounds[19],&rlist[20],&rrank[20],&bounds[20],&tlist[19],&tlist[20],COUNTQ);
				delete tlist[10];
				splity(tlist[11],bounds[11],&rlist[21],&rrank[21],&bounds[21],&rlist[22],&rrank[22],&bounds[22],&tlist[21],&tlist[22],COUNTQ);
				delete tlist[11];
				splity(tlist[12],bounds[12],&rlist[23],&rrank[23],&bounds[23],&rlist[24],&rrank[24],&bounds[24],&tlist[23],&tlist[24],COUNTQ);
				delete tlist[12];
				splity(tlist[13],bounds[13],&rlist[25],&rrank[25],&bounds[25],&rlist[26],&rrank[26],&bounds[26],&tlist[25],&tlist[26],COUNTQ);
				delete tlist[13];
				splity(tlist[14],bounds[14],&rlist[27],&rrank[27],&bounds[27],&rlist[28],&rrank[28],&bounds[28],&tlist[27],&tlist[28],COUNTQ);
				delete tlist[14];
				splity(tlist[15],bounds[15],&rlist[29],&rrank[29],&bounds[29],&rlist[30],&rrank[30],&bounds[30],&tlist[29],&tlist[30],COUNTQ);
				delete tlist[15];
				splity(tlist[16],bounds[16],&rlist[31],&rrank[31],&bounds[31],&rlist[32],&rrank[32],&bounds[32],&tlist[31],&tlist[32],COUNTQ);
				delete tlist[16];
   
#endif
///
		//std::copy(points_begin,points_end,ranks->begin());
		std::sort(ranks->begin(),ranks->end(), [] (Point p1,Point p2){return p1.rank<p2.rank;})	;

		for (int idx=0;idx<COUNTQ;++idx)
		{
			fFoint fit;
			if(!nofixed)
			{
				fit.fx = ranks->at(idx).x*MUL;
				fit.fy = ranks->at(idx).y*MUL;
			}
			else
			{
				fit.x = ranks->at(idx).x;
				fit.y = ranks->at(idx).y;
			}
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

		
		//std::stable_sort(points2->begin(),points2->end(), [] (Moint p1,Moint p2){return p1.rank<p2.rank;});	
		std::sort(points2->begin(),points2->end(), [] (Moint p1,Moint p2){return p1.x<p2.x;});


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


#if 0
		  for (std::vector<Moint>::iterator it=points->begin(); it!=points->end(); ++it)
			printf("x=%f,y=%f,rank=%d\n",it->x,it->y,it->rank);
		printf("count=%d, \n",count);
#endif	
		SearchContext *sc=new SearchContext;
		sc->count = count;
		sc->points= (Moint *) &points->begin()[0];
		sc->vpoints=points;

		sc->vpoints2=points2;

		sc->scratch= new std::vector<Moint>();
		sc->scratch->reserve(points_end-points_begin);
		sc->yrlow = yrlow;
		sc->yrhigh = yrhigh;
		sc->xrlow=xrlow;
		sc->xrhigh=xrhigh;
		sc->ranks=ranks;
		sc->foints=foints;
		
		for(int i=0;i<COUNTPLAT;++i)
		{
			sc->rlist[i]=rlist[i];
			sc->rrank[i]=rrank[i];
			sc->bounds[i]=bounds[i];
		}
		sc->nofixed=nofixed;
		//if(!nofixed)
		//	printf("YEAHFIXED");
		return((SearchContext *)sc);
}
//#define COUNTRANKOUT 1
#ifdef COUNTRANKOUT
int countrankout=0;
int zerout=0;
int fullout=0;
int scanout=0;
int noobout=0;
#endif

extern "C" __declspec(dllexport) int32_t search( const SearchContext* sc,  Rect rect, const int32_t count, Point* out_points)
{
	if(!sc)
		return(0);
#ifdef LIES
	if(rect.hx>LIES)
		rect.hx=LIES;
	if(rect.lx<-LIES)
		rect.lx=-LIES;
	if(rect.hy>LIES)
		rect.hy=LIES;
	if(rect.ly<-LIES)
			rect.ly=-LIES;
#endif
	float rhx = rect.hx;
	float rlx = rect.lx;
	float rhy=rect.hy;
	float rly=rect.ly;
	int32_t frhx;
	int32_t frlx;
	int32_t frhy;
	int32_t frly;
#ifdef FIXED32
	if(!sc->nofixed)
	{
		 frhx = rhx*MUL;
		 frlx = rlx*MUL;
		 frhy=rhy*MUL;
		 frly=rly*MUL;
	}

#endif
#ifdef FIXED64
	int frhx = rhx*MUL;
	int frlx = rlx*MUL;
	int frhy=rhy*MUL;
	int frly=rly*MUL;
#endif


#if PROF
StartProfile(PROFILE_GLOBALLEVEL, PROFILE_CURRENTID);
#endif
	
	const std::vector<Point> *ranks= sc->ranks;
#if 1
	if(!sc->nofixed)
	for(int j=COUNTPLAT-1;j>0;j--)
	{
		if(contains(rect,sc->bounds[j]))
		{
			const std::vector<fFoint> *foints= sc->rlist[j];
			const std::vector<int32_t> *rrank = sc->rrank[j];
			unsigned n=distance(sc->rlist[j]->begin(),sc->rlist[j]->end());
			unsigned idx=0;
			for(unsigned i=0;i<n;++i)
			{
				if(fptinrect(foints->at(i),rect))
				{
					int32_t trank = rrank->at(i);
					out_points[idx]=ranks->at(trank);

					++idx;
					if(idx>=count)
					{
#ifdef COUNTRANKOUT
						++scanout;
						if(scanout==7)
						{
							int i=2;
						}
#endif 
						//break;
						
						return(idx);
					}
				}
			}
			goto skipfullplat;
		}
	}
	else //nofixed
	for(int j=COUNTPLAT-1;j>0;j--)
	{
		if(contains(rect,sc->bounds[j]))
		{
			const std::vector<fFoint> *foints= sc->rlist[j];
			const std::vector<int32_t> *rrank = sc->rrank[j];
			unsigned n=distance(sc->rlist[j]->begin(),sc->rlist[j]->end());
			unsigned idx=0;
			for(unsigned i=0;i<n;++i)
			{
				if(ptinrect(foints->at(i),rect))
				{
					int32_t trank = rrank->at(i);
					out_points[idx]=ranks->at(trank);

					++idx;
					if(idx>=count)
					{
#ifdef COUNTRANKOUT
						++scanout;
						if(scanout==7)
						{
							int i=2;
						}
#endif 
						//break;
						
						return(idx);
					}
				}
			}
			goto skipfullplat;
		}
	}
#endif

#if 1
	if(!sc->nofixed)
	{
		const std::vector<fFoint> *foints= sc->foints;
		unsigned idx=0;
		for (unsigned i=0;i<COUNTQ;++i)
		{
			if(fptinrect(foints->at(i),rect))
			{
				out_points[idx]=ranks->at(i);

				++idx;
				if(idx>=count)
				{
	#ifdef COUNTRANKOUT
					++countrankout;
	#endif
					
					return(idx);
				}
			
			}
		}
	}
	else //nofixed
	{
		const std::vector<fFoint> *foints= sc->foints;
		unsigned idx=0;
		for (unsigned i=0;i<COUNTQ;++i)
		{
			if(ptinrect(foints->at(i),rect))
			{
				out_points[idx]=ranks->at(i);

				++idx;
				if(idx>=count)
				{
	#ifdef COUNTRANKOUT
					++countrankout;
	#endif
					
					return(idx);
				}
			
			}
		}
	}
skipfullplat:
#endif
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
		if(rlx > thischunkhigh)
			continue;

		float thischunklow=*xrlow;
		++xrlow;
		if(rhx < thischunklow)
			continue;
		std::vector<Moint>::const_iterator thisbegin2=points2->begin()+(chunk2);
		std::vector<Moint>::const_iterator thisend2;
		if(distance(thisbegin2,points2->end())<chunksize2)
			thisend2=points2->end();
		else
			thisend2=points2->begin()+(chunk2+chunksize2);
			
		std::vector<Moint>::const_iterator low2,high2;
		Moint Foo2,Bar2;
		Foo2.y=rly;
		Bar2.y=rhy;
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
		++yrlow;
		if(rly > thischunkhigh)
			continue;

		float thischunklow=*(yrlow-1);
		
		if(rhy < thischunklow)
			continue;
		std::vector<Moint>::const_iterator thisbegin=points->begin()+(chunk);
		std::vector<Moint>::const_iterator thisend;
		if(distance(thisbegin,points->end())<chunksize)
			thisend=points->end();
		else
			thisend=points->begin()+(chunk+chunksize);
				
		std::vector<Moint>::const_iterator low,high;
		Moint Foo,Bar;
		Foo.x=rlx;
		Bar.x=rhx;
		low=std::lower_bound (thisbegin, thisend, Moint(Foo),[](Moint p1,Moint p2){return p1.x < p2.x;});
		high=std::upper_bound (low, thisend, Moint(Bar),[](Moint p1,Moint p2){return p1.x < p2.x;});
	
		sclow[n]=low;
		schigh[n]=high;
		xblockcount+=distance(low,high);
	}
	}
	if(min(xblockcount,yblockcount)==0)
	{
#ifdef COUNTRANKOUT
		++zerout;
#endif
		return(0);
	}
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
		if(rlx > thischunkhigh)
			continue;

		float thischunklow=*xrlow;
		++xrlow;
		if(rhx < thischunklow)
			continue;

	std::vector<Moint>::const_iterator low2,high2;
	low2=sclow2[n];
	high2=schigh2[n];

	for(;low2!=high2;++low2)
	{	
//++ov;
			if(low2->rank < lrank)
			if((low2->x>=rlx) && (low2->x<=rhx))
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
				
					}
#endif
				}

			}
	}

	}

	}
	else
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
		++yrlow;
		if(rly > thischunkhigh)
			continue;

		float thischunklow=*(yrlow-1);
		
		if(rhy < thischunklow)
			continue;

	std::vector<Moint>::const_iterator low,high;
	low=sclow[n];
	high=schigh[n];
	for(;low!=high;++low)
	{	
//++ov;		
			if(low->rank < lrank)
			if((low->y>=rly) && (low->y<=rhy))
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

#endif
				}
			}
	}

	}

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
#ifdef COUNTRANKOUT
	++fullout;
#endif
#if PROF
	StopProfile(PROFILE_GLOBALLEVEL, PROFILE_CURRENTID);
#endif
	return(min(iv,count));
}

extern "C" __declspec(dllexport) SearchContext *destroy(SearchContext *sc)
{
	if(sc)
	{
#ifdef COUNTRANKOUT
		printf("\n----------scanout=%d,noobout=%d,countrankout=%d,zerout=%d,fullout=%d\n",scanout,noobout,countrankout,zerout,fullout);
#endif
		//if(sc->points)
		//	free(sc->points);
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