cmake_minimum_required(VERSION 2.6)

project(sleepy_64)

ADD_DEFINITIONS(-DNOMINMAX -DUNICODE -D_UNICODE)
ADD_DEFINITIONS(/arch:AVX2)

set(SRC extern/point_search.h
        extern/dllmain.cpp

        data_conversion.cpp
        data_conversion.h
        geom.h
        tree64.cpp
        tree64.h
        search_amd64.cpp
        search_amd64.h)

add_library(sleepy_64 SHARED ${SRC})
