CC=x86_64-w64-mingw32-g++
TARGET=manatee_opengl2_clip_plane.dll

$(TARGET): manatee.cpp Makefile
	$(CC) -march=ivybridge -O2 -funsafe-loop-optimizations -funsafe-math-optimizations -ffast-math -shared -static-libgcc -static-libstdc++ -std=c++14 -static -lpthread -Wall -DNDEBUG -o $(TARGET) manatee2.cpp
# -march=haswell
	strip $(TARGET)

test: $(TARGET)
	WINEPREFIX=~/.wine64 wine64 point_search.exe $(TARGET) reference.dll -p100000

clean: Makefile
	rm -f $(TARGET)
