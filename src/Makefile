CXX=clang++
CXXFLAGS=-g -std=c++2a -O2 -Wall
CXXFLAGS_ASAN=-g -std=c++2a -Wall -fsanitize=address -D_GLIBCXX_DEBUG
LDFLAGS=-g
LDFLAGS_ASAN=-g -fsanitize=address

all: loxpp loxpp-asan

# Debug with AddressSanitizer to detect memory leaks
debug: loxpp-asan

loxpp: loxpp.o vm.o compiler.o scanner.o chunk.o debug.o value.o memory.o object.o
	$(CXX) $(LDFLAGS_ASAN) -o $@ $^

loxpp-asan: loxpp-asan.o vm-asan.o compiler-asan.o scanner-asan.o chunk-asan.o debug-asan.o value-asan.o memory-asan.o object-asan.o
	$(CXX) $(LDFLAGS_ASAN) -o $@ $^

%-asan.o: %.cc
	$(CXX) -c $(CXXFLAGS_ASAN) -o $@ $<

loxpp.o: loxpp.cc chunk.h debug.h vm.hh
loxpp-asan.o: loxpp.cc chunk.h debug.h vm.hh

vm.o: vm.cc vm.hh chunk.h compiler.hh debug.h memory.h object.h
vm-asan.o: vm.cc vm.hh chunk.h compiler.hh debug.h memory.h object.h

memory.o: memory.cc memory.h object.h vm.hh
memory-asan.o: memory.cc memory.h object.h vm.hh

object.o: object.cc object.h value.h memory.h vm.hh
object-asan.o: object.cc object.h value.h memory.h vm.hh

compiler.o: compiler.cc compiler.hh scanner.h chunk.h object.h
compiler-asan.o: compiler.cc compiler.hh scanner.h chunk.h object.h

scanner.o: scanner.cc scanner.h
scanner-asan.o: scanner.cc scanner.h

chunk.o: chunk.cc chunk.h value.h
chunk-asan.o: chunk.cc chunk.h value.h

debug.o: debug.cc debug.h value.h
debug-asan.o: debug.cc debug.h value.h

value.o: value.cc value.h object.h
value-asan.o: value.cc value.h object.h

clean:
	rm -f *.o

.PHONY: all clean debug
