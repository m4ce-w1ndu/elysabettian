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
     */
    explicit constexpr Scanner(std::string_view source)
        : m_source(source), m_start(0), m_current(0), m_line(1) {}

    /**
     * Scans and returns the next token from the source code.
     */
    [[nodiscard]] Token scan_token();
private:
    /**
     * Returns true if the scanner has reached the end of the source string.
     */
    [[nodiscard]] bool is_at_end() const noexcept;

    /**
     * Consumes and returns the current character, advancing the scanner position.
     */
    [[nodiscard]] char advance() noexcept;

    /**
     * Looks at the current character without consuming it.
     */
    [[nodiscard]] char peek() const noexcept;

    /**
     * Looks ahead at the character following the current position without consuming it.
     */
    [[nodiscard]] char peek_next() const noexcept;

    /**
     * Conditionally consumes the current character if it matches the expected character.
     */
    [[nodiscard]] bool match_char(char expected) noexcept;

    /**
     * Emits a token corresponding to the current scanned lexeme slice.
     */
    [[nodiscard]] Token make_token(TokenType token_type) const noexcept;

    /**
     * Emits an error token carrying a static error message string.
     */
    [[nodiscard]] Token error_token(std::string_view message) const noexcept;

    /**
     * Skips whitespace characters and single-line comments (`//`), tracking newline increments.
     */
    void skip_whitespace() noexcept;

    /**
     * Scans a quoted string literal (single or double quotes), supporting multiline strings.
     */
    [[nodiscard]] Token string(char quote);

    /**
     * Scans a numeric literal (integer or floating-point with fractional part).
     */
    [[nodiscard]] Token number();

    /**
     * Scans an identifier or keyword token.
     */
    [[nodiscard]] Token identifier();

    /**
     * Identifies whether the current identifier slice matches a reserved language keyword.
     */
    [[nodiscard]] TokenType identifier_type() const noexcept;

    /// View over the input source code.
    std::string_view m_source{};
    /// Scanner's starting position.
    size_t m_start{0};
    /// Scanner's current position.
    size_t m_current{0};
    /// Scanner's current line in source code.
    uint32_t m_line{1};
};

}
