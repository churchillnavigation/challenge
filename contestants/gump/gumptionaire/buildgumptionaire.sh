#!/bin/bash
rm gumptionaire.o gumptionaire.dll libgumptionairedll.a
x86_64-w64-mingw32-g++ -march=native -Ofast -c -DEXPORT_DLL -Drestrict=__restrict gumptionaire.c
x86_64-w64-mingw32-g++ -shared -o gumptionaire.dll gumptionaire.o -Wl,--out-implib,libgumptionairedll.a