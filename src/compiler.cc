#include "compiler.hh"

#include <cstdio>
#include <iostream>

// I'm too lazy to write the scanner by hand in C++ so I just copied the book's C code
extern "C" {
#include "scanner.h"
void initScanner(const char*);
Token scanToken(void);
}

void compile(std::string source) {
    initScanner(source.c_str());
    int line = -1;
    while (true) {
        Token token = scanToken();
        if (token.line != line) {
            printf("%4d ", token.line);
            line = token.line;
        } else {
            std::cout << "   | ";
        }
        printf("%2d '%.*s'\n", token.type, token.length, token.start);

        if (token.type == TOKEN_EOF) break;
    }
}
