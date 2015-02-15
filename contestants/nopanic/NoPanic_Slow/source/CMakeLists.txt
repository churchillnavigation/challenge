cmake_minimum_required(VERSION 2.8.12)
project(POINTSEARCH)

set (PUBLIC_RELEASE TRUE CACHE BOOL "Public release: only build selected targets and have nice codenames")
set (HAS_PAPI FALSE CACHE BOOL "Build PAPI enabled stuff")
set (MARCH "native" CACHE STRING "-march flag for compiler")

if(NOT MSVC)
	add_compile_options(-std=c++11 -Wall -Wextra -Werror -Wno-unused-parameter -Wno-unused-but-set-variable)
	add_compile_options(-fopenmp -O3 "-march=${MARCH}")
	
	SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fopenmp")
else(NOT MSVC)
	add_compile_options(/favor:INTEL64 /arch:AVX /GS- /W4)

	add_compile_options(/Ox) # Full optimizations
	add_compile_options(/GL) # Whole program optimizations
endif(NOT MSVC)

set(T2_PER_NODE 64)

add_library(tree100 SHARED point_search.h tree1x.cpp Tree.cpp PointRange.cpp Tree.h PointRange.h)

if (PUBLIC_RELEASE)
	set_target_properties(tree100 PROPERTIES OUTPUT_NAME "NoPanic_Slow")
endif (PUBLIC_RELEASE)

if (NOT PUBLIC_RELEASE)
	# snip-snip
endif (NOT PUBLIC_RELEASE)
