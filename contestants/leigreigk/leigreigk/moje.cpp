#include "./moje.h"
#include <omp.h>
#include <string.h>

#define extreme
#define memCP

//typdef float Tfor;
Point *point_table;
Point *extreme_table;
Point *output_table;
Point *output_sort_table;
int output_sort_len;
const bool debug=0;
int size=0;
int size_extreme=0;

//UWAGA

struct {
bool operator()(Point a, Point b)
{
	return a.x < b.x;
}
}CustomLessX;

struct {
bool operator()(Point a, Point b)
{
	return a.y < b.y;
}
}CustomLessY;

struct {
bool operator()(Point a, Point b)
{
	return a.rank < b.rank;
}
}CustomLessR;

struct TreeNode{
Point *table;
Point rankedP[20];
int len;
float down;
float mid;
float up;
bool leaf;
TreeNode *TLow;
TreeNode *THigh;
bool XY;//split X=0; split Y =1
};

void Create_XY_Tree(TreeNode *Node,Point *data,int lenght,bool XY){//UWAGA DANE MUSZE BYC POSORTOWANE

Node->table=data;
Node->len=lenght;
Node->XY=XY;
std::partial_sort(data,&(data[std::min(lenght,20)]),&(data[lenght]),CustomLessR);


for(int i=0;i<std::min(lenght,20);i++){
	Node->rankedP[i]=data[i];
	}

if(lenght<=20){
    Node->leaf=1;
}
else{
    if(XY==0){
        std::sort(data,&(data[lenght]),CustomLessX);
        Node->down=Node->table[0].x;
        Node->mid=Node->table[(lenght+1)/2].x;
        Node->up=Node->table[lenght-1].x;
        }

    if(XY==1){
    std::sort(data,&(data[lenght]),CustomLessY);
        Node->down=Node->table[0].y;
        Node->mid=Node->table[(lenght+1)/2].y;
        Node->up=Node->table[lenght-1].y;
        }

    Node->leaf=0;
    Node->TLow=new TreeNode();
    Node->THigh=new TreeNode();
    Create_XY_Tree(Node->TLow,Node->table,(lenght+1)/2,XY^1);
    Create_XY_Tree(Node->THigh,&(Node->table[(lenght+1)/2] ),lenght/2,XY^1);
    }
}

TreeNode *Node0;
int iterations=0;
SearchContext* __stdcall DLL_EXP create(const Point* points_begin, const Point* points_end){
omp_set_num_threads(4);

point_table=new Point[10000000];

extreme_table=new Point[4];
output_table=new Point[20];

while(points_begin!=points_end){

//Possible gain from excluding extreme points from tree - unkown
	#ifdef extreme
	if( std::fabs(points_begin->x)==1e10||std::fabs(points_begin->y)==1e10){
		extreme_table[size_extreme]=*points_begin;
		points_begin++;
		size_extreme++;
	}
	#endif// extreme
	else{
		point_table[size]=*points_begin;
		points_begin++;
		size++;
	#ifdef extreme
	}
	#endif// extreme

	//	min_x=	points_begin->x;

}

//std::cout<<"size: "<<size<<' '<<point_table[0].x<<std::endl<<std::endl;

//std::sort(point_table,&(point_table[size]),CustomLessX);

//for(int i=0;i<10;i++)std::cout<<point_table[i].x<<' '<<point_table[i].y<<' '<<point_table[i].rank<<' '<<std::endl;fflush(stdout);

Node0=new TreeNode();
//Create_XY_Tree(Node0,point_table,size,0);

{
TreeNode *Node=Node0;
Point *data=point_table;
int lenght=size;
bool XY=0;

Node->table=data;
Node->len=lenght;
Node->XY=XY;
std::partial_sort(data,&(data[std::min(lenght,20)]),&(data[lenght]),CustomLessR);


for(int i=0;i<std::min(lenght,20);i++){
	Node->rankedP[i]=data[i];
	}

if(lenght<=20){
    Node->leaf=1;
}
else{
    if(XY==0){
        std::sort(data,&(data[lenght]),CustomLessX);
        Node->down=Node->table[0].x;
        Node->mid=Node->table[(lenght+1)/2].x;
        Node->up=Node->table[lenght-1].x;
        }

    if(XY==1){
    std::sort(data,&(data[lenght]),CustomLessY);
        Node->down=Node->table[0].y;
        Node->mid=Node->table[(lenght+1)/2].y;
        Node->up=Node->table[lenght-1].y;
        }

    Node->leaf=0;
    Node->TLow=new TreeNode();
    Node->THigh=new TreeNode();
    #pragma omp parallel
        {
        if(omp_get_thread_num()==0)Create_XY_Tree(Node->TLow,Node->table,(lenght+1)/2,XY^1);
        if(omp_get_thread_num()==1)Create_XY_Tree(Node->THigh,&(Node->table[(lenght+1)/2] ),lenght/2,XY^1);
        }
    }
}









if(debug){
	std::cout<<"debug Create"<<std::endl;
	for(int i=0;i<size;i++){
		std::cout<<point_table[i].rank<<' '<<point_table[i].x<<' '<<point_table[i].y<<std::endl;
	}//<<' '<<point_table[i].id
}//std::cout<<"DUPA\n";
delete[] point_table;
output_sort_table=new Point[5000000];
return nullptr;
}

struct Rect_OK{
bool lx,ly,hx,hy;
};


int Search2D(TreeNode & Node,Rect rect,Rect_OK rect_OK/*,Point* out_points*/){
if(size==0)return 0;
//iterations++;


if(rect_OK.lx==1 && rect_OK.hx==1 && rect_OK.ly==1 && rect_OK.hy==1  ){
#ifdef memCP
    memcpy ( &(output_sort_table[output_sort_len]), Node.rankedP, sizeof(Point)*std::min(20,Node.len) );
#else
    Point *to=&(output_sort_table[output_sort_len]),
    *from=Node.rankedP;
    *to++=*from++;
    *to++=*from++;
    *to++=*from++;
    *to++=*from++;
    *to++=*from++;
    *to++=*from++;
    *to++=*from++;
    *to++=*from++;
    *to++=*from++;
    *to++=*from++;
    *to++=*from++;
    *to++=*from++;
    *to++=*from++;
    *to++=*from++;
    *to++=*from++;
    *to++=*from++;
    *to++=*from++;
    *to++=*from++;
    *to++=*from++;
    *to++=*from++;
#endif
    output_sort_len+=std::min(20,Node.len);

/*    for(int i=0;i<std::min(20,Node.len);i++){

        output_sort_table[output_sort_len]=Node.rankedP[i];
        output_sort_len++;
	}*/
    return 0;
}

if(Node.leaf){
    for(int i=0;i<std::min(20,Node.len);i++)
        if( Node.rankedP[i].x>=rect.lx && Node.rankedP[i].x<=rect.hx && \
        Node.rankedP[i].y<=rect.hy && Node.rankedP[i].y>=rect.ly){
            output_sort_table[output_sort_len]=Node.rankedP[i];
            output_sort_len++;
        }
    return 0;
}

Rect_OK rect_OK_down=rect_OK;
Rect_OK rect_OK_up=rect_OK;
/*
Point out_down[20];
int count_down=0;
Point out_up[20];
int count_up=0;
*/
if(Node.XY==0){


//lower half
if( Node.down>=rect.lx  && Node.down<=rect.hx)rect_OK_down.lx=1;else
rect_OK_down.lx=0;
if(Node.mid<=rect.hx && Node.mid>=rect.lx)rect_OK_down.hx=1;
else
rect_OK_down.hx=0;
if(rect_OK_down.lx||rect_OK_down.hx){
	Search2D(*(Node.TLow),rect,rect_OK_down);
}
else if(Node.down<rect.lx && Node.mid>rect.hx){
	Search2D(*(Node.TLow),rect,rect_OK_down);
}
//upper half
if(Node.mid>=rect.lx && Node.mid<=rect.hx)rect_OK_up.lx=1;
else rect_OK_up.lx=0;
if(Node.up<=rect.hx && Node.up>=rect.lx)rect_OK_up.hx=1;
else rect_OK_up.hx=0;
if(rect_OK_up.lx||rect_OK_up.hx){
	Search2D(*(Node.THigh),rect,rect_OK_up);
}

if(Node.mid<rect.lx && Node.up>rect.hx){
	Search2D(*(Node.THigh),rect,rect_OK_up);
}
}


if(Node.XY==1){
//lower half
if(Node.down>=rect.ly  && Node.down<=rect.hy)rect_OK_down.ly=1;else rect_OK_down.ly=0;
if(Node.mid<=rect.hy  && Node.mid>=rect.ly)rect_OK_down.hy=1;
else rect_OK_down.hy=0;
if(rect_OK_down.ly||rect_OK_down.hy){
	Search2D(*(Node.TLow),rect,rect_OK_down);
}
else if(Node.down<rect.ly && Node.mid>rect.hy){
	Search2D(*(Node.TLow),rect,rect_OK_down);
}
//upper half
if(Node.mid>=rect.ly && Node.mid<=rect.hy)rect_OK_up.ly=1;
else rect_OK_up.ly=0;
if(Node.up<=rect.hy && Node.up>=rect.ly)rect_OK_up.hy=1;
else rect_OK_up.hy=0;

if(rect_OK_up.ly||rect_OK_up.hy){
	Search2D(*(Node.THigh),rect,rect_OK_up);
}
else if(Node.mid<rect.ly && Node.up>rect.hy){
	Search2D(*(Node.THigh),rect,rect_OK_up);
}
}

//Scal - Merge Sort
/*
int it_down=0;
int it_up=0;
for(int i=0;i<std::min(20,count_up+count_down);i++){
    if( (out_down[it_down].rank<out_up[it_up].rank&&it_down<count_down)||(it_up==count_up)  ){
        out_points[i]=out_down[it_down];
        it_down++;
        }
    else{
        out_points[i]=out_up[it_up];
        it_up++;
    }

}
return std::min(20,count_up+count_down);
*/
return 0;
}




int32_t __stdcall DLL_EXP search(SearchContext* sc, const Rect rect, const int32_t count, Point* out_points){
//int32_t num=0;
//iterations=0;
Rect_OK check_bounds;
check_bounds.lx=0;
check_bounds.ly=0;
check_bounds.hx=0;
check_bounds.hy=0;


output_sort_len=0;

int ret= Search2D(*Node0,rect,check_bounds/*,out_points*/);

//output_sort_table
if(debug){
	std::cout<<std::endl<<"OUT:\n"<<ret<<std::endl;
	for(int i=0;i<ret;i++)std::cout<<out_points[i].rank<<' '<<out_points[i].x<<' '<<out_points[i].y<<' '<<std::endl;
	std::cout<<iterations<<' '<<rect.lx<<' '<<rect.ly<<' '<<rect.hy<<' '<<rect.hy<<std::endl;
}

#ifdef extreme
for(int i=0;i<size_extreme;i++){
    if(extreme_table[i].x>=rect.lx && extreme_table[i].x<=rect.hx && extreme_table[i].y>=rect.ly && extreme_table[i].y<=rect.hy){
        output_sort_table[output_sort_len]=extreme_table[i];
        output_sort_len++;
    }
}
#endif //extreme
std::partial_sort(output_sort_table,&(output_sort_table[std::min(output_sort_len,20)]),&(output_sort_table[output_sort_len]),CustomLessR);
for(int i=0;i<std::min(output_sort_len,20);i++)out_points[i]=output_sort_table[i];
return std::min(output_sort_len,20);
//SEARCH2D - zastapione


/*{//SEARCH2D
//int Search2D(TreeNode & Node,Rect rect,Rect_OK rect_OK,Point* out_points)

TreeNode Node=*Node0;
Rect_OK rect_OK=check_bounds;

if(size==0)return 0;
iterations++;



if(rect_OK.lx==1 && rect_OK.hx==1 && rect_OK.ly==1 && rect_OK.hy==1  ){
    for(int i=0;i<std::min(20,Node.len);i++){
        out_points[i]=Node.rankedP[i];
	}
    return std::min(20,Node.len);
}

if(Node.leaf){
    int point_inside=0;
    for(int i=0;i<std::min(20,Node.len);i++)
        if( Node.rankedP[i].x>=rect.lx && Node.rankedP[i].x<=rect.hx && \
        Node.rankedP[i].y<=rect.hy && Node.rankedP[i].y>=rect.ly){
            out_points[point_inside]=Node.rankedP[i];
            point_inside++;
        }
    return point_inside;
}

Rect_OK rect_OK_down=rect_OK;
Rect_OK rect_OK_up=rect_OK;

Point out_down[20];
int count_down=0;
Point out_up[20];
int count_up=0;

if(Node.XY==0){
#pragma omp parallel
    {
    if(omp_get_thread_num()==0){
        //lower half
        if( Node.down>=rect.lx  && Node.down<=rect.hx)rect_OK_down.lx=1;else
        rect_OK_down.lx=0;
        if(Node.mid<=rect.hx && Node.mid>=rect.lx)rect_OK_down.hx=1;
        else
        rect_OK_down.hx=0;
        if(rect_OK_down.lx||rect_OK_down.hx){
            count_down=Search2D(*(Node.TLow),rect,rect_OK_down,out_down);
            }
        else if(Node.down<rect.lx && Node.mid>rect.hx){
            count_down=Search2D(*(Node.TLow),rect,rect_OK_down,out_down);
            }
        }
    if(omp_get_thread_num()==2){
        //upper half
        if(Node.mid>=rect.lx && Node.mid<=rect.hx)rect_OK_up.lx=1;
        else rect_OK_up.lx=0;
        if(Node.up<=rect.hx && Node.up>=rect.lx)rect_OK_up.hx=1;
        else rect_OK_up.hx=0;
        if(rect_OK_up.lx||rect_OK_up.hx){
            count_up=Search2D(*(Node.THigh),rect,rect_OK_up,out_up);
        }

        if(Node.mid<rect.lx && Node.up>rect.hx){
            count_up=Search2D(*(Node.THigh),rect,rect_OK_up,out_up);
            }
        }
    }
}

//Scal - Merge Sort
int it_down=0;
int it_up=0;
for(int i=0;i<std::min(20,count_up+count_down);i++){
    if( (out_down[it_down].rank<out_up[it_up].rank&&it_down<count_down)||(it_up==count_up)  ){
        out_points[i]=out_down[it_down];
        it_down++;
        }
    else{
        out_points[i]=out_up[it_up];
        it_up++;
    }

}
return std::min(20,count_up+count_down);

}//*/


}


SearchContext* __stdcall DLL_EXP destroy(SearchContext* sc){
size=0;
//size_extreme=0;

//delete[] point_table;
//delete[] output_table;
//while(1);
return nullptr;
}

