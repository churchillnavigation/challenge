#pragma once

#define myMin(x,y)  ((x)>(y)?(y):(x))
#define myMax(x,y)  ((x)<(y)?(y):(x))

#define ALIGN64 __declspec(align(64))
#define ALIGN32 __declspec(align(32))
#define ALIGN16 __declspec(align(16))
#define ALIGN8  __declspec(align(8))
#define ALIGN4  __declspec(align(4))

#define PT_LB_FLOAT    -1e9
#define PT_UB_FLOAT    +1e9

struct indexRank_t {
    int index;
    int rank;
};

