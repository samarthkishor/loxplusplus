#ifndef __MEMORY_H_
#define __MEMORY_H_

#include <stdlib.h>

#include "object.h"

#define ALLOCATE(type, count) (type*)reallocate(NULL, 0, sizeof(type) * (count))

#define FREE(type, pointer) reallocate(pointer, sizeof(type), 0)

#define FREE_ARRAY(type, pointer, oldCount) reallocate(pointer, sizeof(type) * (oldCount), 0)

void* reallocate(void* previous, size_t oldSize, size_t newSize);

/**
 * Frees every object allocated by the VM
 */
void freeObjects();

#endif  // __MEMORY_H_
