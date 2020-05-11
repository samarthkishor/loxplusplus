#ifndef __VALUE_H_
#define __VALUE_H_

#include <iostream>

enum ValueType { VAL_BOOL, VAL_NIL, VAL_NUMBER };

// "tagged union" for the low-level representation of a Value
struct Value {
    ValueType type;
    union {
        bool boolean;
        double number;
    } as;
};

#define BOOL_VAL(value) ((Value){VAL_BOOL, {.boolean = value}})
#define NIL_VAL ((Value){VAL_NIL, {.number = 0}})
#define NUMBER_VAL(value) ((Value){VAL_NUMBER, {.number = value}})

bool valuesEqual(Value a, Value b);
void printValue(Value value);

#endif  // __VALUE_H_
