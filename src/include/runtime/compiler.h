#ifndef ELY_RT_COMPILER_H
#define ELY_RT_COMPILER_H

#include "language/tokenizer.h"
#include "runtime/value.h"

#include <iostream>
#include <memory>
#include <optional>
#include <cstddef>
#include <array>
#include <functional>

enum class PrecedenceType {
    None,
    Assignment,  // =
    Or,          // ||, |
    And,         // &&, &
    Equality,    // == !=
    Comparison,  // < > <= >=
    Term,        // + -
    Factor,      // * /
    Unary,       // ! - +
    Call,        // . () []
    Primary
};

class Parser;

using ParseFn = std::function<void(bool)>;

struct ParseRule {
    std::function<void(bool)> prefix;
    std::function<void(bool)> infix;
    PrecedenceType precedence;
};

struct LocalVar {
    const bool false_value = false;
    
    std::string name;
    int depth;
    bool is_captured;
    LocalVar(const std::string& name, int depth): name(name), depth(depth), is_captured{false_value} {};
};

class UpvalueVar {
public:
    uint8_t index;
    bool is_local;
    explicit UpvalueVar(uint8_t index, bool is_local)
        : index(index), is_local(is_local) {}
};

enum class FunctionType {
    Function, Initializer, Method, Script
};

/**
 * @brief Compiler turns the high-level Elysabettian language into
 * a more manageable bytecode, which is then executed by the language's
 * runtime.
*/
class Compiler {
    const Func default_function = std::make_shared<FunctionObj>(0, "");
    Parser* parser;

    FunctionType type;
    Func function;

    std::unique_ptr<Compiler> enclosing;
    
    std::vector<LocalVar> locals;
    std::vector<UpvalueVar> upvalues;
    int scope_depth = 0;

public:
    explicit Compiler(Parser* parser, FunctionType type, std::unique_ptr<Compiler> enclosing);
    void add_local(const std::string& name);
    void declare_variable(const std::string& name);
    void mark_initialized();
    int resolve_local(const std::string_view& name);
    int resolve_upvalue(const std::string& name);
    int add_upvalue(uint8_t index, bool is_local);
    void begin_scope();
    void end_scope();
    bool is_local() const;

    friend Parser;
};

class class_compiler_t {
    const bool superclass_default = false;

    std::unique_ptr<class_compiler_t> enclosing;
    bool has_superclass;
public:
    explicit class_compiler_t(std::unique_ptr<class_compiler_t> enclosing);
    friend Parser;
};

/**
 * @brief Parser parses the language and converts it into an abstract
 * syntax representation of statements and expressions, used by the
 * compiler to emit the correct bytecode.
*/
class Parser {
    const std::nullptr_t null_value = nullptr;
    const bool false_value = false;

    Token previous;
    Token current;
    Tokenizer scanner;
    std::unique_ptr<Compiler> compiler;
    std::unique_ptr<class_compiler_t> class_compiler;
    
    bool had_error;
    bool panic_mode;
    
    void advance();
    void consume(TokenType type, const std::string& message);
    bool check(TokenType type) const;
    bool match(TokenType type);
    
    void emit(uint8_t byte);
    void emit(Opcode op);
    void emit(Opcode op, uint8_t byte);
    void emit(Opcode op1, Opcode op2);
    void emit_loop(int loopStart);
    int emit_jump(Opcode op);
    void emit_return();
    uint8_t make_constant(const Value& value);
    void emit_constant(const Value& value);
    void patch_jump(int offset);
    
    Func end_compiler();
    
    void array(bool can_assign);
    void array_idx(bool can_assign);
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
    void parse_precedence(PrecedenceType precedence);
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

    void build_parse_rules();

    std::array<ParseRule, 46> rules;
    
    friend Compiler;
    
public:
    explicit Parser(const std::string& source);
    Chunk& CurrentChunk()
    {
        return compiler->function->get_chunk();
    }
    std::optional<Func> compile();
};

#endif
