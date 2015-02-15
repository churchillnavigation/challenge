solution.dll: solution.cpp
	x86_64-w64-mingw32-g++ $^ -o $@ -shared -std=c++11 \
		-fno-exceptions -fno-rtti -static \
		-Wl,--strip-all,--discard-all -O3 -Wall
