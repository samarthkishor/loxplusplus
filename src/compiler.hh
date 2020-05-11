#ifndef __COMPILER_H_
#define __COMPILER_H_

#include <string>

#include "chunk.h"

bool compile(std::string source, Chunk* chunk);

#endif  // __COMPILER_H_
