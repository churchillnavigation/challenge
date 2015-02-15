CC=x86_64-w64-mingw32-g++-win32
TARGET=opengl_clip_plane_search.dll

$(TARGET): manatee.cpp Makefile
	$(CC) -march=haswell -O3 -funroll-loops -funsafe-loop-optimizations -funsafe-math-optimizations -ffast-math -shared -static-libgcc -static-libstdc++ -std=c++14 -static -lpthread -Wall -DNDEBUG -o $(TARGET) manatee.cpp
	strip $(TARGET)

test: $(TARGET)
	WINEPREFIX=~/.wine64 wine64 point_search.exe $(TARGET) reference.dll

clean: Makefile
	rm -f $(TARGET)
