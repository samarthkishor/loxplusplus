#include "compiler.hh"

#include <cstdio>
#include <iostream>

#include "scanner.h"

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
