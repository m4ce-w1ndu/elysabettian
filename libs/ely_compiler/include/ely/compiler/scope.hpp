#pragma once

#include "token.hpp"
#include <cstddef>
#include <cstdint>
#include <vector>

namespace ely::compiler {

/**
 * Local variable representation tracked on the virtual machine stack frame.
 */
struct Local {
    /// Identifier token of this local variable.
    Token name;
    /// Scope depth where this variable was declared (-1 if uninitialized).
    int32_t depth{-1};
    /// Whether this local variable has been closed over as an upvalue.
    bool is_captured{false};
};

/**
 * Upvalue descriptor tracking closed-over variables across scopes.
 */
struct Upvalue {
    /// Stack slot index (if is_local) or enclosing upvalue index.
    uint8_t index{0};
    /// True if capturing a local in the immediately enclosing scope, false if transitive.
    bool is_local{false};
};

/**
 * Classifies the type of function or script currently being compiled.
 */
enum class FunctionType : uint8_t {
    /// Top-level script block.
    Script,
    /// Standard named or anonymous function.
    Function,
    /// Class member method (binds slot 0 to 'this').
    Method,
    /// Class constructor/initializer (returns 'this').
    Initializer
};

/**
 * Manages lexical scope depth, local variable allocation, and upvalue tracking.
 */
class CompilerContext {
public:
    /**
     * Constructs a new CompilerContext for the given function type.
     *
     * Automatically claims slot 0 for 'this' (methods/initializers) or function name.
     *
     * @param type Function category being compiled.
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
     * Increments lexical block nesting depth.
     */
    void begin_scope() noexcept { m_scope_depth++; }

    /**
     * Decrements scope depth and pops all out-of-scope locals from tracking.
     *
     * @return Number of local variables removed from the frame.
     */
    [[nodiscard]] size_t pop_scope() noexcept;

    /**
     * Returns mutable reference to local variables list.
     *
     * @return Vector of active and declared Local descriptors.
     */
    [[nodiscard]] std::vector<Local>& locals() noexcept { return m_locals; }

    /**
     * Returns mutable reference to upvalues list.
     *
     * @return Vector of captured Upvalue descriptors.
     */
    [[nodiscard]] std::vector<Upvalue>& upvalues() noexcept { return m_upvalues; }

    /**
     * Returns read-only reference to local variables list.
     *
     * @return Vector of active and declared Local descriptors.
     */
    [[nodiscard]] const std::vector<Local>& locals() const noexcept { return m_locals; }

    /**
     * Returns read-only reference to upvalues list.
     *
     * @return Vector of captured Upvalue descriptors.
     */
    [[nodiscard]] const std::vector<Upvalue>& upvalues() const noexcept { return m_upvalues; }

    /**
     * Returns the kind of function managed by this context.
     *
     * @return FunctionType enum value.
     */
    [[nodiscard]] constexpr FunctionType function_type() const noexcept { return m_function_type; }

    /**
     * Sets a new function kind for this context.
     *
     * @param type New function kind to set.
     */
    constexpr void set_function_type(FunctionType type) noexcept { m_function_type = type; }

    /**
     * Returns the current lexical scope nesting depth.
     *
     * @return 0 for top-level, >0 for block scopes.
     */
    [[nodiscard]] constexpr uint32_t scope_depth() const noexcept { return m_scope_depth; }

    /**
     * Sets a new scope depth for this context.
     *
     * @param scope_depth The new scope depth for this param.
     */
    constexpr void set_scope_depth(uint32_t scope_depth) noexcept { m_scope_depth = scope_depth; }
private:
    /// Tracks all local variables in the current call frame.
    std::vector<Local> m_locals{};
    /// Tracks upvalues captured from enclosing scopes.
    std::vector<Upvalue> m_upvalues{};
    /// Current block scope nesting level.
    uint32_t m_scope_depth{0};
    /// Current function kind.
    FunctionType m_function_type{FunctionType::Script};
};

} // namespace ely::compiler
