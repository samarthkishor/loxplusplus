#ifndef __OBJECT_H_
#define __OBJECT_H_

#include "value.h"

enum ObjType { OBJ_STRING };

#define OBJ_TYPE(value) ((value).as.obj->type)

#define IS_STRING(value) isObjType(value, OBJ_STRING)

#define AS_STRING(value) (ObjString*)((value).as.obj)
#define AS_CSTRING(value) (((ObjString*)((value).as.obj))->chars)

struct sObj {
    ObjType type;
    struct sObj* next;
};

struct sObjString {
    Obj obj;
    int length;
    char* chars;
};

ObjString* takeString(char* chars, int length);

ObjString* copyString(const char* chars, int length);

void printObject(Value value);

static inline bool isObjType(Value value, ObjType type) {
    return IS_OBJ(value) && (value.as.obj)->type == type;
}

#endif  // __OBJECT_H_
