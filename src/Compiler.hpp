#ifndef COMPILER_HPP
#define COMPILER_HPP

#include "Tokenizer.hpp"
#include "Value.hpp"

#include <iostream>
#include <memory>
#include <optional>

enum class Precedence {
    NONE,
    ASSIGNMENT,  // =
    OR,          // or
    AND,         // and
    EQUALITY,    // == !=
    COMPARISON,  // < > <= >=
    TERM,        // + -
    FACTOR,      // * /
    UNARY,       // ! - +
    CALL,        // . () []
    PRIMARY
};

class Parser;

typedef void (Parser::*ParseFn)(bool can_assign);

struct ParseRule {
    std::function<void(bool)> prefix;
    std::function<void(bool)> infix;
    Precedence precedence;
};

struct Local {
    std::string name;
    int depth;
    bool is_captured;
    Local(std::string name, int depth): name(name), depth(depth), is_captured(false) {};
};

class Upvalue {
public:
    uint8_t index;
    bool is_local;
    explicit Upvalue(uint8_t index, bool is_local)
        : index(index), is_local(is_local) {}
};

class Parser;

typedef enum {
    TYPE_FUNCTION, TYPE_INITIALIZER, TYPE_METHOD, TYPE_SCRIPT
} FunctionType;

class Compiler {
    Parser* parser;

    FunctionType type;
    Func function;

    std::unique_ptr<Compiler> enclosing;
    
    std::vector<Local> locals;
    std::vector<Upvalue> upvalues;
    int scope_depth = 0;

public:
    explicit Compiler(Parser* parser, FunctionType type, std::unique_ptr<Compiler> enclosing);
    void add_local(const std::string& name);
    void declare_variable(const std::string& name);
    void mark_initialized();
    int resolve_local(const std::string& name);
    int resolve_upvalue(const std::string& name);
    int add_upvalue(uint8_t index, bool is_local);
    void begin_scope();
    void end_scope();
    bool is_local();

    friend Parser;
};

class ClassCompiler {
    std::unique_ptr<ClassCompiler> enclosing;
    bool has_superclass;
public:
    explicit ClassCompiler(std::unique_ptr<ClassCompiler> enclosing);
    friend Parser;
};

class Parser {
    Token previous;
    Token current;
    Tokenizer scanner;
    std::unique_ptr<Compiler> compiler;
    std::unique_ptr<ClassCompiler> class_compiler;
    
    bool had_error;
    bool panic_mode;
    
    void advance();
    void consume(TokenType type, const std::string& message);
    bool check(TokenType type);
    bool match(TokenType type);
    
    void emit(uint8_t byte);
    void emit(OpCode op);
    void emit(OpCode op, uint8_t byte);
    void emit(OpCode op1, OpCode op2);
    void emit_loop(int loopStart);
    int emit_jump(OpCode op);
    void emit_return();
    uint8_t make_constant(Value value);
    void emit_constant(Value value);
    void patch_jump(int offset);
    
    Func end_compiler();
    
    void binary(bool can_assign);
    void call(bool can_assign);
    void dot(bool can_assign);
    void literal(bool can_assign);
    void grouping(bool can_assign);
    void number(bool can_assign);
    void or_(bool can_assign);
    void string_(bool can_assign);
    void named_variable(const std::string& name, bool can_assign);
    void variable(bool can_assign);
    void super(bool can_assign);
    void this_(bool can_assign);
    void and_(bool can_assign);
    void unary(bool can_assign);
    ParseRule& get_rule(TokenType type);
    void parse_precedence(Precedence precedence);
    int identifier_constant(const std::string& name);
    uint8_t parse_variable(const std::string& errorMessage);
    void define_variable(uint8_t global);
    uint8_t args_list();
    void expression();
    void block();
    void function(FunctionType type);
    void method();
    void class_declaration();
    void func_declaration();
    void var_declaration();
    void expression_statement();
    void for_statement();
    void if_statement();
    void declaration();
    void statement();
    void print_statement();
    void return_statement();
    void while_statement();
    void sync();

    void error_at(const Token& token, const std::string& message);
    
    void error(const std::string& message)
    {
        error_at(previous, message);
    };
    
    void error_at_current(const std::string& message)
    {
        error_at(current, message);
    };
    
    friend Compiler;
    
public:
    Parser(const std::string& source);
    Chunk& CurrentChunk()
    {
        return compiler->function->get_chunk();
    }
    std::optional<Func> Compile();
};

#endif /* compiler_hpp */
