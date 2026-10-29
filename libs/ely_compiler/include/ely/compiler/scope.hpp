#pragma once

#include "token.hpp"
#include <cstdint>
#include <vector>

namespace ely::compiler {

/**
 * Local variable (or object) representation.
 */
struct Local {
    /// Name (identifier) of this variable.
    Token name;
    /// Associated scope depth.
    int32_t depth{-1};
    /// Whether this variable is part of a closure.
    bool is_captured{false};
};

/**
 * Upvalue representation on the compiler stack.
 */
struct Upvalue {
    /// Index of this upvalue in the Chunk's values.
    uint8_t index;
    /// Whether this upvalue is local to this scope.
    bool is_local;
};

/**
 * FunctionType identifies the subtype of a specific function.
 */
enum class FunctionType : uint8_t {
    /// The function is a top-level code statement.
    Script,
    /// The function is a standard function declared and defined in the code.
    Function,
    /// The function is a member function of a specific class.
    Method,
    /// The function is a constructor of a specific class.
    Initializer
};

/**
 * Holds the current context (state) of the compiler in regards to the
 * language's lexical scoping.
 */
class CompilerContext {
public:
    /**
     * Constructs a new CompilerContext with ano optional function type.
     */
    explicit CompilerContext(FunctionType type = FunctionType::Script) noexcept
        : m_function_type(type) {

        Token reserved_token{};
        if (type == FunctionType::Method || type == FunctionType::Initializer) {
            reserved_token = Token(TokenType::This, "this", 0);
        }

        m_locals.push_back(Local{reserved_token, 0, false});
    }

    /**
     * Begins a new (inner) scoper.
     */
    void begin_scope() noexcept { m_scope_depth++; }

    /**
     * Ends (returns and cleans) an inner scope.
     */
    [[nodiscard]] size_t pop_scope() noexcept;

    /**
     * Returns all local variables (non-const).
     */
    [[nodiscard]] std::vector<Local>& locals() noexcept { return m_locals; }

    /**
     * Returns all upvalues (non-const).
     */
    [[nodiscard]] std::vector<Upvalue>& upvalues() noexcept { return m_upvalues; }

    /**
     * Returns all local variables (const).
     */
    [[nodiscard]] const std::vector<Local>& locals() const noexcept { return m_locals; }

    /**
     * Returns all upvalues (const).
     */
    [[nodiscard]] const std::vector<Upvalue>& upvalues() const noexcept { return m_upvalues; }

    /**
     * Returns the current function type.
     */
    [[nodiscard]] constexpr FunctionType function_type() const noexcept { return m_function_type; }

    /**
     * Returns the current scope depth.
     */
    [[nodiscard]] constexpr uint32_t scope_depth() const noexcept { return m_scope_depth; }
private:
    /// Holds all variables local to this scope.
    std::vector<Local> m_locals{};
    /// Holds all upvalues related with this scope.
    std::vector<Upvalue> m_upvalues{};
    /// Holds the current scope depth.
    uint32_t m_scope_depth{0};
    /// Holds the current function type.
    FunctionType m_function_type{};
};

}
