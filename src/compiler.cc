#include "compiler.hh"

#include <cstdio>
#include <iostream>

#include "debug.h"
#include "object.h"
#include "scanner.h"

struct Parser {
    Token current;
    Token previous;
    bool hadError;
    bool panicMode;  // makes sure errors don't cascade
};

enum Precedence {
    PREC_NONE,
    PREC_ASSIGNMENT,  // =
    PREC_OR,          // or
    PREC_AND,         // and
    PREC_EQUALITY,    // == !=
    PREC_COMPARISON,  // < > <= >=
    PREC_TERM,        // + -
    PREC_FACTOR,      // * /
    PREC_UNARY,       // ! -
    PREC_CALL,        // . ()
    PREC_PRIMARY
};

typedef void (*ParseFn)(bool canAssign);

struct ParseRule {
    ParseFn prefix;
    ParseFn infix;
    Precedence precedence;
};

struct Local {
    Token name;
    int depth;
};

struct Compiler {
    std::vector<Local> locals;  // has the same layout as variables on the VM's stack
    int scopeDepth;
};

Parser parser;
Compiler* current = nullptr;
Chunk* compilingChunk;

static Chunk* currentChunk() {
    return compilingChunk;
}

static void errorAt(Token* token, std::string message) {
    if (parser.panicMode) {
        return;
    }
    parser.panicMode = true;

    fprintf(stderr, "[line %d] Error", token->line);

    if (token->type == TOKEN_EOF) {
        std::cerr << " at end";
    } else if (token->type == TOKEN_ERROR) {
        // Nothing.
    } else {
        fprintf(stderr, " at '%.*s'", token->length, token->start);
    }

    fprintf(stderr, ": %s\n", message.c_str());
    parser.hadError = true;
}

static void error(std::string message) {
    errorAt(&parser.previous, message);
}

static void errorAtCurrent(std::string message) {
    errorAt(&parser.current, message);
}

static void advance() {
    parser.previous = parser.current;

    for (;;) {
        parser.current = scanToken();
        if (parser.current.type != TOKEN_ERROR) break;

        errorAtCurrent(parser.current.start);
    }
}

static void consume(TokenType type, std::string message) {
    if (parser.current.type == type) {
        advance();
        return;
    }

    errorAtCurrent(message);
}

static bool check(TokenType type) {
    return parser.current.type == type;
}

static bool match(TokenType type) {
    if (!check(type)) {
        return false;
    }
    advance();
    return true;
}

static void emitByte(uint8_t byte) {
    writeChunk(currentChunk(), byte, parser.previous.line);
}

static void emitBytes(uint8_t byte1, uint8_t byte2) {
    emitByte(byte1);
    emitByte(byte2);
}

static void emitReturn() {
    emitByte(OP_RETURN);
}

static uint8_t makeConstant(Value value) {
    int constant = addConstant(currentChunk(), value);
    if (constant > UINT8_MAX) {
        error("Too many constants in one chunk.");
        return 0;
    }

    return (uint8_t)constant;
}

static void emitConstant(Value value) {
    emitBytes(OP_CONSTANT, makeConstant(value));
}

static void initCompiler(Compiler* compiler) {
    compiler->scopeDepth = 0;
    current = compiler;
}

static void endCompiler() {
    emitReturn();
#ifdef DEBUG_PRINT_CODE
    if (!parser.hadError) {
        disassembleChunk(currentChunk(), "code");
    }
#endif
}

static void beginScope() {
    current->scopeDepth++;
}

static void endScope() {
    current->scopeDepth--;

    // when the block ends, pop the variables from the previous scope
    while (current->locals.size() > 0 && current->locals.back().depth > current->scopeDepth) {
        emitByte(OP_POP);
        current->locals.pop_back();
    }
}

static void expression();
static void statement();
static void declaration();
static uint8_t parseVariable(const char* errorMessage);
static void defineVariable(uint8_t global);
static ParseRule* getRule(TokenType type);
static void parsePrecedence(Precedence precedence);

static void binary(bool canAssign) {
    // Remember the operator.
    TokenType operatorType = parser.previous.type;

    // Compile the right operand.
    ParseRule* rule = getRule(operatorType);
    parsePrecedence((Precedence)(rule->precedence + 1));

    // Emit the operator instruction.
    switch (operatorType) {
        case TOKEN_BANG_EQUAL:
            emitBytes(OP_EQUAL, OP_NOT);
            break;
        case TOKEN_EQUAL_EQUAL:
            emitByte(OP_EQUAL);
            break;
        case TOKEN_GREATER:
            emitByte(OP_GREATER);
            break;
        case TOKEN_GREATER_EQUAL:
            emitBytes(OP_LESS, OP_NOT);
            break;
        case TOKEN_LESS:
            emitByte(OP_LESS);
            break;
        case TOKEN_LESS_EQUAL:
            emitBytes(OP_GREATER, OP_NOT);
            break;
        case TOKEN_PLUS:
            emitByte(OP_ADD);
            break;
        case TOKEN_MINUS:
            emitByte(OP_SUBTRACT);
            break;
        case TOKEN_STAR:
            emitByte(OP_MULTIPLY);
            break;
        case TOKEN_SLASH:
            emitByte(OP_DIVIDE);
            break;
        default:
            return;  // Unreachable.
    }
}

static void literal(bool canAssign) {
    switch (parser.previous.type) {
        case TOKEN_FALSE:
            emitByte(OP_FALSE);
            break;
        case TOKEN_NIL:
            emitByte(OP_NIL);
            break;
        case TOKEN_TRUE:
            emitByte(OP_TRUE);
            break;
        default:
            return;  // Unreachable.
    }
}

static void expression() {
    parsePrecedence(PREC_ASSIGNMENT);
}

static void block() {
    while (!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF)) {
        declaration();
    }

    consume(TOKEN_RIGHT_BRACE, "Expect '}' after block.");
}

static void varDeclaration() {
    uint8_t global = parseVariable("Expect variable name.");

    if (match(TOKEN_EQUAL)) {
        expression();
    } else {
        emitByte(OP_NIL);
    }
    consume(TOKEN_SEMICOLON, "Expect ';' after variable declaration.");

    defineVariable(global);
}

static void expressionStatement() {
    expression();
    consume(TOKEN_SEMICOLON, "Expect ';' after expression.");
    emitByte(OP_POP);
}

static void printStatement() {
    expression();
    consume(TOKEN_SEMICOLON, "Expect ';' after value.");
    emitByte(OP_PRINT);
}

static void synchronize() {
    parser.panicMode = false;

    while (parser.current.type != TOKEN_EOF) {
        if (parser.previous.type == TOKEN_SEMICOLON) return;

        switch (parser.current.type) {
            case TOKEN_CLASS:
            case TOKEN_FUN:
            case TOKEN_VAR:
            case TOKEN_FOR:
            case TOKEN_IF:
            case TOKEN_WHILE:
            case TOKEN_PRINT:
            case TOKEN_RETURN:
                return;

            default:
                // Do nothing.
                ;
        }

        advance();
    }
}

static void declaration() {
    if (match(TOKEN_VAR)) {
        varDeclaration();
    } else {
        statement();
    }

    if (parser.panicMode) {
        synchronize();
    }
}

static void statement() {
    if (match(TOKEN_PRINT)) {
        printStatement();
    } else if (match(TOKEN_LEFT_BRACE)) {
        beginScope();
        block();
        endScope();
    } else {
        expressionStatement();
    }
}

static void grouping(bool canAssign) {
    expression();
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
}

static void number(bool canAssign) {
    double value = std::stod(parser.previous.start);
    emitConstant(NUMBER_VAL(value));
}

static void string(bool canAssign) {
    emitConstant(OBJ_VAL(copyString(parser.previous.start + 1, parser.previous.length - 2)));
}

static uint8_t identifierConstant(Token* name) {
    return makeConstant(OBJ_VAL(copyString(name->start, name->length)));
}

static bool identifiersEqual(Token* a, Token* b) {
    return (a->length == b->length) && (memcmp(a->start, b->start, a->length) == 0);
}

/**
 * Returns the index of the locals vector which has the same name as the identifier token. This is
 * the last declared variable with the given identifier.
 *
 * If no variable is found, returns -1 to indicate a global variable.
 */
static int resolveLocal(Compiler* compiler, Token* name) {
    for (int i = compiler->locals.size() - 1; i >= 0; i--) {
        Local* local = &compiler->locals[i];
        if (identifiersEqual(name, &local->name)) {
            // make sure the variable is defined
            if (local->depth == -1) {
                error("Cannot read local variable in its own initializer.");
            }

            return i;
        }
    }

    return -1;
}

static void addLocal(Token name) {
    if (current->locals.size() == UINT8_MAX + 1) {
        error("Too many local variables in function.");
        return;
    }

    Local local = {name, -1};
    current->locals.push_back(local);
}

static void declareVariable() {
    // Global variables are implicitly declared.
    // This is because they are dynamically bound.
    if (current->scopeDepth == 0) return;

    Token* name = &parser.previous;
    // current scope is at the end of the locals vector
    // so iterate backwards
    for (int i = current->locals.size() - 1; i >= 0; i--) {
        Local* local = &current->locals[i];
        if (local->depth != -1 && local->depth < current->scopeDepth) {
            break;
        }

        if (identifiersEqual(name, &local->name)) {
            error("Variable with this name already declared in this scope.");
        }
    }

    addLocal(*name);
}

static void namedVariable(Token name, bool canAssign) {
    int arg = resolveLocal(current, &name);
    uint8_t getOp = (arg != -1) ? OP_GET_LOCAL : OP_GET_GLOBAL;
    uint8_t setOp = (arg != -1) ? OP_SET_LOCAL : OP_SET_GLOBAL;
    if (arg == -1) {
        arg = identifierConstant(&name);
    }

    if (canAssign && match(TOKEN_EQUAL)) {
        expression();
        emitBytes(setOp, (uint8_t)arg);
    } else {
        emitBytes(getOp, (uint8_t)arg);
    }
}

static void variable(bool canAssign) {
    namedVariable(parser.previous, canAssign);
}

static void unary(bool canAssign) {
    TokenType operatorType = parser.previous.type;

    // Compile the operand.
    parsePrecedence(PREC_UNARY);

    // Emit the operator instruction.
    switch (operatorType) {
        case TOKEN_BANG_EQUAL:
            emitBytes(OP_EQUAL, OP_NOT);
            break;
        case TOKEN_EQUAL_EQUAL:
            emitByte(OP_EQUAL);
            break;
        case TOKEN_GREATER:
            emitByte(OP_GREATER);
            break;
        case TOKEN_GREATER_EQUAL:
            emitBytes(OP_LESS, OP_NOT);
            break;
        case TOKEN_LESS:
            emitByte(OP_LESS);
            break;
        case TOKEN_LESS_EQUAL:
            emitBytes(OP_GREATER, OP_NOT);
            break;
        case TOKEN_PLUS:
            emitByte(OP_ADD);
            break;
        case TOKEN_MINUS:
            emitByte(OP_NEGATE);
            break;
        default:
            return;  // Unreachable.
    }
}

static void parsePrecedence(Precedence precedence) {
    advance();
    ParseFn prefixRule = getRule(parser.previous.type)->prefix;
    if (prefixRule == NULL) {
        error("Expect expression.");
        return;
    }

    bool canAssign = precedence <= PREC_ASSIGNMENT;
    prefixRule(canAssign);

    while (precedence <= getRule(parser.current.type)->precedence) {
        advance();
        ParseFn infixRule = getRule(parser.previous.type)->infix;
        infixRule(canAssign);
    }

    if (canAssign && match(TOKEN_EQUAL)) {
        error("Invalid assignment target.");
    }
}

static uint8_t parseVariable(const char* errorMessage) {
    consume(TOKEN_IDENTIFIER, errorMessage);

    declareVariable();
    if (current->scopeDepth > 0) {
        return 0;
    }

    return identifierConstant(&parser.previous);
}

static void markInitialized() {
    current->locals.back().depth = current->scopeDepth;
}

static void defineVariable(uint8_t global) {
    if (current->scopeDepth > 0) {
        markInitialized();
        return;
    }

    emitBytes(OP_DEFINE_GLOBAL, global);
}

ParseRule rules[] = {
    {grouping, NULL, PREC_NONE},      // TOKEN_LEFT_PAREN
    {NULL, NULL, PREC_NONE},          // TOKEN_RIGHT_PAREN
    {NULL, NULL, PREC_NONE},          // TOKEN_LEFT_BRACE
    {NULL, NULL, PREC_NONE},          // TOKEN_RIGHT_BRACE
    {NULL, NULL, PREC_NONE},          // TOKEN_COMMA
    {NULL, NULL, PREC_NONE},          // TOKEN_DOT
    {unary, binary, PREC_TERM},       // TOKEN_MINUS
    {NULL, binary, PREC_TERM},        // TOKEN_PLUS
    {NULL, NULL, PREC_NONE},          // TOKEN_SEMICOLON
    {NULL, binary, PREC_FACTOR},      // TOKEN_SLASH
    {NULL, binary, PREC_FACTOR},      // TOKEN_STAR
    {unary, NULL, PREC_NONE},         // TOKEN_BANG
    {NULL, binary, PREC_EQUALITY},    // TOKEN_BANG_EQUAL
    {NULL, NULL, PREC_NONE},          // TOKEN_EQUAL
    {NULL, binary, PREC_EQUALITY},    // TOKEN_EQUAL_EQUAL
    {NULL, binary, PREC_COMPARISON},  // TOKEN_GREATER
    {NULL, binary, PREC_COMPARISON},  // TOKEN_GREATER_EQUAL
    {NULL, binary, PREC_COMPARISON},  // TOKEN_LESS
    {NULL, binary, PREC_COMPARISON},  // TOKEN_LESS_EQUAL
    {variable, NULL, PREC_NONE},      // TOKEN_IDENTIFIER
    {string, NULL, PREC_NONE},        // TOKEN_STRING
    {number, NULL, PREC_NONE},        // TOKEN_NUMBER
    {NULL, NULL, PREC_NONE},          // TOKEN_AND
    {NULL, NULL, PREC_NONE},          // TOKEN_CLASS
    {NULL, NULL, PREC_NONE},          // TOKEN_ELSE
    {literal, NULL, PREC_NONE},       // TOKEN_FALSE
    {NULL, NULL, PREC_NONE},          // TOKEN_FOR
    {NULL, NULL, PREC_NONE},          // TOKEN_FUN
    {NULL, NULL, PREC_NONE},          // TOKEN_IF
    {literal, NULL, PREC_NONE},       // TOKEN_NIL
    {NULL, NULL, PREC_NONE},          // TOKEN_OR
    {NULL, NULL, PREC_NONE},          // TOKEN_PRINT
    {NULL, NULL, PREC_NONE},          // TOKEN_RETURN
    {NULL, NULL, PREC_NONE},          // TOKEN_SUPER
    {NULL, NULL, PREC_NONE},          // TOKEN_THIS
    {literal, NULL, PREC_NONE},       // TOKEN_TRUE
    {NULL, NULL, PREC_NONE},          // TOKEN_VAR
    {NULL, NULL, PREC_NONE},          // TOKEN_WHILE
    {NULL, NULL, PREC_NONE},          // TOKEN_ERROR
    {NULL, NULL, PREC_NONE},          // TOKEN_EOF
};

static ParseRule* getRule(TokenType type) {
    return &rules[static_cast<unsigned int>(type)];
}

bool compile(std::string source, Chunk* chunk) {
    initScanner(source.c_str());
    Compiler compiler;
    initCompiler(&compiler);
    compilingChunk = chunk;

    parser.hadError = false;
    parser.panicMode = false;

    advance();

    while (!match(TOKEN_EOF)) {
        declaration();
    }

    endCompiler();
    return !parser.hadError;
}
