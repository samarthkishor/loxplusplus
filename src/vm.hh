#ifndef __VM_H_
#define __VM_H_

#include <stack>
#include <unordered_set>

#include "chunk.h"
#include "object.h"
#include "value.h"

/**
 * Hash function for the unordered_set.
 */
struct hash_string {
    constexpr std::size_t operator()(ObjString* key) const {
        std::size_t hash = 2166136261u;
        for (int i = 0; i < key->length; i++) {
            hash ^= key->chars[i];
            hash *= 16777619;
        }

        // std::cout << "HASH " << hash << std::endl;

        return hash;
    }
};

/**
 * Returns true if both ObjStrings have the same characters.
 * TODO change this to the commented-out version once it works
 */
struct string_eq {
    constexpr bool operator()(ObjString* a, ObjString* b) const {
        // return (a->length == b->length) && (strcmp(a->chars, b->chars) == 0);
        return hash_string{}(a) == hash_string{}(b);
    }
};

struct VM {
    Chunk* chunk;
    uint8_t* ip;  // instruction pointer
    std::stack<Value> stack;
    Obj* objects;
    std::unordered_set<ObjString*, hash_string, string_eq> strings;  // for string interning
};

enum class InterpretResult { OK, COMPILE_ERROR, RUNTIME_ERROR };

void initVM();

void freeVM();

InterpretResult interpret(std::string source);

extern VM vm;  // allow other files to reference the global VM

#endif  // __VM_H_
