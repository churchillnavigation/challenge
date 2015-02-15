TEMPLATE = lib
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt
TARGET = stefan

SOURCES += \
    solution.cpp \
    dll.cpp \
    rankheap.cpp \
    binary_search.cpp

include(deployment.pri)
qtcAddDeployment()

HEADERS += \
    point_search.h \
    dll.h \
    solution.h \
    util.h \
    timer.h \
    rankheap.h \
    aligned_allocator.h \
    binary_search.h

DEFINES += CHURCHILL_EXPORTS

*-msvc-* {
    QMAKE_CXXFLAGS += \
        /Zi \  # create external .pdb
        /GS-\  # disable buffer security checks
        /GL \  # global optimizations
        /arch:AVX
    QMAKE_CXXFLAGS_RELEASE += \
        /FA \  # generate .asm
        /O2 \
        /EHsc
    QMAKE_LFLAGS_RELEASE += \
        /OPT:REF    # remove unused functions
}
*mingw* {
    QMAKE_CXXFLAGS += \
        -std=c++11 \
        -march=native
    QMAKE_CXXFLAGS_RELEASE += \
        -O3 \
        -fno-rtti \
        -mtune=native
}
