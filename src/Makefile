CC=gcc
CXX=g++
CFLAGS=-g -Wall
CPPFLAGS=-g -std=c++2a -Wall -pedantic
LDFLAGS=-g

all: loxpp

loxpp: loxpp.o vm.o compiler.o scanner.o chunk.o debug.o value.o
	$(CXX) $(LDFLAGS) -o loxpp loxpp.o vm.o compiler.o scanner.o chunk.o debug.o value.o

loxpp.o: loxpp.cc chunk.h debug.h vm.hh

vm.o: vm.cc vm.hh chunk.h compiler.hh debug.h util.hh

compiler.o: compiler.cc compiler.hh scanner.h

scanner.o: scanner.c scanner.h
	$(CC) $(CFLAGS) -c scanner.c scanner.h

chunk.o: chunk.cc chunk.h value.h

debug.o: debug.cc debug.h value.h

value.o: value.cc value.h

clean:
	rm -f *.o

.PHONY: all clean
