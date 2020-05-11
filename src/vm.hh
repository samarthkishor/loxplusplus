#ifndef __VM_H_
#define __VM_H_

#include <stack>

#include "chunk.h"

struct VM {
    Chunk* chunk;
    uint8_t* ip;  // instruction pointer
    std::stack<Value> stack;
    Obj* objects;
};

enum class InterpretResult { OK, COMPILE_ERROR, RUNTIME_ERROR };

void initVM();

void freeVM();

InterpretResult interpret(std::string source);

extern VM vm;  // allow other files to reference the global VM

#endif  // __VM_H_
