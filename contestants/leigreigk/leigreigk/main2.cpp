#include "./moje.h"


struct aaa{
bool operator()(Point a, Point b)
{
	return a.rank < b.rank;
}
}CustomLessR;

int main(){
printf("hello\n");
const int punkty_len=615;
Point punkty[punkty_len];

for(int i=0;i<punkty_len;i++){
	punkty[i].id=int8_t(1111);
	punkty[i].rank=punkty_len-i;
	punkty[i].x=-float(i%10);
	punkty[i].y=-float(i);
}

std::partial_sort(&(punkty[0]),&(punkty[20]),&(punkty[punkty_len]),CustomLessR);
for(int i=0;i<punkty_len;i++){
std::cout<<punkty[i].rank<<' '<<punkty[i].x<<' '<<punkty[i].y<<' '<<punkty[i].id<<std::endl;
}
//return 0;

SearchContext *sc=create(punkty,&(punkty[punkty_len]) );
Point out_points[20];
Rect rect;
rect.lx=-20;
rect.ly=-20;
rect.hx=-1;
rect.hy=-1;



int count=search(sc,rect,20,out_points);
std::cout<<"OUT:"<<std::endl;
for(int i=0;i<count;i++)std::cout<<out_points[i].rank<<' '<<out_points[i].x<<' '<<out_points[i].y<<' '<<out_points[i].id<<std::endl;
std::cout<<'\n';
std::cout<<punkty[20].rank<<' '<<punkty[20].x<<' '<<punkty[20].y<<' '<<punkty[20].id<<std::endl;
destroy(sc);
return 0;
}
