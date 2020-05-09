#include "vm.hh"

#include <functional>

#include "compiler.hh"
#include "debug.h"
#include "util.hh"

VM vm;

void binaryOp(std::function<Value(Value, Value)> op) {
    auto b = vm.stack.top();
    vm.stack.pop();
    auto a = vm.stack.top();
    vm.stack.pop();
    vm.stack.push(op(a, b));
}

static InterpretResult run() {
#define READ_BYTE() (*vm.ip++)
#define READ_CONSTANT() (vm.chunk->constants[READ_BYTE()])

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
            case OP_ADD: {
                binaryOp([](Value a, Value b) -> Value { return a + b; });
                break;
            }
            case OP_SUBTRACT: {
                binaryOp([](Value a, Value b) -> Value { return a - b; });
                break;
            }
            case OP_MULTIPLY: {
                binaryOp([](Value a, Value b) -> Value { return a * b; });
                break;
            }
            case OP_DIVIDE: {
                binaryOp([](Value a, Value b) -> Value { return a / b; });
                break;
            }
            case OP_NEGATE: {
                auto top = vm.stack.top();
                vm.stack.pop();
                vm.stack.push(-top);
                break;
            }
            case OP_RETURN: {
                printValue(vm.stack.top());
                std::cout << std::endl;
                vm.stack.pop();
                return InterpretResult::OK;
            }
        }
    }

#undef READ_BYTE
#undef READ_CONSTANT
}

InterpretResult interpret(std::string source) {
    compile(source);
    return InterpretResult::OK;
}
