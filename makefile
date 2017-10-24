CC=g++
ICC=icc
ASMBIN=nasm

all: tria tria-icc release/tria release/tria-icc
	

tria-bench: obj/triangle.o obj/main-bench.o
	$(CC) -m64 -g -D SSE_BENCH_ONLY -o tria-bench obj/main-bench.o obj/triangle.o -lgomp

tria: obj/triangle.o obj/ctriangle.o obj/main.o
	$(CC) -m64 -g -o tria obj/main.o obj/triangle.o obj/ctriangle.o `sdl-config --libs` -lgomp

tria-icc: obj/triangle.o
	$(ICC) -Ofast -std=c++11 -o tria-icc -lgomp `sdl-config --libs` main.cpp ctriangle.cpp obj/triangle.o

obj/ctriangle.o: ctriangle.cpp
	$(CC) -o obj/ctriangle.o -O2 -m64 -c -g ctriangle.cpp

obj/triangle.o: triangle.asm
	$(ASMBIN) -o obj/triangle.o -f elf64 -g -l obj/triangle.lst triangle.asm

obj/main-bench.o: main.cpp
	$(CC) -o obj/main-bench.o -O2 -D SSE_BENCH_ONLY -std=c++11 -m64 -c -g main.cpp

obj/main.o: main.cpp
	$(CC) -o obj/main.o -O2 -std=c++11 -m64 -c -g main.cpp

clean:
	rm -f obj/*.o
	rm -f obj/triangle.lst
	rm -f tria
	rm -f tria-icc
	rm -f release/tria
	rm -f release/tria-icc

