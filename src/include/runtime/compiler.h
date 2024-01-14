#ifndef ELY_RT_COMPILER_H
#define ELY_RT_COMPILER_H

// Elysabettian headers
#include "language/tokenizer.h"
#include "runtime/value.h"

// Standard C++ headers
#include <iostream>
#include <memory>
#include <optional>
#include <cstddef>
#include <array>
#include <functional>

/**
 * @brief Describes the types of precedences used during
 * parsing and compilation of Elysabettian source code.
*/
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

/**
 * @brief Fordward declaration of parser.
*/
class Parser;

/**
 * @brief Parsing function type.
*/
using ParseFn = std::function<void(bool)>;

/**
 * @brief Describes the actions that must be performed
 * for each type of precedence.
*/
struct ParseRule {
    std::function<void(bool)> prefix;
    std::function<void(bool)> infix;
    PrecedenceType precedence;
};

/**
 * @brief Holds data to store a local variable.
*/
struct LocalVar {
    const bool false_value = false;
    
    std::string name;
    int depth;
    bool is_captured;
    LocalVar(const std::string& name, int depth)
        : name(name), depth(depth), is_captured{false_value} {};
};

/**
 * @brief Holds data to store any type of upvalue
 * (scoped local value).
*/
class UpvalueVar {
public:
    uint8_t index;
    bool is_local;
    explicit UpvalueVar(uint8_t index, bool is_local)
        : index(index), is_local(is_local) {}
};

/**
 * @brief Describes the different types of functions
 * defined in the semantics of the Elysabettian language.
*/
enum class FunctionType {
    Function, Initializer, Method, Script
};

/**
 * @brief Compiler turns the high-level Elysabettian language into
 * a more manageable bytecode, which is then executed by the language's
 * runtime.
*/
class Compiler {
public:
    /**
     * @brief Create a new compiler with an associated parser.
     * @param parser Parser to associate to this compiler (pointer to valid instance).
     * @param type Type of the function.
     * @param enclosing Enclosing compiler (if not present, nullptr).
    */
    explicit Compiler(Parser* parser, FunctionType type, std::unique_ptr<Compiler> enclosing);

    /**
     * @brief Add a new local value.
     * @param name Name of the local value to add.
    */
    void add_local(const std::string& name);

    /**
     * @brief Declare a new variable.
     * @param name Name of the variable to declare.
    */
    void declare_variable(const std::string& name);

    /**
     * @brief Mark a variable statement as already initialized.
    */
    void mark_initialized();

    /**
     * @brief Resolve (find) a local value by name.
     * @param name Name of the local to find.
     * @return Index of the local value.
    */
    int resolve_local(const std::string_view& name);

    /**
     * @brief Resolve (find) an upvalue by name.
     * @param name Name of the upvalue to find.
     * @return Index of the uvpalue.
    */
    int resolve_upvalue(const std::string& name);

    /**
     * @brief Add a new upvalue.
     * @param index Index of the new upvalue.
     * @param is_local Locality flag of the upvalue.
     * @return Index (offset) of the upvalue added.
    */
    int add_upvalue(uint8_t index, bool is_local);

    /**
     * @brief Begin a new scope.
    */
    void begin_scope();

    /**
     * @brief End the current scope.
    */
    void end_scope();

    /**
     * @brief Check for the locality of this
     * @return 
    */
    bool is_local() const;

    friend Parser;

private:
    /**
     * @brief Default function (does nothing).
    */
    const Func default_function = std::make_shared<FunctionObj>(0, "");

    /**
     * @brief Pointer to a Parser instance.
    */
    Parser* parser;

    /**
     * @brief Function type related with this compilation unit.
    */
    FunctionType type;

    /**
     * @brief Function associated with this compilation unit.
    */
    Func function;

    /**
     * @brief Enclosing (nested) compiler (list of compilers).
    */
    std::unique_ptr<Compiler> enclosing;

    /**
     * @brief List of local variables.
    */
    std::vector<LocalVar> locals;

    /**
     * @brief List of upvalues
    */
    std::vector<UpvalueVar> upvalues;

    /**
     * @brief Depth of the current scope.
    */
    int scope_depth = 0;
};

/**
 * @brief Compilation utility for Elysabettian classes.
 * Complementary addition to the Elysabettian compiler,
 * helps resolving the natural "nesting" that occurs in
 * classes.
*/
class ClassCompiler {
    const bool superclass_default = false;

    std::unique_ptr<ClassCompiler> enclosing;
    bool has_superclass;
public:
    explicit ClassCompiler(std::unique_ptr<ClassCompiler> enclosing);
    friend Parser;
};

/**
 * @brief Parser parses the language and converts it into an abstract
 * syntax representation of statements and expressions, used by the
 * compiler to emit the correct bytecode.
*/
class Parser {
public:
    /**
     * @brief Create a new parser to parse the provided source code.
     * @param source Source code as string.
    */
    explicit Parser(const std::string& source);

    /**
     * @brief Get the current code chunk.
     * @return Reference to valid current code chunk.
    */
    Chunk& current_chunk()
    {
        return compiler->function->get_chunk();
    }

    /**
     * @brief Iterate over all statements and provide a function.
     * @return Compiled function that can be executed by the VM.
    */
    std::optional<Func> compile();

private:
    /**
     * @brief Number of parse rules.
    */
    static constexpr size_t NUM_PARSE_RULES = 48;

    /**
     * @brief Previous token.
    */
    Token previous;

    /**
     * @brief Current token.
    */
    Token current;

    /**
     * @brief Language tokenizer instance.
    */
    Tokenizer scanner;

    /**
     * @brief Language bytecode compiler instance.
    */
    std::unique_ptr<Compiler> compiler;

    /**
     * @brief Language start class compiler instance.
    */
    std::unique_ptr<ClassCompiler> class_compiler;

    /**
     * @brief Parser encountered a parsing error.
    */
    bool had_error;

    /**
     * @brief Parser is in panic mode.
    */
    bool panic_mode;

    /**
     * @brief Advance the parser.
    */
    void advance();

    /**
     * @brief Consume a token.
    */
    void consume(TokenType type, const std::string& message);

    /**
     * @brief Check the token type.
    */
    bool check(TokenType type) const;

    /**
     * @brief Match the token type and advance the parser
    */
    bool match(TokenType type);

    /**
     * @brief Emit a byte.
    */
    void emit(uint8_t byte);

    /**
     * @brief Emit an Opcode.
    */
    void emit(Opcode op);

    /**
     * @brief Emit an opcode with data payload.
    */
    void emit(Opcode op, uint8_t byte);

    /**
     * @brief Emit two opcodes.
    */
    void emit(Opcode op1, Opcode op2);

    /**
     * @brief Emit a loop instruction with start offset.
    */
    void emit_loop(int loop_start);

    /**
     * @brief Emit a jump instruction.
    */
    int emit_jump(Opcode op);

    /**
     * @brief Emit a return instruction.
    */
    void emit_return();

    /**
     * @brief Make a new constant value.
    */
    uint8_t make_constant(const Value& value);

    /**
     * @brief Emit a constant expression or value.
    */
    void emit_constant(const Value& value);

    /**
     * @brief Patch a jump instruction.
    */
    void patch_jump(int offset);

    /**
     * @brief End compiler session.
    */
    Func end_compiler();

    /* Parsing functions for different keywords. */
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
    /* Parsing functions for different keywords. */

    /**
     * @brief Get the parse rule related with this token.
    */
    ParseRule& get_rule(TokenType type);

    /**
     * @brief Parse the expression using explicit precedence.
    */
    void parse_precedence(PrecedenceType precedence);
    
    /**
     * @brief Get constant from identifier.
    */
    int identifier_constant(const std::string& name);

    /**
     * @brief Parse a variable expression.
    */
    uint8_t parse_variable(const std::string& errorMessage);

    /**
     * @brief Define a new variable.
    */
    void define_variable(uint8_t global);

    /**
     * @brief Parse the arguments list.
    */
    uint8_t args_list();

    /**
     * @brief Parse an expression.
    */
    void expression();

    /**
     * @brief Parse a block statement.
    */
    void block();

    /**
     * @brief Parse a function.
    */
    void function(FunctionType type);

    /**
     * @brief Parse a method block statement.
    */
    void method();
    
    /**
     * @brief Parse a class declaration.
    */
    void class_declaration();

    /**
     * @brief Parse a function declaration.
    */
    void func_declaration();

    /**
     * @brief Parse variable declaration.
    */
    void var_declaration();

    /**
     * @brief Parse expression statement.
    */
    void expression_statement();

    /**
     * @brief Parse for statement.
    */
    void for_statement();

    /**
     * @brief Parse if statement.
    */
    void if_statement();

    /**
     * @brief Parse a declaration.
    */
    void declaration();

    /**
     * @brief Parse a statement.
    */
    void statement();

    /**
     * @brief Parse a print statement.
    */
    void print_statement();

    /**
     * @brief Parse a return statement.
    */
    void return_statement();

    /**
     * @brief Parse a while statement.
    */
    void while_statement();

    /**
     * @brief Sync the parser.
    */
    void sync();

    /**
     * @brief Create a parsing error with specific position.
    */
    void error_at(const Token& token, const std::string& message);

    /**
     * @brief Create a parsing error with specific message.
    */
    void error(const std::string& message)
    {
        error_at(previous, message);
    };

    /**
     * @brief Create a parsing error at current position.
    */
    void error_at_current(const std::string& message)
    {
        error_at(current, message);
    };

    /**
     * @brief Build a set of parse rules.
    */
    void build_parse_rules();

    /**
     * @brief Array of parse rules.
    */
    std::array<ParseRule, NUM_PARSE_RULES> rules;

    friend Compiler;
};

#endif
