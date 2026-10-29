#include <ely/compiler/parser.hpp>
#include <iostream>
#include <format>

namespace ely::compiler {

void Parser::error_at(Token token, std::string_view message) noexcept {
    if (m_panic_mode) return;

    m_panic_mode = true;
    m_had_error = true;

    std::string_view location = "";
    std::string formatted_loc;

    if (token.token_type == TokenType::Eof) {
        location = " at end";
    } else if (token.token_type != TokenType::Error) {
        formatted_loc = std::format(" at '{}'", token.lexeme);
        location = formatted_loc;
    }

    std::cerr << std::format("[line {}] Error{}: {}\n", token.line, location, message);
}

void Parser::advance() noexcept {
    m_previous = m_current;

    while (true) {
        m_current = m_scanner.scan_token();
        if (m_current.token_type != TokenType::Error) break;
        error_at_current(m_current.lexeme);
    }
}

void Parser::consume(TokenType expected, std::string_view message) noexcept {
    if (m_current.token_type == expected) {
        advance();
        return;
    }

    error_at_current(message);
}

bool Parser::match_token(TokenType expected) noexcept {
    if (!check(expected)) return false;
    advance();
    return true;
}

}
