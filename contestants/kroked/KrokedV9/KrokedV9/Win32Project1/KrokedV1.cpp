// KrokedV1.cpp 
//
//        point_search reference.dll acme.dll -p10000000 -q100000 -r20 -s642E3E98-6EA650C8-642E3E98-7E75161E-4FE6D148

#include "stdafx.h"

#include <stdio.h>
#include <stdlib.h>
#include <algorithm>
#include <vector>
using namespace std;
#define at(x) operator[](x)

#define CCHUNK 16
#define CCHUNK2 16
#define MAXPLAT 64
#define COUNTPLAT (MAXPLAT*2+1)


#define ptinrect(PT,RECT) ((PT.x>=rlx) & (PT.x<=rhx) && (PT.y>=rly) & (PT.y<=rhy))
#define fptinrect(PT,RECT) ((PT.fx>=frlx) & (PT.fx<=frhx) && (PT.fy>=frly) & (PT.fy<=frhy))
#include "point_search.h"

#define PROF 0
#if PROF
#include "C:\Program Files (x86)\Microsoft Visual Studio 11.0\Team Tools\Performance Tools\x64\PerfSDK\VSPerf.h"
#endif

#define COUNTQ 185000
#define QCLIP 5000
#define COUNTX 4000

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
	if(left->size()>0)
		boundsleft->hx=left->back().x;
	if(right->size()>0)
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

		it.x=left->at(i).x;
		it.y=left->at(i).y;
			
		leftout->emplace_back(it);
		leftrankout->emplace_back(left->at(i).rank);
	}
	for(int i=0;i<right->size();++i)
	{
		fFoint it;
		
		it.x=right->at(i).x;
		it.y=right->at(i).y;
	
		
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
	if(bottom->size()>0)
		boundsbottom->hy=bottom->back().y;
    if(top->size()>0)
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
		
		it.x=bottom->at(i).x;
		it.y=bottom->at(i).y;
		
		bottomout->emplace_back(it);
		bottomrankout->emplace_back(bottom->at(i).rank);
	}
	for(int i=0;i<top->size();++i)
	{
		fFoint it;
		
		it.x=top->at(i).x;
		it.y=top->at(i).y;

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

#define contains(r1,r2) ((r1.lx>r2.lx) & (r1.hx<r2.hx) && (r1.ly>r2.ly) & (r1.hy<r2.hy))

extern "C" __declspec(dllexport) SearchContext* __stdcall create(const Point* points_begin, const Point* points_end)
{
		
		if(!points_begin)
			return(NULL);
		
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
			
			it.x=points_begin[idx].x;
			it.y=points_begin[idx].y;

			it.rank=points_begin[idx].rank;
			points->emplace_back(it);
			points2->emplace_back(it);
			ranks->emplace_back(points_begin[idx]);
		}
		

	std::vector<Moint> *tlist[COUNTPLAT];
	std::vector<fFoint> *rlist[COUNTPLAT];
	std::vector<int32_t> *rrank[COUNTPLAT];


	static Rect bounds[COUNTPLAT];
	Rect outterbounds;
	outterbounds.lx=-10000000001.0;
	outterbounds.ly=-10000000001.0;
	outterbounds.hx= 10000000001.0;
	outterbounds.hy= 10000000001.0;
	splity(points,outterbounds,&rlist[1],&rrank[1],&bounds[1],&rlist[2],&rrank[2],&bounds[2],&tlist[1],&tlist[2],COUNTQ);
	delete tlist[1];delete tlist[2];
	splitx(points,outterbounds,&rlist[3],&rrank[3],&bounds[3],&rlist[4],&rrank[4],&bounds[4],&tlist[3],&tlist[4],COUNTQ);

	
	for(int ii=3;ii,ii<MAXPLAT;)
	{
		int tc=ii-1;
		for(int jj=0;jj<tc;++jj)
		{
			
			splity(tlist[ii],bounds[ii],&rlist[ii*2-1],&rrank[ii*2-1],&bounds[ii*2-1],&rlist[ii*2],&rrank[ii*2],&bounds[ii*2],&tlist[ii*2-1],&tlist[ii*2],COUNTQ);
			delete(tlist[ii]);
			
			++ii;
		}
		if(ii>=MAXPLAT)
			break;
	    tc=ii-1;
		for(int jj=0;jj<tc;++jj)
		{
			
				splitx(tlist[ii],bounds[ii],&rlist[ii*2-1],&rrank[ii*2-1],&bounds[ii*2-1],&rlist[ii*2],&rrank[ii*2],&bounds[ii*2],&tlist[ii*2-1],&tlist[ii*2],COUNTQ);
				delete(tlist[ii]);
			
	
			++ii;
		}
		
	}
	for(int i=1;i<COUNTPLAT;i++)
	{
		fFoint mt;
		mt.x=0;
		mt.y=0;
		uint32_t oldsize=rlist[i]->size();
		rlist[i]->resize(oldsize+33,mt);
		rlist[i]->resize(oldsize);
		rlist[i]->reserve(oldsize+33);
	}
		
		std::sort(ranks->begin(),ranks->end(), [] (Point p1,Point p2){return p1.rank<p2.rank;})	;

		
		for (int idx=0;idx<COUNTQ;++idx)
		{
			fFoint fit;
			fit.x = ranks->at(idx).x;
			fit.y = ranks->at(idx).y;
			
			foints->emplace_back(fit);
		}
		fFoint mt;
		mt.x=0;
		mt.y=0;
		uint32_t oldsize=foints->size();
		foints->resize(oldsize+33,mt);
		foints->resize(oldsize);
		foints->reserve(oldsize+33);
		int64_t count=points_end-points_begin;

		
		std::sort(points->begin(),points->end(), [] (Moint p1,Moint p2){return p1.y<p2.y;})	;
			

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
		
		sc->pc=GetPriorityClass(GetCurrentProcess());
		SetPriorityClass(GetCurrentProcess(),REALTIME_PRIORITY_CLASS);
		return((SearchContext *)sc);
}
typedef struct ab
{
	int64_t a[4];
};
	typedef struct{
		union{
			 ab ab;
			 int8_t o[32];
		};
	}xmmb;
	
extern "C" __declspec(dllexport) int32_t search( const SearchContext* sc,  Rect rect, const int32_t count, Point* out_points)
{
#if PROF	
StartProfile(PROFILE_GLOBALLEVEL, PROFILE_CURRENTID);
#endif
	if(!sc)
		return(0);

	float rlx = rect.lx;
	float rhx = rect.hx;
	float rly=rect.ly;
	float rhy=rect.hy;
	
	const std::vector<Point> *ranks= sc->ranks;
	int j=0;
	
	if(rect.hx<sc->bounds[4].lx)
	{
		j=3;
		if(rect.hy<sc->bounds[6].ly)
		{
			j=5;
			if(rect.hx<sc->bounds[10].lx)
			{
				j=9;
				if(rect.hy<sc->bounds[18].ly)
				{
					j=17;
					if(rect.hx<sc->bounds[34].lx)
					{
						j=33;
						if(rect.hy<sc->bounds[66].ly)
						{
							j=65;
						}
						else
						if(rect.ly>sc->bounds[65].hy)
						{
							j=66;
						}

					}
					else
					if(rect.lx>sc->bounds[33].hx)
					{
						j=34;
						if(rect.hy<sc->bounds[68].ly)
						{
							j=67;
						}
						else
						if(rect.ly>sc->bounds[67].hy)
						{
							j=68;
						}
					}

				}
				else
				if(rect.ly>sc->bounds[17].hy)
				{
					j=18;
					if(rect.hx<sc->bounds[36].lx)
					{
						j=35;
						if(rect.hy<sc->bounds[70].ly)
						{
							j=69;
						}
						else
						if(rect.ly>sc->bounds[69].hy)
						{
							j=70;
						}
					}
					else
					if(rect.lx>sc->bounds[35].hx)
					{
						j=36;
						if(rect.hy<sc->bounds[72].ly)
						{
							j=71;
						}
						else
						if(rect.ly>sc->bounds[71].hy)
						{
							j=72;
						}
					}
				}
			}
			else
			if(rect.lx>sc->bounds[9].hx)
			{
				j=10;
				if(rect.hy<sc->bounds[20].ly)
				{
					j=19;
					if(rect.hx<sc->bounds[38].lx)
					{
						j=37;
						if(rect.hy<sc->bounds[74].ly)
						{
							j=73;
						}
						else
						if(rect.ly>sc->bounds[73].hy)
						{
							j=74;
						}
					}
					else
					if(rect.lx>sc->bounds[37].hx)
					{
						j=38;
						if(rect.hy<sc->bounds[76].ly)
						{
							j=75;
						}
						else
						if(rect.ly>sc->bounds[75].hy)
						{
							j=76;
						}
					}
				}
				else
				if(rect.ly>sc->bounds[19].hy)
				{
					j=20;
					if(rect.hx<sc->bounds[40].lx)
					{
						j=39;
						if(rect.hy<sc->bounds[78].ly)
						{
							j=77;
						}
						else
						if(rect.ly>sc->bounds[77].hy)
						{
							j=78;
						}
					}
					else
					if(rect.lx>sc->bounds[39].hx)
					{
						j=40;
						if(rect.hy<sc->bounds[80].ly)
						{
							j=79;
						}
						else
						if(rect.ly>sc->bounds[79].hy)
						{
							j=80;
						}
					}
				}
			}
		}
		else
		if(rect.ly>sc->bounds[5].hy)
		{
			j=6;
			if(rect.hx<sc->bounds[12].lx)
			{
				j=11;
				if(rect.hy<sc->bounds[22].ly)
				{
					j=21;
					if(rect.hx<sc->bounds[42].lx)
					{
						j=41;
						if(rect.hy<sc->bounds[82].ly)
						{
							j=81;
						}
						else
						if(rect.ly>sc->bounds[81].hy)
						{
							j=82;
						}
					}
					else
					if(rect.lx>sc->bounds[41].hx)
					{
						j=42;
						if(rect.hy<sc->bounds[84].ly)
						{
							j=83;
						}
						else
						if(rect.ly>sc->bounds[83].hy)
						{
							j=84;
						}
					}
				}
				else
				if(rect.ly>sc->bounds[21].hy)
				{
					j=22;
					if(rect.hx<sc->bounds[44].lx)
					{
						j=43;
						if(rect.hy<sc->bounds[86].ly)
						{
							j=85;
						}
						else
						if(rect.ly>sc->bounds[85].hy)
						{
							j=86;
						}
					}
					else
					if(rect.lx>sc->bounds[43].hx)
					{
						j=44;
						if(rect.hy<sc->bounds[88].ly)
						{
							j=87;
						}
						else
						if(rect.ly>sc->bounds[87].hy)
						{
							j=88;
						}
					}
				}
			}
			else
			if(rect.lx>sc->bounds[11].hx)
			{
				j=12;
				if(rect.hy<sc->bounds[24].ly)
				{
					j=23;
					if(rect.hx<sc->bounds[46].lx)
					{
						j=45;
						if(rect.hy<sc->bounds[90].ly)
						{
							j=89;
						}
						else
						if(rect.ly>sc->bounds[89].hy)
						{
							j=90;
						}
					}
					else
					if(rect.lx>sc->bounds[45].hx)
					{
						j=46;
						if(rect.hy<sc->bounds[92].ly)
						{
							j=91;
						}
						else
						if(rect.ly>sc->bounds[91].hy)
						{
							j=92;
						}
					}
				}
				else
				if(rect.ly>sc->bounds[23].hy)
				{
					j=24;
					if(rect.hx<sc->bounds[48].lx)
					{
						j=47;
						if(rect.hy<sc->bounds[94].ly)
						{
							j=93;
						}
						else
						if(rect.ly>sc->bounds[93].hy)
						{
							j=94;
						}
					}
					else
					if(rect.lx>sc->bounds[47].hx)
					{
						j=48;
						if(rect.hy<sc->bounds[96].ly)
						{
							j=95;
						}
						else
						if(rect.ly>sc->bounds[95].hy)
						{
							j=96;
						}
					}
				}
			}
		}
	}
	else
	if(rect.lx>sc->bounds[3].hx)
	{
		j=4;
		if(rect.hy<sc->bounds[8].ly)
		{
			j=7;
			if(rect.hx<sc->bounds[14].lx)
			{
				j=13;
				if(rect.hy<sc->bounds[26].ly)
				{
					j=25;
					if(rect.hx<sc->bounds[50].lx)
					{
						j=49;
						if(rect.hy<sc->bounds[98].ly)
						{
							j=97;
						}
						else
						if(rect.ly>sc->bounds[97].hy)
						{
							j=98;
						}
					}
					else
					if(rect.lx>sc->bounds[49].hx)
					{
						j=50;
						if(rect.hy<sc->bounds[100].ly)
						{
							j=99;
						}
						else
						if(rect.ly>sc->bounds[99].hy)
						{
							j=100;
						}
					}
				}
				else
				if(rect.ly>sc->bounds[25].hy)
				{
					j=26;
					if(rect.hx<sc->bounds[52].lx)
					{
						j=51;
						if(rect.hy<sc->bounds[102].ly)
						{
							j=101;
						}
						else
						if(rect.ly>sc->bounds[101].hy)
						{
							j=102;
						}
					}
					else
					if(rect.lx>sc->bounds[51].hx)
					{
						j=52;
						if(rect.hy<sc->bounds[104].ly)
						{
							j=103;
						}
						else
						if(rect.ly>sc->bounds[103].hy)
						{
							j=104;
						}
					}
				}
			}
			else
			if(rect.lx>sc->bounds[13].hx)
			{
				j=14;
				if(rect.hy<sc->bounds[28].ly)
				{
					j=27;
					if(rect.hx<sc->bounds[54].lx)
					{
						j=53;
						if(rect.hy<sc->bounds[106].ly)
						{
							j=105;
						}
						else
						if(rect.ly>sc->bounds[105].hy)
						{
							j=106;
						}
					}
					else
					if(rect.lx>sc->bounds[53].hx)
					{
						j=54;
						if(rect.hy<sc->bounds[108].ly)
						{
							j=107;
						}
						else
						if(rect.ly>sc->bounds[107].hy)
						{
							j=108;
						}
					}
				}
				else
				if(rect.ly>sc->bounds[27].hy)
				{
					j=28;
					if(rect.hx<sc->bounds[56].lx)
					{
						j=55;
						if(rect.hy<sc->bounds[110].ly)
						{
							j=109;
						}
						else
						if(rect.ly>sc->bounds[109].hy)
						{
							j=110;
						}
					}
					else
					if(rect.lx>sc->bounds[55].hx)
					{
						j=56;
						if(rect.hy<sc->bounds[112].ly)
						{
							j=111;
						}
						else
						if(rect.ly>sc->bounds[111].hy)
						{
							j=112;
						}
					}
				}
			}
		}
		else
		if(rect.ly>sc->bounds[7].hy)
		{
			j=8;
			if(rect.hx<sc->bounds[16].lx)
			{
				j=15;
				if(rect.hy<sc->bounds[30].ly)
				{
					j=29;
					if(rect.hx<sc->bounds[58].lx)
					{
						j=57;
						if(rect.hy<sc->bounds[114].ly)
						{
							j=113;
						}
						else
						if(rect.ly>sc->bounds[113].hy)
						{
							j=114;
						}
					}
					else
					if(rect.lx>sc->bounds[57].hx)
					{
						j=58;
						if(rect.hy<sc->bounds[116].ly)
						{
							j=115;
						}
						else
						if(rect.ly>sc->bounds[115].hy)
						{
							j=116;
						}
					}
				}
				else
				if(rect.ly>sc->bounds[29].hy)
				{
					j=30;
					if(rect.hx<sc->bounds[60].lx)
					{
						j=59;
						if(rect.hy<sc->bounds[118].ly)
						{
							j=117;
						}
						else
						if(rect.ly>sc->bounds[117].hy)
						{
							j=118;
						}
					}
					else
					if(rect.lx>sc->bounds[59].hx)
					{
						j=60;
						if(rect.hy<sc->bounds[120].ly)
						{
							j=119;
						}
						else
						if(rect.ly>sc->bounds[119].hy)
						{
							j=120;
						}
					}
				}
			}
			else
			if(rect.lx>sc->bounds[15].hx)
			{
				j=16;
				if(rect.hy<sc->bounds[32].ly)
				{
					j=31;
					if(rect.hx<sc->bounds[62].lx)
					{
						j=61;
						if(rect.hy<sc->bounds[122].ly)
						{
							j=121;
						}
						else
						if(rect.ly>sc->bounds[121].hy)
						{
							j=122;
						}
					}
					else
					if(rect.lx>sc->bounds[61].hx)
					{
						j=62;
						if(rect.hy<sc->bounds[124].ly)
						{
							j=123;
						}
						else
						if(rect.ly>sc->bounds[123].hy)
						{
							j=124;
						}
					}
				}
				else
				if(rect.ly>sc->bounds[31].hy)
				{
					j=32;
					if(rect.hx<sc->bounds[64].lx)
					{
						j=63;
						if(rect.hy<sc->bounds[126].ly)
						{
							j=125;
						}
						else
						if(rect.ly>sc->bounds[125].hy)
						{
							j=126;
						}
					}
					else
					if(rect.lx>sc->bounds[63].hx)
					{
						j=64;
						if(rect.hy<sc->bounds[128].ly)
						{
							j=127;
						}
						else
						if(rect.ly>sc->bounds[127].hy)
						{
							j=128;
						}
					}
				}
			}
		}
	}
	else
	if(rect.hy<sc->bounds[2].ly)
	{
		j=1;
	}
	else
	if(rect.ly>sc->bounds[1].hy)
	{
		j=2;
	}
	unsigned idx=0;
	unsigned secondpass=0;
	unsigned startpass=0;
	
dopass:
	if(j)
	{		
		{
			const std::vector<fFoint> *foints= sc->rlist[j];
			const std::vector<int32_t> *rrank = sc->rrank[j];
			unsigned n=foints->size();

			if(!secondpass)
				n=min(n,QCLIP);
		__declspec(align(16)) xmmb o;
		for(unsigned i=startpass;i<n;i+=32)
		{
			for(unsigned a=0;a<32;++a)
			{
				o.o[a]=(foints->at(i+a).x>=rlx)&(foints->at(i+a).x<=rhx)&(foints->at(i+a).y>=rly)&(foints->at(i+a).y<=rhy);
			}
			//if(o.ab.a[0]||o.ab.a[1]||o.ab.a[2]||o.ab.a[3])
			//for(unsigned a=0;a<32;++a)
			for(unsigned kk=0;kk<4;++kk)
			if(o.ab.a[kk])
			for(unsigned a=kk<<3;a<(kk+1)<<3;++a)
			{
				if(o.o[a]) 
				{
					if(i+a<n)
					{
						int32_t trank = rrank->at(i+a);
						//out_points[idx]=ranks->at(trank);
						*(uint64_t *)&out_points[idx]=*(uint64_t *)&(ranks->at(trank)); 
						++idx;
						if(idx>=count)
						{
#if PROF
	StopProfile(PROFILE_GLOBALLEVEL, PROFILE_CURRENTID);
#endif	
							return(count);
						}
					}
				}
			}		
		}
			goto skipfullplat;
		}
	}
	
	{
		
		const std::vector<fFoint> *foints= sc->foints;

	    unsigned n=foints->size();
		if(!secondpass)
				n=min(n,QCLIP);

		__declspec(align(32)) xmmb o;
		for( unsigned i=startpass;i<n;i+=32)
		{

			for(unsigned a=0;a<32;++a)
			{
				o.o[a]=(foints->at(i+a).x>=rlx)&(foints->at(i+a).x<=rhx)&(foints->at(i+a).y>=rly)&(foints->at(i+a).y<=rhy);
			}			
			for(unsigned kk=0;kk<4;++kk)
			if(o.ab.a[kk])
			for(unsigned a=kk<<3;a<(kk+1)<<3;++a)
			{
				if(o.o[a]) 
				{
					if(i+a<n)
					{
						*(uint64_t *)&out_points[idx]=*(uint64_t *)&(ranks->at(i+a));
						++idx;
						if(idx>=count)
						{
#if PROF
	StopProfile(PROFILE_GLOBALLEVEL, PROFILE_CURRENTID);
#endif	
							return(count);
						}
					}
				}
			}
			
		}
			
	}

skipfullplat:
	if(secondpass)
		goto skipahead;
	const std::vector<Moint> *points = sc->vpoints;
	const std::vector<Moint> *points2 = sc->vpoints2;
	std::vector<Moint> *vlist = sc->scratch;
	
	std::vector<Moint>::const_iterator sclow[CCHUNK+1];
	std::vector<Moint>::const_iterator schigh[CCHUNK+1];
	std::vector<Moint>::const_iterator sclow2[CCHUNK2+1];
	std::vector<Moint>::const_iterator schigh2[CCHUNK2+1];

	
	int32_t lrank=MAXINT32;
	int iv=0;
	int64_t yblockcount=0;
	int64_t xblockcount=0;
	

	{
	size_t chunk2;
	size_t chunksize2;
	std::vector<float>::const_iterator xrlow=sc->xrlow.begin();
	std::vector<float>::const_iterator xrhigh=sc->xrhigh.begin();

	chunksize2=CCHUNK2;
	if(points2->size() > CCHUNK2*10)
		chunksize2 = points2->size()/CCHUNK2;

	unsigned k=points2->size();
	for(int n=0,chunk2=0;chunk2<k;chunk2+=chunksize2,n++)
	{
		float thischunkhigh=*xrhigh;
		++xrhigh;
		++xrlow;
		if(rlx > thischunkhigh)
			continue;

		float thischunklow=*(xrlow-1);
		
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

	unsigned k=points->size();
	for(int n=0,chunk=0;chunk<k;chunk+=chunksize,n++)
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
#if PROF
	StopProfile(PROFILE_GLOBALLEVEL, PROFILE_CURRENTID);
#endif
		return(0);
	}
if(!secondpass)
{
	secondpass=1;
	startpass=QCLIP;
	if(min(xblockcount,yblockcount)>((COUNTX)))
	{
		goto dopass;
	}
	else
	{

	}
}
skipahead:
	
	if(yblockcount<xblockcount)
{

	size_t chunk2;
	size_t chunksize2;
	std::vector<float>::const_iterator xrlow=sc->xrlow.begin();
	std::vector<float>::const_iterator xrhigh=sc->xrhigh.begin();

	chunksize2=CCHUNK2;
	if(points2->size() > CCHUNK2*10)
		chunksize2 = points2->size()/CCHUNK2;

	unsigned k=points2->size();
	for(int n=0,chunk2=0;chunk2<k;chunk2+=chunksize2,n++)
	{
		float thischunkhigh=*xrhigh;
		++xrhigh;
		++xrlow;
		if(rlx > thischunkhigh)
			continue;

		float thischunklow=*(xrlow-1);
		
		if(rhx < thischunklow)
			continue;

	std::vector<Moint>::const_iterator low2,high2;

	low2=sclow2[n];
	high2=schigh2[n];

	for(;low2!=high2;++low2)
	{	
			if(low2->rank < lrank)
			if((low2->x>=rlx) && (low2->x<=rhx))
			{

				vlist->emplace_back(*low2);
				++iv;

#define MYMASK 0x3F
				if((iv&MYMASK)==0)
				{
					std::partial_sort(vlist->begin(),vlist->begin()+min(iv,count),vlist->end(),[] (Moint p1,Moint p2){return p1.rank<p2.rank;});
					vlist->resize(count);
					lrank=vlist->at(count-1).rank;
				
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
	unsigned k=points->size();
	for(int n=0,chunk=0;chunk<k;chunk+=chunksize,n++)
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
			if(low->rank < lrank)
			if((low->y>=rly) && (low->y<=rhy))
			{
				
				vlist->emplace_back(*low);
				++iv;

				if((iv&MYMASK)==0)
				{
					std::partial_sort(vlist->begin(),vlist->begin()+min(iv,count),vlist->end(),[] (Moint p1,Moint p2){return p1.rank<p2.rank;});
					vlist->resize(count);
					lrank=vlist->at(count-1).rank;
				
				}
				
			}
	}

	}

	}
	unsigned cv=min(iv,count);
	if(iv)
	{
		std::partial_sort(vlist->begin(),vlist->begin()+cv,vlist->end(),[] (Moint p1,Moint p2){return p1.rank<p2.rank;});
		
		for (int idx=0;idx<cv;idx++)
		{
			    int32_t rank=vlist->at(idx).rank;
				*(uint64_t *)&out_points[idx]=*(uint64_t *)&(ranks->at(rank));
				//out_points[idx].id=ranks->at(rank).id;
				//out_points[idx].x=vlist->at(idx).x;
				//out_points[idx].y=vlist->at(idx).y;
				//out_points[idx].rank=vlist->at(idx).rank;
		}
							
		vlist->clear();
	}

#if PROF
	StopProfile(PROFILE_GLOBALLEVEL, PROFILE_CURRENTID);
#endif
	return(cv);
}

extern "C" __declspec(dllexport) SearchContext *destroy(SearchContext *sc)
{
	if(sc)
	{
		SetPriorityClass(GetCurrentProcess(),sc->pc);
	
		if(sc->scratch)
		delete sc;
	}
	
	return (NULL);
}