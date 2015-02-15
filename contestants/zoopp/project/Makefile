
dll: $(wildcard src/*.cpp)
	mkdir -p build
	g++ -shared -static -o build/zoopp.dll src/*.cpp -std=c++14 -O3 -march=native
	strip -s build/mihai.dll
	cp lib/cygwin1.dll build/

clean:
	rm -r build
