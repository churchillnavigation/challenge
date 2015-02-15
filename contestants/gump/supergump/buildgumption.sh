#!/bin/bash
rm gumption.o gumption.dll libgumptiondll.a
x86_64-w64-mingw32-g++ -Ofast -c -DEXPORT_DLL gumption.c
x86_64-w64-mingw32-g++ -shared -o gumption.dll gumption.o -Wl,--out-implib,libgumptiondll.a
