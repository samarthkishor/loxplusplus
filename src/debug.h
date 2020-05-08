#ifndef __DEBUG_H_
#define __DEBUG_H_

#include "chunk.h"

#define DEBUG_TRACE_EXECUTION

void disassembleChunk(Chunk* chunk, const std::string& name);
int disassembleInstruction(Chunk* chunk, int offset);

#endif  // __DEBUG_H_
