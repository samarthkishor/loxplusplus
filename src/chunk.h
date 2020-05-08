#ifndef __CHUNK_H_
#define __CHUNK_H_

#include <stdint.h>
#include <vector>
#include <map>

#include "value.h"

enum OpCode {
    OP_CONSTANT,
    OP_RETURN,
};

struct Chunk {
    std::vector<uint8_t> code;
    std::vector<Value> constants;
    std::map<int, int> lines;
};

void writeChunk(Chunk* chunk, uint8_t byte, int line);
int addConstant(Chunk* chunk, Value value);

#endif // __CHUNK_H_