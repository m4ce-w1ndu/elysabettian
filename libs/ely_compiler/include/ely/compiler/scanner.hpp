#pragma once

#include "token.hpp"
#include <string_view>
#include <cstdint>

namespace ely::compiler {

/**
 * Zero-allocation lexical scanner that tokenizes Elysabettian source code.
 */
class Scanner {
public:
    /**
     * Constructs a new scanner starting from fresh source code.
     *
     * @param source Non-owning string view over the source text.
     */
    explicit constexpr Scanner(std::string_view source)
        : m_source(source), m_start(0), m_current(0), m_line(1) {}

    /**
     * Scans and returns the next token from the source code.
     *
     * @return Next valid Token or Eof/Error token.
     */
    [[nodiscard]] Token scan_token();
private:
    /**
     * Returns true if the scanner has reached the end of the source string.
     *
     * @return True if at end of source, false otherwise.
     */
    [[nodiscard]] bool is_at_end() const noexcept;

    /**
     * Consumes and returns the current character, advancing the scanner position.
     *
     * @return Consumed character.
     */
    [[nodiscard]] char advance() noexcept;

    /**
     * Looks at the current character without consuming it.
     *
     * @return Current character or '\0' if at end of source.
     */
    [[nodiscard]] char peek() const noexcept;

    /**
     * Looks ahead at the character following the current position without consuming it.
     *
     * @return Next character or '\0' if at end of source.
     */
    [[nodiscard]] char peek_next() const noexcept;

    /**
     * Conditionally consumes the current character if it matches the expected character.
     *
     * @param expected Character to test against.
     * @return True if matched and consumed, false otherwise.
     */
    [[nodiscard]] bool match_char(char expected) noexcept;

    /**
     * Emits a token corresponding to the current scanned lexeme slice.
     *
     * @param token_type Token category to emit.
     * @return Newly formed Token.
     */
    [[nodiscard]] Token make_token(TokenType token_type) const noexcept;

    /**
     * Emits an error token carrying a static error message string.
     *
     * @param message Error description string.
     * @return Token with TokenType::Error containing the message.
     */
    [[nodiscard]] Token error_token(std::string_view message) const noexcept;

    /**
     * Skips whitespace characters and single-line comments (`//`), tracking newline increments.
     */
    void skip_whitespace() noexcept;

    /**
     * Scans a quoted string literal (single or double quotes), supporting multiline strings.
     *
     * @param quote Opening quote character ('\'' or '"').
     * @return Token with TokenType::String or TokenType::Error if unterminated.
     */
    [[nodiscard]] Token string(char quote);

    /**
     * Scans a numeric literal (integer or floating-point with fractional part).
     *
     * @return Token with TokenType::Number.
     */
    [[nodiscard]] Token number();

    /**
     * Scans an identifier or keyword token.
     *
     * @return Token with corresponding keyword or identifier TokenType.
     */
    [[nodiscard]] Token identifier();

    /**
     * Identifies whether the current identifier slice matches a reserved language keyword.
     *
     * @return Resolved TokenType (keyword or Identifier).
     */
    [[nodiscard]] TokenType identifier_type() const noexcept;

    /// View over the input source code.
    std::string_view m_source{};
    /// Scanner's starting position of the current token.
    size_t m_start{0};
    /// Scanner's current reading position.
    size_t m_current{0};
    /// Scanner's current 1-based line number.
    uint32_t m_line{1};
};

} // namespace ely::compiler
