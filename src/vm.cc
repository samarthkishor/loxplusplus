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

    vm.stack = std::vector<Value>();
}

static Value peek(int distance) {
    return vm.stack[vm.stack.size() - 1 - distance];
}

static bool isFalsey(Value value) {
    return (value.type == VAL_NIL) || (value.type == VAL_BOOL && !value.as.boolean);
}

static void concatenate() {
    auto bval = vm.stack.back();
    vm.stack.pop_back();
    auto aval = vm.stack.back();
    vm.stack.pop_back();
    ObjString* b = AS_STRING(bval);
    ObjString* a = AS_STRING(aval);

    int length = a->length + b->length;
    char* chars = ALLOCATE(char, length + 1);
    memcpy(chars, a->chars, a->length);
    memcpy(chars + a->length, b->chars, b->length);
    chars[length] = '\0';

    ObjString* result = takeString(chars, length);
    vm.stack.push_back(OBJ_VAL(result));
}

static InterpretResult binaryOp(std::function<Value(Value, Value)> op) {
    if (peek(0).type != VAL_NUMBER || peek(1).type != VAL_NUMBER) {
        runtimeError("Operands must be numbers.");
        return InterpretResult::RUNTIME_ERROR;
    }
    auto b = vm.stack.back();
    vm.stack.pop_back();
    auto a = vm.stack.back();
    vm.stack.pop_back();
    vm.stack.push_back(op(a, b));
    return InterpretResult::OK;
}

static InterpretResult run() {
#define READ_BYTE() (*vm.ip++)
#define READ_CONSTANT() (vm.chunk->constants[READ_BYTE()])
#define READ_SHORT() (vm.ip += 2, (uint16_t)((vm.ip[-2] << 8) | vm.ip[-1]))
#define READ_STRING() AS_STRING(READ_CONSTANT())

    while (true) {
#ifdef DEBUG_TRACE_EXECUTION
        std::cout << "          ";
        for (auto stack_iter = vm.stack.begin(); stack_iter != vm.stack.end(); ++stack_iter) {
            std::cout << "[ ";
            printValue(*stack_iter);
            std::cout << " ]";
        }
        std::cout << std::endl;
        disassembleInstruction(vm.chunk, (int)(vm.ip - vm.chunk->code.data()));
#endif

        uint8_t instruction;
        switch (instruction = READ_BYTE()) {
            case OP_CONSTANT: {
                Value constant = READ_CONSTANT();
                vm.stack.push_back(constant);
                break;
            }
            case OP_NIL: {
                vm.stack.push_back(NIL_VAL);
                break;
            }
            case OP_TRUE: {
                vm.stack.push_back(BOOL_VAL(true));
                break;
            }
            case OP_FALSE: {
                vm.stack.push_back(BOOL_VAL(false));
                break;
            }
            case OP_POP: {
                vm.stack.pop_back();
                break;
            }
            case OP_GET_LOCAL: {
                uint8_t slot = READ_BYTE();
                vm.stack.push_back(vm.stack[slot]);
                break;
            }
            case OP_SET_LOCAL: {
                uint8_t slot = READ_BYTE();
                vm.stack[slot] = peek(0);
                break;
            }
            case OP_GET_GLOBAL: {
                ObjString* name = READ_STRING();
                auto value_iter = vm.globals.find(name);
                if (value_iter == vm.globals.end()) {
                    runtimeError("Undefined variable '%s'.", name->chars);
                    return InterpretResult::RUNTIME_ERROR;
                }
                vm.stack.push_back(value_iter->second);
                break;
            }
            case OP_DEFINE_GLOBAL: {
                ObjString* name = READ_STRING();
                vm.globals.insert({name, peek(0)});
                vm.stack.pop_back();
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
                auto a = vm.stack.back();
                vm.stack.pop_back();
                auto b = vm.stack.back();
                vm.stack.pop_back();
                vm.stack.push_back(BOOL_VAL(valuesEqual(a, b)));
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
                    auto b = vm.stack.back();
                    vm.stack.pop_back();
                    auto a = vm.stack.back();
                    vm.stack.pop_back();
                    vm.stack.push_back(NUMBER_VAL(a.as.number + b.as.number));
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
                auto top = vm.stack.back();
                vm.stack.pop_back();
                vm.stack.push_back(BOOL_VAL(isFalsey(top)));
                break;
            }
            case OP_NEGATE: {
                if (peek(0).type != VAL_NUMBER) {
                    return InterpretResult::RUNTIME_ERROR;
                }
                auto top = vm.stack.back();
                vm.stack.pop_back();
                vm.stack.push_back(NUMBER_VAL(-top.as.number));
                break;
            }
            case OP_PRINT: {
                printValue(vm.stack.back());
                vm.stack.pop_back();
                std::cout << std::endl;
                break;
            }
            case OP_JUMP: {
                uint16_t offset = READ_SHORT();
                vm.ip += offset;
                break;
            }
            case OP_JUMP_IF_FALSE: {
                uint16_t offset = READ_SHORT();
                // if the expression is falsey, skip over the code in the then-branch
                if (isFalsey(peek(0))) {
                    vm.ip += offset;
                }
                break;
            }
            case OP_LOOP: {
                uint16_t offset = READ_SHORT();
                vm.ip -= offset;
                break;
            }
            case OP_RETURN: {
                return InterpretResult::OK;
            }
        }
    }

#undef READ_BYTE
#undef READ_SHORT
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
