#-------------------------------------------------
#
# Project created by QtCreator 2014-12-15T20:31:58
#
#-------------------------------------------------

TEMPLATE = lib

CONFIG   -= qt

TARGET = SelenatorReference

# uncomment when using MinGW tool chain
#QMAKE_CXXFLAGS += -Ofast -march=native -mtune=intel -funroll-loops -mfpmath=sse -flto
QMAKE_CXXFLAGS += -Ofast -march=haswell -mtune=intel -funroll-loops -mfpmath=sse -flto
QMAKE_LFLAGS += -static -flto

DEFINES += SCHULTZSOFTWARESOLUTIONS_LIBRARY

SOURCES += \
    SchultzReference.cpp

HEADERS +=\
    point_search.h \
    schultzreference_global.h \
    SchultzReference.h

