#pragma once

#include "scanner.hpp"
#include "token.hpp"
#include <cstdint>
#include <string_view>

namespace ely::compiler {

/**
 * Expression precedence levels ordered from lowest to highest for Pratt parsing.
 */
enum class Precedence : uint8_t {
    None = 0,
    Assignment,
    Or,
    And,
    Equality,
    Comparison,
    Term,
    Factor,
    Unary,
    Call,
    Primary
};

/**
 * Returns the immediate higher precedence level.
 *
 * @param current Current precedence level.
 * @return Next higher Precedence, clamped at Precedence::Primary.
 */
[[nodiscard]] constexpr Precedence next(Precedence current) {
    if (current >= Precedence::Primary) return Precedence::Primary;
    return static_cast<Precedence>(static_cast<uint8_t>(current) + 1);
}

/**
 * Maintains the lexical scanner and syntax error state during compilation.
 */
class Parser {
public:
    /**
     * Constructs a new parser wrapping the given Scanner.
     *
     * @param scanner Initialized Scanner instance.
     */
    constexpr explicit Parser(Scanner scanner) noexcept
        : m_scanner(scanner) {}

    /**
     * Emits a syntax error at the given token location unless currently in panic mode.
     *
     * @param token Token where the syntax violation occurred.
     * @param message Descriptive error message.
     */
    void error_at(Token token, std::string_view message) noexcept;

    /**
     * Emits a syntax error at the previous token position.
     *
     * @param message Descriptive error message.
     */
    void error(std::string_view message) noexcept {
        error_at(m_previous, message);
    }

    /**
     * Emits a syntax error at the current lookahead token position.
     *
     * @param message Descriptive error message.
     */
    void error_at_current(std::string_view message) noexcept {
        error_at(m_current, message);
    }

    /**
     * Advances the parser by consuming the next valid token from the scanner.
     */
    void advance() noexcept;

    /**
     * Consumes the current token if it matches `expected`, otherwise emits a syntax error.
     *
     * @param expected Expected token type.
     * @param message Error message to report if the token does not match.
     */
    void consume(TokenType expected, std::string_view message) noexcept;

    /**
     * Checks whether the current token matches the given type without consuming it.
     *
     * @param expected Token type to check.
     * @return True if current token matches expected, false otherwise.
     */
    [[nodiscard]] bool check(TokenType expected) const noexcept {
        return m_current.token_type == expected;
    }

    /**
     * Consumes the current token if it matches `expected`, returning whether it matched.
     *
     * @param expected Token type to match and consume.
     * @return True if matched and advanced, false otherwise.
     */
    [[nodiscard]] bool match_token(TokenType expected) noexcept;

    /**
     * Returns true if any syntax error was recorded during parsing.
     *
     * @return True if errors occurred, false otherwise.
     */
    [[nodiscard]] constexpr bool had_error() const noexcept { return m_had_error; }

    /**
     * Returns true if the parser is currently suppressing cascading errors.
     *
     * @return True if in panic mode, false otherwise.
     */
    [[nodiscard]] constexpr bool panic_mode() const noexcept { return m_panic_mode; }

    /**
     * Returns the most recently consumed token.
     *
     * @return Previous token.
     */
    [[nodiscard]] constexpr Token previous() const noexcept { return m_previous; }

    /**
     * Returns the current lookahead token without consuming it.
     *
     * @return Current token.
     */
    [[nodiscard]] constexpr Token current() const noexcept { return m_current; }

private:
    /// Scanner to extract token stream.
    Scanner m_scanner;
    /// Current position in the token stream.
    Token m_current{};
    /// Last position in the token stream.
    Token m_previous{};
    /// Parser error state.
    bool m_had_error{false};
    /// Parser panic mode (suppresses cascaded error cascades).
    bool m_panic_mode{false};
};

} // namespace ely::compiler
