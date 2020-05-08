#ifndef __VM_H_
#define __VM_H_

#include <stack>

#include "chunk.h"

struct VM {
    Chunk* chunk;
    uint8_t* ip;  // instruction pointer
    std::stack<Value> stack;
};

enum class InterpretResult { OK, COMPILE_ERROR, RUNTIME_ERROR };

InterpretResult interpret(Chunk* chunk);

#endif  // __VM_H_
