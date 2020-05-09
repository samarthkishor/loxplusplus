CXX=clang++
CXXFLAGS=-g -std=c++2a -O2 -Wall -pedantic
CXXFLAGS_ASAN=-g -std=c++2a -Wall -pedantic -fsanitize=address -D_GLIBCXX_DEBUG
LDFLAGS=-g
LDFLAGS_ASAN=-g -fsanitize=address

all: loxpp

# Debug with AddressSanitizer to detect memory leaks
debug: loxpp loxpp-asan

loxpp: loxpp.o vm.o compiler.o scanner.o chunk.o debug.o value.o
	$(CXX) $(LDFLAGS) -o loxpp loxpp.o vm.o compiler.o scanner.o chunk.o debug.o value.o

loxpp-asan: loxpp-asan.o vm-asan.o compiler-asan.o scanner-asan.o chunk-asan.o debug-asan.o value-asan.o
	$(CXX) $(LDFLAGS_ASAN) -o $@ $^

%-asan.o: %.cc
	$(CXX) -c $(CXXFLAGS_ASAN) -o $@ $<

loxpp.o: loxpp.cc chunk.h debug.h vm.hh
loxpp-asan.o: loxpp.cc chunk.h debug.h vm.hh

vm.o: vm.cc vm.hh chunk.h compiler.hh debug.h util.hh
vm-asan.o: vm.cc vm.hh chunk.h compiler.hh debug.h util.hh

compiler.o: compiler.cc compiler.hh scanner.h
compiler-asan.o: compiler.cc compiler.hh scanner.h

scanner.o: scanner.cc scanner.h
scanner-asan.o: scanner.cc scanner.h

chunk.o: chunk.cc chunk.h value.h
chunk-asan.o: chunk.cc chunk.h value.h

debug.o: debug.cc debug.h value.h
debug-asan.o: debug.cc debug.h value.h

value.o: value.cc value.h
value-asan.o: value.cc value.h

clean:
	rm -f *.o

.PHONY: all clean debug
