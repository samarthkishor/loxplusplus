#include "vm.hh"

#include <cstdarg>
#include <cstdio>
#include <functional>

#include "compiler.hh"
#include "debug.h"
#include "memory.h"
#include "object.h"

VM vm;

void initVM() {
    vm.objects = nullptr;
}

void freeVM() {
    freeObjects();
}

static void runtimeError(const char* format, ...) {
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    std::cerr << std::endl;

    size_t instruction = vm.ip - vm.chunk->code.data() - 1;
    int line = vm.chunk->lines[instruction];
    fprintf(stderr, "[line %d] in script\n", line);

    vm.stack = std::stack<Value>();
}

static Value peek(int distance) {
    // TODO maybe a stack isn't the most efficient way to do this
    if (distance == 0) {
        return vm.stack.top();
    }

    if (distance == 1) {
        Value top = vm.stack.top();
        vm.stack.pop();
        Value result = vm.stack.top();
        vm.stack.push(top);
        return result;
    }

    return stackToVec(vm.stack)[distance];
}

static bool isFalsey(Value value) {
    return (value.type == VAL_NIL) || (value.type == VAL_BOOL && !value.as.boolean);
}

static void concatenate() {
    auto bval = vm.stack.top();
    vm.stack.pop();
    auto aval = vm.stack.top();
    vm.stack.pop();
    ObjString* b = AS_STRING(bval);
    ObjString* a = AS_STRING(aval);

    int length = a->length + b->length;
    char* chars = ALLOCATE(char, length + 1);
    memcpy(chars, a->chars, a->length);
    memcpy(chars + a->length, b->chars, b->length);
    chars[length] = '\0';

    ObjString* result = takeString(chars, length);
    vm.stack.push(OBJ_VAL(result));
}

static InterpretResult binaryOp(std::function<Value(Value, Value)> op) {
    if (peek(0).type != VAL_NUMBER || peek(1).type != VAL_NUMBER) {
        runtimeError("Operands must be numbers.");
        return InterpretResult::RUNTIME_ERROR;
    }
    auto b = vm.stack.top();
    vm.stack.pop();
    auto a = vm.stack.top();
    vm.stack.pop();
    vm.stack.push(op(a, b));
    return InterpretResult::OK;
}

static InterpretResult run() {
#define READ_BYTE() (*vm.ip++)
#define READ_CONSTANT() (vm.chunk->constants[READ_BYTE()])
#define READ_STRING() AS_STRING(READ_CONSTANT())

    while (true) {
#ifdef DEBUG_TRACE_EXECUTION
        std::cout << "          ";
        std::stack<Value> temp;
        reverseStack(vm.stack, temp);
        while (!temp.empty()) {
            std::cout << "[ ";
            printValue(temp.top());
            temp.pop();
            std::cout << " ] ";
        }
        std::cout << std::endl;
        disassembleInstruction(vm.chunk, (int)(vm.ip - vm.chunk->code.data()));
#endif

        uint8_t instruction;
        switch (instruction = READ_BYTE()) {
            case OP_CONSTANT: {
                Value constant = READ_CONSTANT();
                vm.stack.push(constant);
                break;
            }
            case OP_NIL: {
                vm.stack.push(NIL_VAL);
                break;
            }
            case OP_TRUE: {
                vm.stack.push(BOOL_VAL(true));
                break;
            }
            case OP_FALSE: {
                vm.stack.push(BOOL_VAL(false));
                break;
            }
            case OP_POP: {
                vm.stack.pop();
                break;
            }
            case OP_GET_GLOBAL: {
                ObjString* name = READ_STRING();
                auto value_iter = vm.globals.find(name);
                if (value_iter == vm.globals.end()) {
                    runtimeError("Undefined variable '%s'.", name->chars);
                    return InterpretResult::RUNTIME_ERROR;
                }
                vm.stack.push(value_iter->second);
                break;
            }
            case OP_DEFINE_GLOBAL: {
                ObjString* name = READ_STRING();
                vm.globals.insert({name, peek(0)});
                vm.stack.pop();
                break;
            }
            case OP_SET_GLOBAL: {
                ObjString* name = READ_STRING();
                auto value_iter = vm.globals.find(name);
                if (value_iter == vm.globals.end()) {
                    runtimeError("Undefined variable '%s'.", name->chars);
                    return InterpretResult::RUNTIME_ERROR;
                }
                vm.globals[name] = peek(0);
                break;
            }
            case OP_EQUAL: {
                auto a = vm.stack.top();
                vm.stack.pop();
                auto b = vm.stack.top();
                vm.stack.pop();
                vm.stack.push(BOOL_VAL(valuesEqual(a, b)));
                break;
            }
            case OP_GREATER: {
                auto res = binaryOp(
                    [](Value a, Value b) -> Value { return BOOL_VAL(a.as.number > b.as.number); });
                if (res == InterpretResult::RUNTIME_ERROR) {
                    return InterpretResult::RUNTIME_ERROR;
                }
                break;
            }
            case OP_LESS: {
                auto res = binaryOp(
                    [](Value a, Value b) -> Value { return BOOL_VAL(a.as.number < b.as.number); });
                if (res == InterpretResult::RUNTIME_ERROR) {
                    return InterpretResult::RUNTIME_ERROR;
                }
                break;
            }
            case OP_ADD: {
                if (IS_STRING(peek(0)) && IS_STRING(peek(1))) {
                    concatenate();
                } else if (IS_NUMBER(peek(0)) && IS_NUMBER(peek(1))) {
                    auto b = vm.stack.top();
                    vm.stack.pop();
                    auto a = vm.stack.top();
                    vm.stack.pop();
                    vm.stack.push(NUMBER_VAL(a.as.number + b.as.number));
                } else {
                    runtimeError("Operands must be two numbers or two strings.");
                    return InterpretResult::RUNTIME_ERROR;
                }
                break;
            }
            case OP_SUBTRACT: {
                auto res = binaryOp([](Value a, Value b) -> Value {
                    return NUMBER_VAL(a.as.number - b.as.number);
                });
                if (res == InterpretResult::RUNTIME_ERROR) {
                    return InterpretResult::RUNTIME_ERROR;
                }
                break;
            }
            case OP_MULTIPLY: {
                auto res = binaryOp([](Value a, Value b) -> Value {
                    return NUMBER_VAL(a.as.number * b.as.number);
                });
                if (res == InterpretResult::RUNTIME_ERROR) {
                    return InterpretResult::RUNTIME_ERROR;
                }
                break;
            }
            case OP_DIVIDE: {
                auto res = binaryOp([](Value a, Value b) -> Value {
                    return NUMBER_VAL(a.as.number / b.as.number);
                });
                if (res == InterpretResult::RUNTIME_ERROR) {
                    return InterpretResult::RUNTIME_ERROR;
                }
                break;
            }
            case OP_NOT: {
                auto top = vm.stack.top();
                vm.stack.pop();
                vm.stack.push(BOOL_VAL(isFalsey(top)));
                break;
            }
            case OP_NEGATE: {
                if (peek(0).type != VAL_NUMBER) {
                    return InterpretResult::RUNTIME_ERROR;
                }
                auto top = vm.stack.top();
                vm.stack.pop();
                vm.stack.push(NUMBER_VAL(-top.as.number));
                break;
            }
            case OP_PRINT: {
                printValue(vm.stack.top());
                vm.stack.pop();
                std::cout << std::endl;
                break;
            }
            case OP_RETURN: {
                return InterpretResult::OK;
            }
        }
    }

#undef READ_BYTE
#undef READ_CONSTANT
#undef READ_STRING
}

InterpretResult interpret(std::string source) {
    Chunk chunk;

    if (!compile(source, &chunk)) {
        return InterpretResult::COMPILE_ERROR;
    }

    vm.chunk = &chunk;
    vm.ip = vm.chunk->code.data();

    auto result = run();

    return result;
}
