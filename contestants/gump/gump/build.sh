#!/bin/bash
rm gump.o gump.dll libgumpdll.a
x86_64-w64-mingw32-g++ -Ofast -c -DEXPORT_DLL gump.c
x86_64-w64-mingw32-g++ -shared -o gump.dll gump.o -Wl,--out-implib,libgumpdll.a
