# Super Q&D Makefile for automatic builds where MonoDevelop isn't available.

CC=g++
CFLAGS=-DDEBUG -D_FILE_OFFSET_BITS=64 -std=c++11 -O3 -lrt -lpthread -lfuse -lOpenCL

all: mawd mawsh

mawd:
	mkdir -p Maws/bin/Debug
	$(CC) $(CFLAGS) Maws/{*,*/}*.cpp -o Maws/bin/Debug/mawd

mawsh:
	mkdir -p Mawsh/bin/Debug
	$(CC) $(CFLAGS) Mawsh/*.cpp -o Mawsh/bin/Debug/mawsh

clean:
	rm -f Mawsh/bin/Debug/mawsh Maws/bin/Debug/mawd
