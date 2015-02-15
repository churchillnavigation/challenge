@echo off
rem build script to compile DLL for Churchill Navigation programming challenge
rem Sascha Kratky, skratky@gmail.com

setlocal enabledelayedexpansion

cd /D "%~dp0"

if defined VS120COMNTOOLS (
	set "VISUAL_STUDIO_HOME=!VS120COMNTOOLS!\..\.."
) else (
	echo Error: Visual Studio 2013 is not installed!
	exit /B 1
)

call "!VISUAL_STUDIO_HOME!\VC\vcvarsall.bat" x64

cl.exe /nologo /W3 /EHsc /MT /O2 /GL /DNDEBUG point_search_sakra.cpp /link /dll /ltcg /out:sakra.dll
