#ifndef COMPILER_HPP
#define COMPILER_HPP

#include "tokenizer.hpp"
#include "value.hpp"

#include <iostream>
#include <memory>
#include <optional>
#include <cstddef>

enum class precedence_t {
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

class parser_t;

using parsefn_t = void (parser_t::*)(bool);

struct parse_rule_t {
    std::function<void(bool)> prefix;
    std::function<void(bool)> infix;
    precedence_t precedence;
};

struct local_t {
    const bool false_value = false;
    
    std::string name;
    int depth;
    bool is_captured;
    local_t(const std::string& name, int depth): name(name), depth(depth), is_captured{false_value} {};
};

class upvalue_t {
public:
    uint8_t index;
    bool is_local;
    explicit upvalue_t(uint8_t index, bool is_local)
        : index(index), is_local(is_local) {}
};

enum class function_type_t {
    TYPE_FUNCTION, TYPE_INITIALIZER, TYPE_METHOD, TYPE_SCRIPT
};

class compiler_t {
    const func_t default_function = std::make_shared<function_obj>(0, "");
    parser_t* parser;

    function_type_t type;
    func_t function;

    std::unique_ptr<compiler_t> enclosing;
    
    std::vector<local_t> locals;
    std::vector<upvalue_t> upvalues;
    int scope_depth = 0;

public:
    explicit compiler_t(parser_t* parser, function_type_t type, std::unique_ptr<compiler_t> enclosing);
    void add_local(const std::string& name);
    void declare_variable(const std::string& name);
    void mark_initialized();
    int resolve_local(const std::string_view& name);
    int resolve_upvalue(const std::string& name);
    int add_upvalue(uint8_t index, bool is_local);
    void begin_scope();
    void end_scope();
    bool is_local() const;

    friend parser_t;
};

class class_compiler_t {
    const bool superclass_default = false;

    std::unique_ptr<class_compiler_t> enclosing;
    bool has_superclass;
public:
    explicit class_compiler_t(std::unique_ptr<class_compiler_t> enclosing);
    friend parser_t;
};

class parser_t {
    const std::nullptr_t null_value = nullptr;
    const bool false_value = false;

    token_t previous;
    token_t current;
    tokenizer_t scanner;
    std::unique_ptr<compiler_t> compiler;
    std::unique_ptr<class_compiler_t> class_compiler;
    
    bool had_error;
    bool panic_mode;
    
    void advance();
    void consume(token_type_t type, const std::string& message);
    bool check(token_type_t type) const;
    bool match(token_type_t type);
    
    void emit(uint8_t byte);
    void emit(opcode_t op);
    void emit(opcode_t op, uint8_t byte);
    void emit(opcode_t op1, opcode_t op2);
    void emit_loop(int loopStart);
    int emit_jump(opcode_t op);
    void emit_return();
    uint8_t make_constant(const value_t& value);
    void emit_constant(const value_t& value);
    void patch_jump(int offset);
    
    func_t end_compiler();
    
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
    parse_rule_t& get_rule(token_type_t type);
    void parse_precedence(precedence_t precedence);
    int identifier_constant(const std::string& name);
    uint8_t parse_variable(const std::string& errorMessage);
    void define_variable(uint8_t global);
    uint8_t args_list();
    void expression();
    void block();
    void function(function_type_t type);
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

    void error_at(const token_t& token, const std::string& message);
    
    void error(const std::string& message)
    {
        error_at(previous, message);
    };
    
    void error_at_current(const std::string& message)
    {
        error_at(current, message);
    };
    
    friend compiler_t;
    
public:
    explicit parser_t(const std::string& source);
    Chunk& CurrentChunk()
    {
        return compiler->function->get_chunk();
    }
    std::optional<func_t> compile();
};

#endif /* compiler_hpp */
