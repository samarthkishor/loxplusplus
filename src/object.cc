#include "object.h"

#include <stdio.h>
#include <string.h>

#include "memory.h"
#include "object.h"
#include "value.h"
#include "vm.hh"

#define ALLOCATE_OBJ(type, objectType) (type*)allocateObject(sizeof(type), objectType)

static Obj* allocateObject(size_t size, ObjType type) {
    Obj* object = (Obj*)reallocate(NULL, 0, size);
    object->type = type;

    object->next = vm.objects;
    vm.objects = object;
    return object;
}

static ObjString* allocateString(char* chars, int length) {
    ObjString* string = ALLOCATE_OBJ(ObjString, OBJ_STRING);
    string->length = length;
    string->chars = chars;

    vm.strings.insert(string);

    return string;
}

ObjString* takeString(char* chars, int length) {
    ObjString* str = new ObjString{{OBJ_STRING, nullptr}, length, strdup(chars)};
    auto interned_iter = vm.strings.find(str);
    if (interned_iter != vm.strings.end()) {
        FREE_ARRAY(char, chars, length + 1);
        return *interned_iter;
    }
    delete str;

    return allocateString(chars, length);
}

/**
 * Copies a C string into a Lox string.
 * If the string is already in the VM, return the existing string.
 */
ObjString* copyString(const char* chars, int length) {
    ObjString* str = new ObjString{{OBJ_STRING, nullptr}, length, strdup(chars)};
    auto interned_iter = vm.strings.find(str);
    if (interned_iter != vm.strings.end()) {
        return *interned_iter;
    }
    delete str;

    char* heapChars = ALLOCATE(char, length + 1);
    memcpy(heapChars, chars, length);
    heapChars[length] = '\0';

    return allocateString(heapChars, length);
}

void printObject(Value value) {
    switch (OBJ_TYPE(value)) {
        case OBJ_STRING:
            printf("%s", AS_CSTRING(value));
            break;
    }
}
