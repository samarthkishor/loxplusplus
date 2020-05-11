#include <stdio.h>
#include <stdlib.h>

#include <fstream>
#include <iostream>
#include <string>

#include "chunk.h"
#include "debug.h"
#include "vm.hh"

static void repl() {
    std::cout << "> ";
    for (std::string line; std::getline(std::cin, line);) {
        if (line.empty()) {
            std::cout << std::endl;
            break;
        }

        interpret(line);
        std::cout << "> ";
    }
}

static std::string readFile(std::string path) {
    FILE* file = fopen(path.c_str(), "rb");
    if (file == NULL) {
        std::cerr << "Could not open file \"" << path << "\"." << std::endl;
        exit(74);
    }

    fseek(file, 0L, SEEK_END);
    size_t file_size = ftell(file);
    rewind(file);

    char* buffer = new char[file_size + 1];
    size_t bytes_read = fread(buffer, sizeof(char), file_size, file);
    if (bytes_read < file_size) {
        std::cerr << "Could not read file \"" << path << "\"." << std::endl;
        exit(74);
    }

    buffer[bytes_read] = '\0';
    std::string file_contents = buffer;

    fclose(file);
    delete[] buffer;

    return file_contents;
}

static void runFile(std::string path) {
    std::string source = readFile(path);
    InterpretResult result = interpret(source);

    if (result == InterpretResult::COMPILE_ERROR) {
        exit(65);
    }
    if (result == InterpretResult::RUNTIME_ERROR) {
        exit(70);
    }
}

int main(int argc, char* argv[]) {
    initVM();

    switch (argc) {
        case 1: {
            repl();
            break;
        }
        case 2: {
            runFile(argv[1]);
            break;
        }
        default: {
            std::cerr << "Usage: loxpp [path]" << std::endl;
            exit(64);
        }
    }

    freeVM();

    return 0;
}
