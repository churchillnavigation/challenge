
The distribution contains the following components:

1) source code

   - point_search.h
   - schultzreference_global.h
   - SchultzReference.h
   - SchultzReference.cpp

2) project files

   - Makefile
   - Makefile.Release
   - SelenatorStageOne.pro (Qt qmake project file)

3) pre-built project

   - release\SelenatorStageOne.dll

4) project build contents and instructions

   - README  - "THIS FILE!!!!"


Build Instructions
=============================

1) unzip the "SelenatorStageOne.zip" file into root directory of choice

2) open Visual Studio Command Window

3) cd to "SelenatorStageOne" directory

4) nmake /f Makefile clean

5) nmake /f Makefile

NOTE 1: After these steps (assuming no errors) there will be a .dll file in the "release" directory.
