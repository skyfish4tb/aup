#include <cstdio>
#include <cstdlib>
#include <string.h>

#include "lexer.hpp"
#include "chunk.hpp"
#include "object.hpp"
#include "vm.hpp"

using namespace aup;

struct Parser
{
    VM *vm;
    Chunk *compilingChunk;
    Lexer *lexer;
    Source *source;
    struct Compiler *compiler;
    Token current;
    Token previous;
    int subExprs;
    bool hadCall;
    bool hadAssign;
    bool hadError;
    bool panicMode;
};

enum Precedence
{
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
} ;

typedef void (* ParseFn)(Parser *parser, bool canAssign);

struct ParseRule
{
    ParseFn prefix;
    ParseFn infix;
    Precedence precedence;
};

struct Local {
    Token name;
    int depth;
};

enum FunType
{
    TYPE_FUNCTION,
    TYPE_SCRIPT
};

struct Compiler
{
    Compiler *enclosing;
    Fun *function;
    FunType type;
    Local locals[UINT8_COUNT];
    int localCount;
    int scopeDepth;
};

static Chunk *currentChunk(Parser *parser)
{
    return &parser->compiler->function->chunk;
}

static void errorAt(Parser *parser, Token *token, const char *message)
{
    if (parser->panicMode) return;
    parser->panicMode = true;

    int length = token->start - token->currentLine + token->length;
    const char *line = token->currentLine;

    fprintf(stderr, "[%s:%d:%d] Error", parser->source->fname, token->line, token->column);

    if (token->type == TOKEN_EOF) {
        fprintf(stderr, " at end");
    }
    else if (token->type == TOKEN_ERROR) {
        // Nothing.                                                
    }
    else {
        fprintf(stderr, " at '%.*s'", token->length, token->start);
    }

    if (token->type != TOKEN_EOF) {
        fprintf(stderr, ": %s\n", message);
        fprintf(stderr, "  | %.*s\n", length, line);
        fprintf(stderr, "    %*s", length - token->length, "");
        for (int i = 0; i < token->length; i++) fputc('^', stderr);
        
    }

    fprintf(stderr, "\n");
    fflush(stderr);
    parser->hadError = true; 
}

static void error(Parser *parser, const char *message)
{
    errorAt(parser, &parser->previous, message);
}

static void errorAtCurrent(Parser *parser, const char *message)
{
    errorAt(parser, &parser->current, message);
}

static void advance(Parser *parser)
{
    parser->previous = parser->current;

    for (;;) {
        parser->current = parser->lexer->scan();
        if (parser->current.type != TOKEN_ERROR) break;

        errorAtCurrent(parser, parser->current.start);
    }
}

static void consume(Parser *parser, TokenType type, const char* message)
{
    if (parser->current.type == type) {
        advance(parser);
        return;
    }

    errorAtCurrent(parser, message);
}

static bool check(Parser *parser, TokenType type)
{
    return parser->current.type == type;
}

static bool match(Parser *parser, TokenType type)
{
    if (!check(parser, type)) return false;
    advance(parser);
    return true;
}

static void emitByte(Parser *parser, uint8_t byte)
{
    currentChunk(parser)->emit(byte,
        parser->previous.line, parser->previous.column);
}

static void emitBytes(Parser *parser, uint8_t byte1, uint8_t byte2)
{
    emitByte(parser, byte1);
    emitByte(parser, byte2);
}

static void emitNBytes(Parser *parser, void *bytes, size_t size)
{
    const uint8_t *bs = reinterpret_cast<const uint8_t *>(bytes);
    for (size_t i = 0; i < size; i++) {
        emitByte(parser, bytes == NULL ? 0 : bs[i]);
    }
}

static int emitJump(Parser *parser, uint8_t instruction)
{
    emitByte(parser, instruction);
    emitBytes(parser, 0, 0);
    return currentChunk(parser)->code.size() - 2;
}

static void emitReturn(Parser *parser)
{
    emitByte(parser, OP_NIL);
    emitByte(parser, OP_RET);
}

static uint8_t makeConstant(Parser *parser, Value value)
{
    int constant = currentChunk(parser)->constants.push(value, false);
    if (constant > UINT8_MAX) {
        error(parser, "Too many constants in one chunk.");
        return 0;
    }

    return (uint8_t)constant;
}

static void emitSmart(Parser *parser, uint8_t op, int arg)
{
    emitBytes(parser, op, (uint8_t)arg);
}

static void emitConstant(Parser *parser, Value value)
{
    uint8_t constant = makeConstant(parser, value);
    emitSmart(parser, OP_CONST, constant);
}

static void patchJump(Parser *parser, int offset)
{
    // -2 to adjust for the bytecode for the jump offset itself.
    int jump = currentChunk(parser)->code.size() - offset - 2;

    if (jump > UINT16_MAX) {
        error(parser, "Too much code to jump over.");
    }

    currentChunk(parser)->code[offset] = (jump >> 8) & 0xff;
    currentChunk(parser)->code[offset + 1] = jump & 0xff;
}

static void initCompiler(Parser *parser, Compiler *compiler, FunType type)
{
    compiler->enclosing = parser->compiler;
    compiler->function = NULL;
    compiler->type = type;
    compiler->localCount = 0;
    compiler->scopeDepth = 0;
    compiler->function = parser->vm->newFunction(parser->source);

    if (type != TYPE_SCRIPT) {
        compiler->function->name = parser->vm->copyString(parser->previous.start,
            parser->previous.length);
    }

    Local *local = &compiler->locals[compiler->localCount++];
    local->depth = 0;
    local->name.start = "";
    local->name.length = 0;

    parser->compiler = compiler;
}

static Fun *endCompiler(Parser *parser)
{
    emitReturn(parser);
    Fun *function = parser->compiler->function;

#ifdef DEBUG_PRINT_CODE                      
    if (!parser->hadError) {
        //disassembleChunk(currentChunk(parser), "code");
    }
#endif

    parser->compiler = parser->compiler->enclosing;
    return function;
}

static void beginScope(Parser *parser)
{
    Compiler *current = parser->compiler;
    current->scopeDepth++;
}

static void endScope(Parser *parser)
{
    Compiler *current = parser->compiler;
    current->scopeDepth--;

    while (current->localCount > 0 &&
        current->locals[current->localCount - 1].depth >
        current->scopeDepth) {
        emitByte(parser, OP_POP);
        current->localCount--;
    }
}

static void expression(Parser *parser);
static void statement(Parser *parser);
static void declaration(Parser *parser);
static ParseRule *getRule(TokenType type);
static void parsePrecedence(Parser *parser, Precedence precedence);

static uint8_t identifierConstant(Parser *parser, Token *name)
{
    Str *id = parser->vm->copyString(name->start, name->length);
    return makeConstant(parser, Value(id));
}

static bool identifiersEqual(Token *a, Token *b)
{
    if (a->length != b->length) return false;
    return memcmp(a->start, b->start, a->length) == 0;
}

static int resolveLocal(Parser *parser, Compiler *compiler, Token *name)
{
    for (int i = compiler->localCount - 1; i >= 0; i--) {
        Local *local = &compiler->locals[i];
        if (identifiersEqual(name, &local->name)) {
            if (local->depth == -1) {
                error(parser, "Cannot read local variable in its own initializer.");
            }
            return i;
        }
    }

    return -1;
}

static void addLocal(Parser *parser, Token name)
{
    Compiler *current = parser->compiler;

    if (current->localCount == UINT8_COUNT) {
        error(parser, "Too many local variables in function.");
        return;
    }

    Local *local = &current->locals[current->localCount++];
    local->name = name;
    local->depth = -1;
}

static void declareVariable(Parser *parser)
{
    Compiler *current = parser->compiler;

    // Global variables are implicitly declared.
    if (current->scopeDepth == 0) return;

    Token *name = &parser->previous;
    for (int i = current->localCount - 1; i >= 0; i--) {
        Local *local = &current->locals[i];
        if (local->depth != -1 && local->depth < current->scopeDepth) {
            break;
        }

        if (identifiersEqual(name, &local->name)) {
            error(parser, "Variable with this name already declared in this scope.");
        }
    }

    addLocal(parser, *name);
}

static uint8_t parseVariable(Parser *parser, const char *errorMessage)
{
    consume(parser, TOKEN_IDENTIFIER, errorMessage);

    declareVariable(parser);
    if (parser->compiler->scopeDepth > 0) return 0;

    return identifierConstant(parser, &parser->previous);
}

static void markInitialized(Parser *parser)
{
    Compiler *current = parser->compiler;

    if (current->scopeDepth == 0) return;
    current->locals[current->localCount - 1].depth =
        current->scopeDepth;
}

static void defineVariable(Parser *parser, uint8_t global)
{
    if (parser->compiler->scopeDepth > 0) {
        markInitialized(parser);
        return;
    }

    emitSmart(parser, OP_DEF, global);
}

static uint8_t argumentList(Parser *parser)
{
    uint8_t argCount = 0;
    if (!check(parser, TOKEN_RIGHT_PAREN)) {
        do {
            expression(parser);
            argCount++;
            if (argCount == 32) {
                error(parser, "Cannot have more than 32 arguments.");
            }
        } while (match(parser, TOKEN_COMMA));
    }

    consume(parser, TOKEN_RIGHT_PAREN, "Expect ')' after arguments.");
    return argCount;
}

static void and_(Parser *parser, bool canAssign)
{
    int endJump = emitJump(parser, OP_JMPF);

    emitByte(parser, OP_POP);
    parsePrecedence(parser, PREC_AND);

    patchJump(parser, endJump);
}

static void binary(Parser *parser, bool canAssign)
{
    // Remember the operator.                                
    TokenType operatorType = parser->previous.type;

    // Compile the right operand.                            
    ParseRule *rule = getRule(operatorType);
    parsePrecedence(parser, (Precedence)(rule->precedence + 1));

    // Emit the operator instruction.                        
    switch (operatorType) {
        case TOKEN_EQUAL_EQUAL:   emitByte(parser, OP_EQ); break;
        case TOKEN_LESS:          emitByte(parser, OP_LT); break;
        case TOKEN_LESS_EQUAL:    emitByte(parser, OP_LE); break;

        case TOKEN_BANG_EQUAL:    emitBytes(parser, OP_EQ, OP_NOT); break;
        case TOKEN_GREATER:       emitBytes(parser, OP_LE, OP_NOT); break;
        case TOKEN_GREATER_EQUAL: emitBytes(parser, OP_LT, OP_NOT); break;

        case TOKEN_PLUS:          emitByte(parser, OP_ADD); break;
        case TOKEN_MINUS:         emitByte(parser, OP_SUB); break;
        case TOKEN_STAR:          emitByte(parser, OP_MUL); break;
        case TOKEN_SLASH:         emitByte(parser, OP_DIV); break;
        default:
            return; // Unreachable.                              
    }
}

static void call(Parser *parser, bool canAssign)
{
    uint8_t argCount = argumentList(parser);
    emitBytes(parser, OP_CALL, argCount);
}

static void dot(Parser *parser, bool canAssign)
{
    consume(parser, TOKEN_IDENTIFIER, "Expect member name.");
    uint8_t name = identifierConstant(parser, &parser->previous);

    if (canAssign && match(parser, TOKEN_EQUAL)) {
        expression(parser);
        emitBytes(parser, OP_SET, (uint8_t)name);
    }
    else {
        emitBytes(parser, OP_GET, (uint8_t)name);
    }
}

static void index_(Parser *parser, bool canAssign)
{
    expression(parser);
    consume(parser, TOKEN_RIGHT_BRACKET, "Expected closing ']'");

    if (canAssign && match(parser, TOKEN_EQUAL)) {
        expression(parser);
        emitByte(parser, OP_SETI);

        parser->hadAssign = true;
    }
    else {
        emitByte(parser, OP_GETI);
    }
}

static void literal(Parser *parser, bool canAssign)
{
    switch (parser->previous.type) {
        case TOKEN_FALSE:   emitByte(parser, OP_FALSE); break;
        case TOKEN_NIL:     emitByte(parser, OP_NIL); break;
        case TOKEN_TRUE:    emitByte(parser, OP_TRUE); break;
        case TOKEN_FUN:     emitBytes(parser, OP_LD, 0); break;
        default:
            return; // Unreachable.                   
    }
}

static void grouping(Parser *parser, bool canAssign)
{
    expression(parser);
    consume(parser, TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
}

static void number(Parser *parser, bool canAssign)
{
    double n = strtod(parser->previous.start, NULL);
    emitConstant(parser, Value(n));
}

static void string(Parser *parser, bool canAssign)
{
    Str *s = parser->vm->copyString(parser->previous.start + 1, parser->previous.length - 2);
    emitConstant(parser, Value(s));
}

static void map(Parser *parser, bool canAssign)
{
    uint8_t count = 0;

    if (!check(parser, TOKEN_RIGHT_BRACKET)) {
        do {
            expression(parser);
            count++;
        } while (match(parser, TOKEN_COMMA));
    }

    consume(parser, TOKEN_RIGHT_BRACKET, "Expected closing ']'.");
    emitBytes(parser, OP_MAP, count);
}

static void namedVariable(Parser *parser, Token name, bool canAssign)
{
    uint8_t getOp, setOp;
    int arg = resolveLocal(parser, parser->compiler, &name);

    if (arg != -1) {
        getOp = OP_LD;
        setOp = OP_ST;
    }
    else {
        arg = identifierConstant(parser, &name);
        getOp = OP_GLD;
        setOp = OP_GST;
    }

    if (canAssign && match(parser, TOKEN_EQUAL)) {
        expression(parser);
        emitSmart(parser, setOp, arg);

        parser->hadAssign = true;
    }
    else {
        emitSmart(parser, getOp, arg);
    }
}

static void variable(Parser *parser, bool canAssign)
{
    namedVariable(parser, parser->previous, canAssign);
}

static void or_(Parser *parser, bool canAssign)
{
    int elseJump = emitJump(parser, OP_JMPF);
    int endJump = emitJump(parser, OP_JMP);

    patchJump(parser, elseJump);
    emitByte(parser, OP_POP);

    parsePrecedence(parser, PREC_OR);
    patchJump(parser, endJump);
}

static void unary(Parser *parser, bool canAssign)
{
    TokenType operatorType = parser->previous.type;

    // Compile the operand.                        
    parsePrecedence(parser, PREC_UNARY);

    // Emit the operator instruction.              
    switch (operatorType) {
        case TOKEN_BANG:    emitByte(parser, OP_NOT); break;
        case TOKEN_MINUS:   emitByte(parser, OP_NEG); break;
        default:
            return; // Unreachable.                    
    }
}

static ParseRule rules[] = {
    { grouping, call,    PREC_CALL },       // TOKEN_LEFT_PAREN 
    { NULL,     NULL,    PREC_NONE },       // TOKEN_RIGHT_PAREN     
    { map,      index_,  PREC_CALL },       // TOKEN_LEFT_BRACKET
    { NULL,     NULL,    PREC_NONE },       // TOKEN_RIGHT_BRACKET
    { NULL,     NULL,    PREC_NONE },       // TOKEN_LEFT_BRACE
    { NULL,     NULL,    PREC_NONE },       // TOKEN_RIGHT_BRACE

    { NULL,     NULL,    PREC_NONE },       // TOKEN_COMMA           
    { NULL,     dot,     PREC_CALL },       // TOKEN_DOT

    { unary,    binary,  PREC_TERM },       // TOKEN_MINUS           
    { NULL,     binary,  PREC_TERM },       // TOKEN_PLUS            
    { NULL,     NULL,    PREC_NONE },       // TOKEN_SEMICOLON       
    { NULL,     binary,  PREC_FACTOR },     // TOKEN_SLASH           
    { NULL,     binary,  PREC_FACTOR },     // TOKEN_STAR

    { unary,    NULL,    PREC_NONE },       // TOKEN_BANG
    { NULL,     binary,  PREC_EQUALITY },   // TOKEN_BANG_EQUAL
    { NULL,     NULL,    PREC_NONE },       // TOKEN_EQUAL           
    { NULL,     binary,  PREC_EQUALITY },   // TOKEN_EQUAL_EQUAL  
    { NULL,     binary,  PREC_COMPARISON }, // TOKEN_GREATER      
    { NULL,     binary,  PREC_COMPARISON }, // TOKEN_GREATER_EQUAL
    { NULL,     binary,  PREC_COMPARISON }, // TOKEN_LESS         
    { NULL,     binary,  PREC_COMPARISON }, // TOKEN_LESS_EQUAL 

    { variable, NULL,    PREC_NONE },       // TOKEN_IDENTIFIER
    { string,   NULL,    PREC_NONE },       // TOKEN_STRING          
    { number,   NULL,    PREC_NONE },       // TOKEN_NUMBER

    { NULL,     and_,    PREC_AND },        // TOKEN_AND   
    { NULL,     NULL,    PREC_NONE },       // TOKEN_CLASS           
    { NULL,     NULL,    PREC_NONE },       // TOKEN_ELSE            
    { literal,  NULL,    PREC_NONE },       // TOKEN_FALSE           
    { NULL,     NULL,    PREC_NONE },       // TOKEN_FOR             
    { literal,  NULL,    PREC_NONE },       // TOKEN_FUN             
    { NULL,     NULL,    PREC_NONE },       // TOKEN_IF              
    { literal,  NULL,    PREC_NONE },       // TOKEN_NIL             
    { NULL,     or_,     PREC_OR },         // TOKEN_OR   
    { NULL,     NULL,    PREC_NONE },       // TOKEN_PRINT           
    { NULL,     NULL,    PREC_NONE },       // TOKEN_RETURN          
    { NULL,     NULL,    PREC_NONE },       // TOKEN_SUPER           
    { NULL,     NULL,    PREC_NONE },       // TOKEN_THIS            
    { literal,  NULL,    PREC_NONE },       // TOKEN_TRUE            
    { NULL,     NULL,    PREC_NONE },       // TOKEN_VAR             
    { NULL,     NULL,    PREC_NONE },       // TOKEN_WHILE

    { NULL,     NULL,    PREC_NONE },       // TOKEN_ERROR           
    { NULL,     NULL,    PREC_NONE },       // TOKEN_EOF             
};

static void parsePrecedence(Parser *parser, Precedence precedence)
{
    advance(parser);
    ParseFn prefixRule = getRule(parser->previous.type)->prefix;
    if (prefixRule == NULL) {
        error(parser, "Expect expression.");
        return;
    }

    bool canAssign = precedence <= PREC_ASSIGNMENT;
    prefixRule(parser, canAssign);
    parser->subExprs++;

    while (precedence <= getRule(parser->current.type)->precedence) {
        if (parser->current.line > parser->previous.line) break;
        if (check(parser, TOKEN_LEFT_PAREN)) parser->hadCall = true;
        advance(parser);
        ParseFn infixRule = getRule(parser->previous.type)->infix;
        infixRule(parser, canAssign);
    }

    if (canAssign && match(parser, TOKEN_EQUAL)) {
        error(parser, "Invalid assignment target.");
    }
}

static ParseRule *getRule(TokenType type)
{
    return &rules[type];
}

static void expression(Parser *parser)
{
    parsePrecedence(parser, PREC_ASSIGNMENT);
}

static void block(Parser *parser)
{
    while (!check(parser, TOKEN_RIGHT_BRACE) && !check(parser, TOKEN_EOF)) {
        declaration(parser);
    }

    consume(parser, TOKEN_RIGHT_BRACE, "Expect '}' after block.");
}

static void function(Parser *parser, FunType type)
{
    Compiler compiler;
    initCompiler(parser, &compiler, type);
    beginScope(parser);

    // Compile the parameter list.                                
    consume(parser, TOKEN_LEFT_PAREN, "Expect '(' after function name.");
    if (!check(parser, TOKEN_RIGHT_PAREN)) {
        do {
            int arity = ++parser->compiler->function->arity;
            if (arity > 32) {
                errorAtCurrent(parser, "Cannot have more than 32 parameters.");
            }
            uint8_t paramConstant = parseVariable(parser, "Expect parameter name.");
            defineVariable(parser, paramConstant);
        } while (match(parser, TOKEN_COMMA));
    }
    consume(parser, TOKEN_RIGHT_PAREN, "Expect ')' after parameters.");

    // The body.                                                  
    consume(parser, TOKEN_LEFT_BRACE, "Expect '{' before function body.");
    block(parser);

    // Create the function object.                                
    Fun *function = endCompiler(parser);
    uint8_t constant = makeConstant(parser, Value(function));

    emitSmart(parser, OP_CONST, constant);
}

static void funDeclaration(Parser *parser)
{
    uint8_t global = parseVariable(parser, "Expect function name.");
    markInitialized(parser);
    function(parser, TYPE_FUNCTION);
    defineVariable(parser, global);
}

static void varDeclaration(Parser *parser)
{
    uint8_t global = parseVariable(parser, "Expect variable name.");

    if (match(parser, TOKEN_EQUAL)) {
        expression(parser);
    }
    else {
        emitByte(parser, OP_NIL);
    }

    defineVariable(parser, global);
}

static void expressionStatement(Parser *parser)
{
    parser->hadCall = false;
    parser->hadAssign = false;
    parser->subExprs = 0;

    expression(parser);
    emitByte(parser, OP_POP);

    if ((parser->subExprs <= 1) && !parser->hadCall && !parser->hadAssign) {
        error(parser, "Unexpected expression syntax.");
        return;
    }

    match(parser, TOKEN_SEMICOLON);
}

static void ifStatement(Parser *parser)
{
    consume(parser, TOKEN_LEFT_PAREN, "Expect '(' after 'if'.");
    expression(parser);
    consume(parser, TOKEN_RIGHT_PAREN, "Expect ')' after condition.");

    int thenJump = emitJump(parser, OP_JMPF);
    emitByte(parser, OP_POP);
    statement(parser);

    int elseJump = emitJump(parser, OP_JMP);

    patchJump(parser, thenJump);
    emitByte(parser, OP_POP);

    if (match(parser, TOKEN_ELSE)) statement(parser);
    patchJump(parser, elseJump);
}

static void printStatement(Parser *parser)
{
    int count = 0;

    do {
        expression(parser);
        count++;
        if (count > 32) {
            error(parser, "Too many values in 'print' statement.");
            return;
        }
    } while (match(parser, TOKEN_COMMA));

    emitBytes(parser, OP_PRINT, count);
}

static void returnStatement(Parser *parser)
{
    if (parser->compiler->type == TYPE_SCRIPT) {
        error(parser, "Cannot return from top-level code.");
    }

    if (match(parser, TOKEN_SEMICOLON) ||
        check(parser, TOKEN_RIGHT_BRACE)) {
        emitReturn(parser);
    }
    else {
        expression(parser);
        emitByte(parser, OP_RET);
    }
}

static void synchronize(Parser *parser)
{
    parser->panicMode = false;

    while (parser->current.type != TOKEN_EOF) {
        if (parser->previous.type == TOKEN_SEMICOLON) return;

        switch (parser->current.type) {
            case TOKEN_CLASS:
            case TOKEN_FUN:
            case TOKEN_VAR:
            case TOKEN_FOR:
            case TOKEN_IF:
            case TOKEN_WHILE:
            case TOKEN_PRINT:
            case TOKEN_RETURN:
                return;
            default:; // Do nothing.
        }

        advance(parser);
    }
}

static void declaration(Parser *parser)
{
    if (match(parser, TOKEN_FUN)) {
        funDeclaration(parser);
    }
    else if (match(parser, TOKEN_VAR)) {
        varDeclaration(parser);
    }
    else {
        statement(parser);
    }

    if (parser->panicMode) synchronize(parser);
}

static void statement(Parser *parser)
{
    if (match(parser, TOKEN_PRINT)) {
        printStatement(parser);
    }
    else if (match(parser, TOKEN_IF)) {
        ifStatement(parser);
    }
    else if (match(parser, TOKEN_RETURN)) {
        returnStatement(parser);
    }
    else if (match(parser, TOKEN_LEFT_BRACE)) {
        beginScope(parser);
        block(parser);
        endScope(parser);
    }
    else if (match(parser, TOKEN_SEMICOLON)) {
        // Do nothing.
    }
    else {
        expressionStatement(parser);
    }
}

Fun *VM::compile(Source *source)
{
    Lexer lexer(source->buffer);
    Parser parser;
    Compiler compiler;

    parser.vm = this;
    parser.source = source;
    parser.lexer = &lexer;
    parser.compiler = NULL;
    parser.hadError = false;
    parser.panicMode = false;

    initCompiler(&parser, &compiler, TYPE_SCRIPT);
    
    advance(&parser);
    while (!match(&parser, TOKEN_EOF)) {
        declaration(&parser);
    }

    Fun *function = endCompiler(&parser);
    return parser.hadError ? NULL : function;
}
