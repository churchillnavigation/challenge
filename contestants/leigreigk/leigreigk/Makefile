CC = x86_64-w64-mingw32-g++
LINUX_CC = g++
FLAGS=-Ofast  -c -Wall -std=gnu++1y -fopenmp -march=haswell 
#-fpic
all: leigreigk.dll 



%.o: %.cpp
	${CC} -o $@ ${FLAGS} $< 
	${LINUX_CC}  -pg -o moje.O ${FLAGS} -fpic moje.cpp 
leigreigk.dll: moje.o
	${CC} -shared -o leigreigk.dll moje.o -static-libgcc  -static-libstdc++ -static -fopenmp
#-lgomp
#-static-libgomp
	${LINUX_CC} -shared -pg -o libmoje.so moje.O 


#	${CC} -shared -o moje.dll \
#	-Wl,--out-implib=libmoje.dll.a \
#	-Wl,--export-all-symbols \
#	-Wl,--enable-auto-import \
#	-Wl,--whole-archive ${old_libs} \
#	-Wl,--no-whole-archive ${dependency_libs}

test.exe: moje.dll
	${CC} -o test.exe test.cpp -L./ -lleigreigk

main.exe: moje.dll main.cpp
	${CC} -o main.exe main.cpp -L./ -lleigreigk  -static-libstdc++ -static-libgcc -static-libgomp
	${LINUX_CC}  -pg -o main.out main.cpp moje.O
#-L./ -lmoje  
#-static-libstdc++ -static-libgcc 

main2.exe: main2.cpp
	${CC} -o main2.exe main.cpp -L./ -lreference  -static-libstdc++ -static-libgcc -static-libgomp
#gdb-mingw-w64 mingw-64 mingw-w64-tools mingw32

#binutils-mingw-w64-i686_2.24.51.20140818-1+5.1_amd64.deb    gfortran-mingw-w64-i686_4.9.1-19+14.3_amd64.deb    gnat-mingw-w64-x86-64_4.9.1-3+14_amd64.deb binutils-mingw-w64-x86-64_2.24.51.20140818-1+5.1_amd64.deb  gfortran-mingw-w64-x86-64_4.9.1-19+14.3_amd64.deb  mingw32_4.9.1-19+14.3_all.deb gcc-mingw-w64-base_4.9.1-19+14.3_amd64.deb                  g++-mingw-w64_4.9.1-19+14.3_all.deb                mingw32-binutils_2.24.51.20140818-1+5.1_all.deb gcc-mingw-w64-i686_4.9.1-19+14.3_amd64.deb                  g++-mingw-w64-i686_4.9.1-19+14.3_amd64.deb         mingw-w64_3.2.0-2_all.deb gcc-mingw-w64-x86-64_4.9.1-19+14.3_amd64.deb                g++-mingw-w64-x86-64_4.9.1-19+14.3_amd64.deb       mingw-w64-common_3.2.0-2_all.deb gdb-mingw-w64_7.6.2-1.1+9_amd64.deb                         gnat-mingw-w64_4.9.1-3+14_all.deb                  mingw-w64-i686-dev_3.2.0-2_all.deb gdb-mingw-w64-target_7.6.2-1.1+9_all.deb                    gnat-mingw-w64-base_4.9.1-3+14_amd64.deb           mingw-w64-tools_3.2.0-2_amd64.deb gfortran-mingw-w64_4.9.1-19+14.3_all.deb                    gnat-mingw-w64-i686_4.9.1-3+14_amd64.deb           mingw-w64-x86-64-dev_3.2.0-2_all.deb

