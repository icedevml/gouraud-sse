CC=g++
ICC=icc
ASMBIN=nasm

all: tria tria-icc release/tria release/tria-icc
	

tria: obj/triangle.o obj/ctriangle.o obj/main.o
	$(CC) -m64 -g -o tria obj/main.o obj/triangle.o obj/ctriangle.o `sdl-config --libs` -lgomp

tria-icc: obj/triangle.o
	$(ICC) -Ofast -std=c++11 -o tria-icc -lgomp `sdl-config --libs` main.cpp ctriangle.cpp obj/triangle.o

release/tria: tria
	cp tria release/tria_
	strip -s release/tria_
	upx --ultra-brute --overlay=strip release/tria_
	sed 's/UPX!/\x00\x00\x00\x00/g' release/tria_ > release/tria
	rm -f release/tria_

release/tria-icc: tria-icc
	cp tria-icc release/tria-icc_
	strip -s release/tria-icc_
	upx --ultra-brute --overlay=strip release/tria-icc_
	sed 's/UPX!/\x00\x00\x00\x00/g' release/tria-icc_ > release/tria-icc
	rm -f release/tria-icc_

obj/ctriangle.o: ctriangle.cpp
	$(CC) -o obj/ctriangle.o -O2 -m64 -c -g ctriangle.cpp

obj/triangle.o: triangle.asm
	$(ASMBIN) -o obj/triangle.o -f elf64 -g -l obj/triangle.lst triangle.asm

obj/main.o: main.cpp
	$(CC) -o obj/main.o -O2 -std=c++11 -m64 -c -g main.cpp

clean:
	rm -f obj/*.o
	rm -f obj/triangle.lst
	rm -f tria
	rm -f tria-icc
	rm -f release/tria
	rm -f release/tria-icc

