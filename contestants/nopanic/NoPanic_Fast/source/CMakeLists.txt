cmake_minimum_required(VERSION 2.8.12)
project(POINTSEARCH)

set (PUBLIC_RELEASE FALSE CACHE BOOL "Public release: only build selected targets and have nice codenames")
set (HAS_PAPI FALSE CACHE BOOL "Build PAPI enabled stuff")
set (MARCH "native" CACHE STRING "-march flag for compiler")

if(HAS_PAPI)
	include_directories(/home/taavi/opt/papi/papi-5.4.0/include)
	link_directories(/home/taavi/opt/papi/papi-5.4.0/lib)
endif(HAS_PAPI)

if(NOT MSVC)
	add_compile_options(-std=c++11)
	#add_compile_options(-Wall -Wextra -Werror -Wno-unused-parameter -Wno-unused-but-set-variable)
	add_compile_options(-O3 "-march=${MARCH}")
	
	SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fopenmp")
else(NOT MSVC)
	add_compile_options(/favor:INTEL64 /arch:AVX /W4 /GS-)
	add_compile_options($<$<CONFIG:Release>:/Ox>) # Full optimizations, but only for release build
	add_compile_options(/GL) # Whole program optimizations
endif(NOT MSVC)

set(T2_PER_NODE 64)

add_library(tree100 SHARED point_search.h tree1x.cpp Tree.cpp PointRange.cpp Tree.h PointRange.h)

add_library(tree222 SHARED point_search.h tree2x.cpp Tree2.cpp Tree.cpp PointRange.cpp Tree.h Tree2.h PointRange.h)
set_property(TARGET tree222 APPEND PROPERTY COMPILE_DEFINITIONS PER_NODE=${T2_PER_NODE} USE_SIMD USE_MORE_SIMD Q2_INDIRECT UPDATE_MIN_RANK)

if (PUBLIC_RELEASE)
	set_target_properties(tree100 PROPERTIES OUTPUT_NAME "NoPanic_Slow")
	set_target_properties(tree222 PROPERTIES OUTPUT_NAME "NoPanic_Fast")
endif (PUBLIC_RELEASE)

