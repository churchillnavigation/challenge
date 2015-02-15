TEMPLATE = lib
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt
TARGET = stefan

SOURCES += \
    solution.cpp \
    dll.cpp

include(deployment.pri)
qtcAddDeployment()

HEADERS += \
    point_search.h \
    dll.h \
    solution.h

DEFINES += CHURCHILL_EXPORTS


QMAKE_CXXFLAGS += /arch:AVX
