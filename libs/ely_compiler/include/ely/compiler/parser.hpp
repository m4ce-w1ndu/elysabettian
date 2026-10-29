#pragma once

#include "scanner.hpp"
#include "token.hpp"
#include <cstdint>

namespace ely::compiler {

/**
 * Expression precedence levels ordered from lowest to highest for Pratt parsing.
 */
enum class Precedence : uint8_t {
    None = 0,
    Assignment,
    Or,
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
     * Constructs a new parser with its associated Scanner.
     */
    constexpr explicit Parser(Scanner scanner) noexcept
        : m_scanner(scanner) {}

    /**
     * Emits a syntax error at the given token location unless currently in panic mode.
     */
    void error_at(Token token, std::string_view message) noexcept;

    /**
     * Emits a syntax error at the previous token position.
     */
    void error(std::string_view message) noexcept {
        error_at(m_previous, message);
    }

    /**
     * Emits a syntax error at the current lookahead token position.
     */
    void error_at_current(std::string_view message) noexcept {
        error_at(m_current, message);
    }

    /**
     * Advances the parser by consuming the next token from the scanner.
     */
    void advance() noexcept;

    /**
     * Consumes the current token if it matches `expected`, otherwise emits a syntax error.
     */
    void consume(TokenType expected, std::string_view message) noexcept;

    /**
     * Checks whether the current token matches the given type without consuming it.
     */
    [[nodiscard]] bool check(TokenType expected) const noexcept {
        return m_current.token_type == expected;
    }

    /**
     * Consumes the current token if it matches `expected`, returning whether it matched.
     */
    [[nodiscard]] bool match_token(TokenType expected) noexcept;

    /**
     * Returns true if any syntax error was recorded.
     */
    [[nodiscard]] constexpr bool had_error() const noexcept { return m_had_error; }

    /**
     * Returns true if the parser is in panic mode.
     */
    [[nodiscard]] constexpr bool panic_mode() const noexcept { return m_panic_mode; }

    /**
     * Returns the last token seen in the stream.
     */
    [[nodiscard]] constexpr Token previous() const noexcept { return m_previous; }

    /**
     * Returns the token at current stream position.
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
    /// Parser panic mode (error state + invalid states).
    bool m_panic_mode{false};
};

}
