#ifndef __VALUE_H_
#define __VALUE_H_

#include <iostream>

// An ObjString can be safely converted to an Obj and vice-versa
// TODO Maybe simulate this with classes and stuff later?
typedef struct sObj Obj;
typedef struct sObjString ObjString;

enum ValueType { VAL_BOOL, VAL_NIL, VAL_NUMBER, VAL_OBJ };

// "tagged union" for the low-level representation of a Value
struct Value {
    ValueType type;
    union {
        bool boolean;
        double number;
        Obj* obj;
    } as;
};

#define BOOL_VAL(value) ((Value){VAL_BOOL, {.boolean = value}})
#define NIL_VAL ((Value){VAL_NIL, {.number = 0}})
#define NUMBER_VAL(value) ((Value){VAL_NUMBER, {.number = value}})
#define OBJ_VAL(object) ((Value){VAL_OBJ, {.obj = (Obj*)object}})

// TODO get rid of these macros eventually
#define IS_BOOL(value) ((value).type == VAL_BOOL)
#define IS_NUMBER(value) ((value).type == VAL_NUMBER)
#define IS_OBJ(value) ((value).type == VAL_OBJ)

bool valuesEqual(Value a, Value b);
void printValue(Value value);

#endif  // __VALUE_H_
