
The distribution contains the following components:

1) source code

   - point_search.h
   - schultzreference_global.h
   - SchultzReference.h
   - SchultzReference.cpp

2) project files

   - Makefile
   - Makefile.Release
   - SelenatorReference.pro (Qt qmake project file)

3) MinGW 4.9.1 r1 SEH tool chain

   - mingw64\

4) pre-built project

   - release\SelenatorReference.dll

5) project build contents and instructions

   - README  - "THIS FILE!!!!"


Build Instructions
=============================

1) unzip the SelenatorReference.zip file into root directory of choice

2) open Windows command shell

3) cd to "SelenatorReference" directory

4) set PATH=%PATH%;.\mingw64\bin

5) mingw32-make.exe clean

6) mingw32-make.exe

NOTE 1: After these steps (assuming no errors) there will be a .dll file in the "release" directory.

NOTE 2: The MinGW 4.9.1 r1 distribution included for completeness; however, other MinGW distributions can be used as well.  Just point the PATH variable to that distribution in Step 4 instead.

NOTE 3: Qt Creator 3.2.1 and Qt 5.3.2 OpenGL 64-bit distributions using the MSVC2013 and MinGW 4.9.1 r1 tool chains were used in the development of this software.  The Makefile and Makefile.Release project files included here were modified from the qmake generated versions to provide out-of-the-box compilation without any Qt dependencies.  The MinGW built dll seems to be faster than the Visual Studio 2013 dll.

NOTE 4: Licenses for MinGW 4.9.1 r1 binary distribution are located in the "mingw64\licenses" directory.
