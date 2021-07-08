//
//  compiler.c
//  Elysabettian
//
//  Created by Simone Rolando on 07/07/21.
//

#include "compiler.h"
#include "common.h"
#include "memory.h"
#include "scanner.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef DEBUG_PRINT_CODE
#include "debug.h"
#endif

typedef struct Parser {
    Token current;
    Token previous;
    bool had_error;
    bool panic_mode;
} Parser;

typedef enum Precedence {
    PREC_NONE, PREC_ASSIGN,
    PREC_OR, PREC_AND,
    PREC_EQUALITY, PREC_COMPARISON,
    PREC_TERM, PREC_FACTOR,
    PREC_UNARY, PREC_CALL,
    PREC_PRIMARY
} Precedence;

typedef void (*ParseFun)(bool can_assign);

typedef struct ParseRule {
    ParseFun prefix;
    ParseFun infix;
    Precedence precedence;
} ParseRule;

typedef struct LocalMember {
    Token name;
    int depth;
    bool is_captured;
} LocalMember;

typedef struct UpValue {
    uint8_t index;
    bool is_local;
} UpValue;

typedef enum FunctionType {
    TYPE_FUNCTION,
    TYPE_INITIALIZER,
    TYPE_METHOD,
    TYPE_SCRIPT
} FunctionType;

typedef struct Compiler {
    struct Compiler* enclosing;
    ObjFunction* function;
    FunctionType type;
    LocalMember locals[UINT8_COUNT];
    int locals_count;
    UpValue upvalues[UINT8_COUNT];
    int scope_depth;
} Compiler;

typedef struct ClassCompiler {
    struct ClassCompiler* enclosing;
    bool has_superclass;
} ClassCompiler;

Parser parser;
Compiler* current = NULL;
ClassCompiler* current_class = NULL;

static Chunk* current_chunk()
{
    return &current->function->chunk;
}

static void error_at(Token* token, const char* message)
{
    if (parser.panic_mode) return;
    parser.panic_mode = true;
    
    fprintf(stderr, "[line %d] Error", token->line);
    
    if (token->type == T_EOF)
        fprintf(stderr, " at end");
    else if (token->type == ERROR) {
        // Nothing for now.
    } else
        fprintf(stderr, " at '%.*s'", token->length, token->start);
    fprintf(stderr, ": %s\n", message);
    parser.had_error = true;
}

static void error(const char* message)
{
    error_at(&parser.previous, message);
}

static void error_at_current(const char* message)
{
    error_at(&parser.current, message);
}

static void advance()
{
    parser.previous = parser.current;
    for (;;) {
        parser.current = scan_token();
        if (parser.current.type != ERROR) break;
        error_at_current(parser.current.start);
    }
}

static void consume(TokenType type, const char* message)
{
    if (parser.current.type == type) {
        advance();
        return;
    }
    error_at_current(message);
}

static bool check(TokenType type)
{
    return parser.current.type == type;
}

static bool match(TokenType type)
{
    if (!check(type)) return false;
    advance();
    return true;
}

static void emit_byte(uint8_t byte)
{
    write_chunk(current_chunk(), byte, parser.previous.line);
}

static void emit_bytes(uint8_t byte1, uint8_t byte2)
{
    emit_byte(byte1);
    emit_byte(byte2);
}

static void emit_loop(int loop_start)
{
    emit_byte(OP_LOOP);
    int offset = current_chunk()->count - loop_start + 2;
    if (offset > UINT16_MAX) error("Loop body too large.");
    
    emit_byte((offset >> 8) & 0xff);
    emit_byte(offset & 0xff);
}

static int emit_jump(uint8_t instruction)
{
    emit_byte(instruction);
    emit_byte(0xff);
    emit_byte(0xff);
    return current_chunk()->count - 2;
}

static void emit_return()
{
    if (current->type == TYPE_INITIALIZER)
        emit_bytes(OP_GET_LOCAL, 0);
    else
        emit_byte(OP_NULL);
    
    emit_byte(OP_RETURN);
}

static uint8_t make_constant(Value value)
{
    int constant = add_constant(current_chunk(), value);
    if (constant > UINT8_MAX) {
        error("Too many constants in one chunk.");
        return 0;
    }
    return (uint8_t)constant;
}

static void emit_constant(Value value)
{
    emit_bytes(OP_CONSTANT, make_constant(value));
}

static void patch_jump(int offset)
{
    int jump = current_chunk()->count - offset - 2;
    if (jump > UINT16_MAX)
        error("Too much code to jump over.");
    
    current_chunk()->code[offset] = (jump >> 8) & 0xff;
    current_chunk()->code[offset + 1] = jump & 0xff;
}

static void init_compiler(Compiler* compiler, FunctionType type)
{
    compiler->enclosing = current;
    compiler->function = NULL;
    compiler->type = type;
    compiler->locals_count = 0;
    compiler->scope_depth = 0;
    compiler->function = new_function();
    current = compiler;
    
    if (type != TYPE_SCRIPT)
        current->function->name = copy_string(parser.previous.start, parser.previous.length);
    
    LocalMember* local = &current->locals[current->locals_count++];
    local->depth = 0;
    local->is_captured = false;
    
    if (type != TYPE_FUNCTION) {
        local->name.start = "this";
        local->name.length = 4;
    } else {
        local->name.start = "";
        local->name.length = 0;
    }
}

static ObjFunction* end_compiler()
{
    emit_return();
    ObjFunction* function = current->function;
    
#ifdef DEBUG_PRINT_CODE
    if (!parser.had_error)
        disassemble_chunk(current_chunk(),
                          function->name != NULL
                          ? function->name->chars : "<script>");
#endif
    
    current = current->enclosing;
    return function;
}

static void begin_scope()
{
    current->scope_depth++;
}

static void end_scope()
{
    current->scope_depth--;
    
    while (current->locals_count > 0 &&
           current->locals[current->locals_count - 1].depth > current->scope_depth) {
        if (current->locals[current->locals_count - 1].is_captured)
            emit_byte(OP_CLOSE_UPVALUE);
        else
            emit_byte(OP_POP);
        current->locals_count--;
    }
}

static void expression(void);
static void statement(void);
static void declaration(void);
static ParseRule* get_rule(TokenType type);
static void parse_precedence(Precedence precedence);

static uint8_t identifier_constant(Token* name)
{
    return make_constant(OBJECT_VAL(copy_string(name->start, name->length)));
}

static bool identifiers_equal(Token* a, Token* b)
{
    if (a->length != b->length) return false;
    return memcmp(a->start, b->start, a->length) == 0;
}

static int resolve_local(Compiler* compiler, Token* name)
{
    int i;
    for (i = compiler->locals_count - 1; i >= 0; --i) {
        LocalMember* local = &compiler->locals[i];
        if (identifiers_equal(name, &local->name)) {
            if (local->depth == -1)
                error("Can't read local variable in its own initializer.");
            return i;
        }
    }
    
    return -1;
}

static int add_upvalue(Compiler* compiler, uint8_t index, bool is_local)
{
    int upvalue_count = compiler->function->upvalue_count;
    int i;
    
    for (i = 0; i < upvalue_count; ++i) {
        UpValue* upvalue = &compiler->upvalues[i];
        if (upvalue->index == index && upvalue->is_local == is_local)
            return i;
    }
    
    if (upvalue_count == UINT8_COUNT) {
        error("Too many closure variables in function.");
        return 0;
    }
    
    compiler->upvalues[upvalue_count].is_local = is_local;
    compiler->upvalues[upvalue_count].index = index;
    return compiler->function->upvalue_count++;
}

static int resolve_upvalue(Compiler* compiler, Token* name)
{
    if (compiler->enclosing == NULL) return -1;
    
    int local = resolve_local(compiler->enclosing, name);
    if (local != -1) {
        compiler->enclosing->locals[local].is_captured = true;
        return add_upvalue(compiler, (uint8_t)local, true);
    }
    
    int upvalue = resolve_upvalue(compiler->enclosing, name);
    if (upvalue != -1)
        return add_upvalue(compiler, (uint8_t)upvalue, false);
    
    return -1;
}

static void add_local(Token name)
{
    if (current->locals_count == UINT8_COUNT) {
        error("Too many local variables in function.");
        return;
    }
    
    LocalMember* local = &current->locals[current->locals_count++];
    local->name = name;
    
    local->depth = 1;
    local->is_captured = false;
}

static void declare_variable()
{
    if (current->scope_depth == 0) return;
    
    Token* name = &parser.previous;
    int i;
    for (i = current->locals_count - 1; i >= 0; --i) {
        LocalMember* local = &current->locals[i];
        if (local->depth != -1 && local->depth < current->scope_depth)
            break;
        
        if (identifiers_equal(name, &local->name))
            error("Already a variable with this name in this scope.");
    }
    
    add_local(*name);
}

static uint8_t parse_variable(const char* error_message)
{
    consume(IDENTIFIER, error_message);
    declare_variable();
    if (current->scope_depth > 0) return 0;
    
    return identifier_constant(&parser.previous);
}

static void mark_initialized()
{
    if (current->scope_depth == 0) return;
    current->locals[current->locals_count - 1].depth = current->scope_depth;
}

static void define_variable(uint8_t global)
{
    if (current->scope_depth > 0) {
        mark_initialized();
        return;
    }
    emit_bytes(OP_DEFINE_GLOBAL, global);
}

static uint8_t argument_list()
{
    uint8_t argc = 0;
    if (!check(CLOSE_PAREN)) {
        do {
            expression();
            if (argc == 255)
                error("Can't have more than 255 arguments.");
            argc++;
        } while (match(COMMA));
    }
    consume(CLOSE_PAREN, "Expected ')' after arguments");
    return argc;
}

static void and(bool can_assign)
{
    int end_jmp = emit_jump(OP_JUMP_IF_FALSE);
    emit_byte(OP_POP);
    parse_precedence(PREC_AND);
    patch_jump(end_jmp);
}

static void binary(bool can_assign)
{
    TokenType operator = parser.previous.type;
    ParseRule* rule = get_rule(operator);
    parse_precedence((Precedence)(rule->precedence + 1));
    
    switch (operator) {
        case NON_EQUALITY:          emit_bytes(OP_EQUAL, OP_NOT); break;
        case EQUALITY:              emit_byte(OP_EQUAL); break;
        case GREATER_THAN:          emit_byte(OP_GREATER); break;
        case GREATER_EQUAL:         emit_bytes(OP_GREATER, OP_NOT); break;
        case LESS_THAN:             emit_byte(OP_LESS); break;
        case LESS_EQUAL:            emit_bytes(OP_LESS, OP_NOT); break;
        case ADD:                   emit_byte(OP_ADD); break;
        case SUB:                   emit_byte(OP_SUB); break;
        case MUL:                   emit_byte(OP_MUL); break;
        case DIV:                   emit_byte(OP_DIV); break;
        case MOD:                   emit_byte(OP_MOD); break;
        case BITWISE_OR:            emit_byte(OP_BITWISE_OR); break;
        case BITWISE_AND:           emit_byte(OP_BITWISE_AND); break;
        case BITWISE_NOT:           emit_byte(OP_BITWISE_NOT); break;
        case BITWISE_XOR:           emit_byte(OP_BITWISE_XOR); break;
        default:                    return;
    }
}

static void call(bool can_assign)
{
    uint8_t argc = argument_list();
    emit_bytes(OP_CALL, argc);
}

static void dot(bool can_assign)
{
    consume(IDENTIFIER, "Expected property name after '.'.");
    uint8_t name = identifier_constant(&parser.previous);
    
    if (can_assign && match(ASSIGN)) {
        expression();
        emit_bytes(OP_SET_PROPERTY, name);
    } else if (match(OPEN_PAREN)) {
        uint8_t argc = argument_list();
        emit_bytes(OP_INVOKE, name);
        emit_byte(argc);
    }
    else
        emit_bytes(OP_GET_PROPERTY, name);
}

static void literal(bool can_assign)
{
    switch (parser.previous.type) {
        case FALSEVAL: emit_byte(OP_FALSE); break;
        case TRUEVAL: emit_byte(OP_TRUE); break;
        case NULLVAL: emit_byte(OP_NULL); break;
        default: return;
    }
}

static void grouping(bool can_assign)
{
    expression();
    consume(CLOSE_PAREN, "Expected ')' after expression.");
}

static void number(bool can_assign)
{
    double value = strtod(parser.previous.start, NULL);
    emit_constant(NUMBER_VAL(value));
}

static void or(bool can_assign)
{
    int else_jump = emit_jump(OP_JUMP_IF_FALSE);
    int end_jump = emit_jump(OP_JUMP);
    
    patch_jump(else_jump);
    emit_byte(OP_POP);
    
    parse_precedence(PREC_OR);
    patch_jump(end_jump);
}

static void string(bool can_assign)
{
    emit_constant(OBJECT_VAL(copy_string(parser.previous.start + 1,
                                         parser.previous.length - 2)));
}

static void named_variable(Token name, bool can_assign)
{
    uint8_t get_op, set_op;
    int arg = resolve_local(current, &name);
    
    if (arg != -1) {
        get_op = OP_GET_LOCAL;
        set_op = OP_SET_LOCAL;
    } else if ((arg = resolve_upvalue(current, &name)) != -1) {
        get_op = OP_GET_UPVALUE;
        set_op = OP_SET_UPVALUE;
    } else {
        arg = identifier_constant(&name);
        get_op = OP_GET_GLOBAL;
        set_op = OP_SET_GLOBAL;
    }
    
    if (can_assign && match(ASSIGN)) {
        expression();
        emit_bytes(set_op, (uint8_t)arg);
    } else {
        emit_bytes(get_op, (uint8_t)arg);
    }
}

static void variable(bool can_assign)
{
    named_variable(parser.previous, can_assign);
}

static Token synthetic_token(const char* text)
{
    Token token;
    token.start = text;
    token.length = (int)strlen(text);
    return token;
}

static void super(bool can_assign)
{
    if (current_class == NULL)
        error("Can't use 'super' attribute outside of a class.");
    else if (!current_class->has_superclass)
        error("Can't use 'super' in a class with no superclass.");
    
    consume(DOT, "Expected '.' after 'super'.");
    consume(IDENTIFIER, "Expect superclass method name");
    uint8_t name = identifier_constant(&parser.previous);
    
    named_variable(synthetic_token("this"), false);
    
    if (match(OPEN_PAREN)) {
        uint8_t argc = argument_list();
        named_variable(synthetic_token("super"), false);
        emit_bytes(OP_SUPER_INVOKE, name);
        emit_byte(argc);
    } else {
        named_variable(synthetic_token("super"), false);
        emit_bytes(OP_GET_SUPER, name);
    }
}

static void this(bool can_assign)
{
    if (current_class == NULL) {
        error("Can't use 'this' outside of a class.");
        return;
    }
    
    variable(false);
}

static void unary(bool can_assign)
{
    TokenType operator = parser.previous.type;
    parse_precedence(PREC_UNARY);
    
    switch (operator) {
        case NOT: emit_byte(OP_NOT); break;
        case SUB: emit_byte(OP_NEGATE); break;
        case BITWISE_NOT: emit_byte(OP_BITWISE_NOT); break;
        default: return;
    }
}

ParseRule rules[] = {
    [OPEN_PAREN] = {grouping, call, PREC_CALL},
    [CLOSE_PAREN] = { NULL, NULL, PREC_NONE },
    [OPEN_CURLY] = { NULL, NULL, PREC_NONE },
    [CLOSE_CURLY] = { NULL, NULL, PREC_NONE},
    [COMMA] = { NULL, NULL, PREC_NONE },
    [DOT] = { NULL, dot, PREC_CALL },
    [SUB] = { unary, binary, PREC_TERM },
    [ADD] = { NULL, binary, PREC_TERM },
    [SEMICOLON] = { NULL, NULL, PREC_NONE },
    [DIV] = { NULL, binary, PREC_FACTOR },
    [MUL] = { NULL, binary, PREC_FACTOR },
    [NOT] = { unary, NULL, PREC_NONE },
    [NON_EQUALITY] = { NULL, binary, PREC_EQUALITY },
    [ASSIGN] = { NULL, binary, PREC_NONE },
    [EQUALITY] = { NULL, binary, PREC_EQUALITY },
    [LESS_THAN] = { NULL, binary, PREC_COMPARISON },
    [LESS_EQUAL] = { NULL, binary, PREC_COMPARISON },
    [GREATER_THAN] = { NULL, binary, PREC_COMPARISON },
    [GREATER_EQUAL] = { NULL, binary, PREC_COMPARISON },
    [IDENTIFIER] = { variable, NULL, PREC_NONE },
    [STRING] = { string, NULL, PREC_NONE },
    [NUMBER] = { number, NULL, PREC_NONE },
    [AND] = { NULL, and, PREC_AND },
    [CLASS] = { NULL, NULL, PREC_NONE },
    [ELSE] = { NULL, NULL, PREC_NONE },
    [FALSEVAL] = { literal, NULL, PREC_NONE },
    [TRUEVAL] = { literal, NULL, PREC_NONE },
    [FOR] = { NULL, NULL, PREC_NONE },
    [FUNC] = { NULL, NULL, PREC_NONE },
    [IF] = { NULL, NULL, PREC_NONE },
    [NULLVAL] = { literal, NULL, PREC_NONE },
    [OR] = { NULL, or, PREC_OR },
    [PRINT] = { NULL, NULL, PREC_NONE },
    [RETURN] = { NULL, NULL, PREC_NONE },
    [SUPER] = { super, NULL, PREC_NONE },
    [THIS] = { this, NULL, PREC_NONE },
    [VAR] = { NULL, NULL, PREC_NONE },
    [WHILE] = { NULL, NULL, PREC_NONE },
    [ERROR] = { NULL, NULL, PREC_NONE },
    [T_EOF] = { NULL, NULL, PREC_NONE }
};

static void parse_precedence(Precedence precedence)
{
    advance();
    ParseFun prefix_rule = get_rule(parser.previous.type)->prefix;
    if (prefix_rule == NULL) {
        error("Expected expression.");
        return;
    }
    
    bool can_assign = precedence <= PREC_ASSIGN;
    prefix_rule(can_assign);
    
    while (precedence <= get_rule(parser.current.type)->precedence) {
        advance();
        ParseFun infix_rule = get_rule(parser.previous.type)->infix;
        
        infix_rule(can_assign);
    }
    
    if (can_assign && match(ASSIGN))
        error("Invalid assignment target.");
}

static ParseRule* get_rule(TokenType type)
{
    return &rules[type];
}

static void expression()
{
    parse_precedence(PREC_ASSIGN);
}

static void block()
{
    while (!check(CLOSE_CURLY) && !check(T_EOF))
        declaration();
    consume(CLOSE_CURLY, "Expected '}' after block.");
}

static void function(FunctionType type)
{
    Compiler compiler;
    init_compiler(&compiler, type);
    begin_scope();
    
    consume(OPEN_PAREN, "Expected '(' after function name.");
    if (!(check(CLOSE_PAREN))) {
        do {
            current->function->arity++;
            if (current->function->arity > 255)
                error_at_current("Can't have more than 255 parameters.");
            uint8_t constant = parse_variable("Expected parameter name.");
            define_variable(constant);
        } while (match(COMMA));
    }
    consume(CLOSE_PAREN, "Expected ')' after parameters.");
    consume(OPEN_CURLY, "Expected '{' before function body.");
    block();
    
    ObjFunction* function = end_compiler();
    emit_bytes(OP_CLOSURE, make_constant(OBJECT_VAL(function)));
    
    int i;
    for (i = 0; i < function->upvalue_count; ++i) {
        emit_byte(compiler.upvalues[i].is_local ? 1 : 0);
        emit_byte(compiler.upvalues[i].index);
    }
}

static void method()
{
    consume(IDENTIFIER, "Expected method name.");
    uint8_t constant = identifier_constant(&parser.previous);
    
    FunctionType type = TYPE_METHOD;
    if (parser.previous.length == 4 && memcmp(parser.previous.start, "init", 4) == 0)
        type = TYPE_INITIALIZER;
    function(type);
    emit_bytes(OP_METHOD, constant);
}

static void class_declaration()
{
    consume(IDENTIFIER, "Expected class name.");
    Token class_name = parser.previous;
    
    uint8_t name_constant = identifier_constant(&parser.previous);
    declare_variable();
    
    emit_bytes(OP_CLASS, name_constant);
    define_variable(name_constant);
    
    ClassCompiler class_compiler;
    class_compiler.has_superclass = false;
    class_compiler.enclosing = current_class;
    current_class = &class_compiler;
    
    if (match(SEMICOLON)) {
        consume(IDENTIFIER, "Expected superclass name.");
        variable(false);
        
        if (identifiers_equal(&class_name, &parser.previous))
            error("A class cannot inherit from itself.");
        
        begin_scope();
        add_local(synthetic_token("super"));
        define_variable(0);
        
        named_variable(class_name, false);
        emit_byte(OP_INHERIT);
        class_compiler.has_superclass = true;
    }
    
    named_variable(class_name, false);
    consume(OPEN_CURLY, "Expected '{' before class body.");
    
    while (!check(CLOSE_CURLY) && !check(T_EOF))
        method();
    
    consume(CLOSE_CURLY, "Expected '}' after class body.");
    emit_byte(OP_POP);
    
    if (class_compiler.has_superclass)
        end_scope();
    current_class = current_class->enclosing;
}

static void func_declaration()
{
    uint8_t global = parse_variable("Expected function name.");
    mark_initialized();
    function(TYPE_FUNCTION);
    define_variable(global);
}

static void var_declaration()
{
    uint8_t global = parse_variable("Expected variable name.");
    if (match(ASSIGN))
        expression();
    else
        emit_byte(OP_NULL);
    consume(SEMICOLON, "Expected ';' after variable declaration.");
    define_variable(global);
}

static void expression_statement()
{
    expression();
    consume(SEMICOLON, "Expected ';' after expression.");
    emit_byte(OP_POP);
}

static void for_statement()
{
    begin_scope();
    consume(OPEN_PAREN, "Expected '(' after 'for'.");
    if (match(SEMICOLON)) {
        // No init;
    } else if (match(VAR)) {
        var_declaration();
    } else {
        expression_statement();
    }
    
    int loop_start = current_chunk()->count;
    int exitjump = -1;
    
    if (!match(SEMICOLON)) {
        expression();
        consume(SEMICOLON, "Expected ';' after loop condition.");
        exitjump = emit_jump(OP_JUMP_IF_FALSE);
        emit_byte(OP_POP);
    }
    
    if (!match(CLOSE_PAREN)) {
        int bodyjump = emit_jump(OP_JUMP);
        int increment = current_chunk()->count;
        expression();
        emit_byte(OP_POP);
        consume(CLOSE_PAREN, "Expected ')' after for clauses.");
        emit_loop(loop_start);
        loop_start = increment;
        patch_jump(bodyjump);
    }
    
    statement();
    emit_loop(loop_start);
    
    if (exitjump != -1) {
        patch_jump(exitjump);
        emit_byte(loop_start);
    }
    
    end_scope();
}

static void if_statement()
{
    consume(OPEN_PAREN, "Expected '(' after 'if'.");
    expression();
    consume(CLOSE_PAREN, "Expected ')' after condition.");
    
    int then_jump = emit_jump(OP_JUMP_IF_FALSE);
    emit_byte(OP_POP);
    statement();
    
    int else_jump = emit_jump(OP_POP);
    patch_jump(then_jump);
    emit_byte(OP_POP);
    
    if (match(ELSE)) statement();
    patch_jump(else_jump);
}

static void print_statement()
{
    expression();
    consume(SEMICOLON, "Expected ';' after value.");
    emit_byte(OP_PRINT);
}

static void return_statement()
{
    if (current->type == TYPE_SCRIPT)
        error("Cannot return from main execution path.");
    
    if (match(SEMICOLON)) {
        emit_return();
    } else {
        if (current->type == TYPE_INITIALIZER)
            error("Cannot return a value from an initializer.");
        expression();
        consume(SEMICOLON, "Expected ';' after return value.");
        emit_byte(OP_RETURN);
    }
}

static void while_statement()
{
    int loop_start = current_chunk()->count;
    consume(OPEN_PAREN, "Expected '(' after 'while'.");
    expression();
    consume(CLOSE_PAREN, "Expected ')' after condition.");
    
    int exit_jump = emit_jump(OP_JUMP_IF_FALSE);
    emit_byte(OP_POP);
    statement();
    
    emit_loop(loop_start);
    patch_jump(exit_jump);
    emit_byte(OP_POP);
}

static void synchronize()
{
    parser.panic_mode = false;
    
    while (parser.current.type != T_EOF) {
        if (parser.previous.type == SEMICOLON) return;
        switch (parser.current.type) {
            case CLASS:
            case FUNC:
            case VAR:
            case FOR:
            case IF:
            case WHILE:
            case PRINT:
            case RETURN:
                return;
            default:;
        }
        
        advance();
    }
}

static void declaration()
{
    if (match(CLASS)) {
        class_declaration();
    } else if(match(FUNC)) {
        func_declaration();
    } else if (match(VAR)) {
        var_declaration();
    } else {
        statement();
    }
    
    if (parser.panic_mode) synchronize();
}

static void statement()
{
    if (match(PRINT)) {
        print_statement();
    } else if (match(FOR)) {
        for_statement();
    } else if (match(IF)) {
        if_statement();
    } else if (match(RETURN)) {
        return_statement();
    } else if (match(WHILE)) {
        while_statement();
    } else if (match(OPEN_CURLY)) {
        begin_scope();
        block();
        end_scope();
    } else {
        expression_statement();
    }
}

ObjFunction* compile(const char* source)
{
    init_scanner(source);
    Compiler compiler;
    
    init_compiler(&compiler, TYPE_SCRIPT);
    
    parser.had_error = false;
    parser.panic_mode = false;
    
    advance();
    
    while (!match(T_EOF)) {
        declaration();
    }
    
    ObjFunction* function = end_compiler();
    return parser.had_error ? NULL : function;
}

void mark_compiler_roots()
{
    Compiler* compiler = current;
    while (compiler != NULL) {
        mark_object((Object*)compiler->function);
        compiler = compiler->enclosing;
    }
}
