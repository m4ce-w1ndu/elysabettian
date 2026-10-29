#include "ely/compiler/token.hpp"
#include <ely/compiler/scanner.hpp>
#include <cctype>
#include <string_view>
#include <unordered_map>

namespace ely::compiler {

/// Constant hash map for language keywords conversion to TokenType.
static const std::unordered_map<std::string_view, TokenType> keywords {
    { "class",  TokenType::Class },
    { "else",   TokenType::Else },
    { "false",  TokenType::False },
    { "for",    TokenType::For },
    { "func",   TokenType::Func },
    { "if",     TokenType::If },
    { "import", TokenType::Import },
    { "null",   TokenType::Null },
    { "print",  TokenType::Print },
    { "return", TokenType::Return },
    { "super",  TokenType::Super },
    { "this",   TokenType::This },
    { "true",   TokenType::True },
    { "while",  TokenType::While },
    { "var",    TokenType::Var }
};

Token Scanner::scan_token() {
    // Skip all whitespaces
    skip_whitespace();
    m_start = m_current;

    if (is_at_end()) return make_token(TokenType::Eof);

    const auto c = advance();
    if (std::isalpha(static_cast<unsigned char>(c)) || c == '_') return identifier();
    if (std::isdigit(static_cast<unsigned char>(c))) return number();

    switch (c) {
        case '(': return make_token(TokenType::OpenParen);
        case ')': return make_token(TokenType::CloseParen);
        case '{': return make_token(TokenType::OpenCurly);
        case '}': return make_token(TokenType::CloseCurly);
        case '[': return make_token(TokenType::OpenSquare);
        case ']': return make_token(TokenType::CloseSquare);
        case ';': return make_token(TokenType::Semicolon);
        case ',': return make_token(TokenType::Comma);
        case '.': return make_token(TokenType::Dot);
        case '-': return make_token(
            match_char('-')
                ? TokenType::MinusMinus
                : match_char('=')
                ? TokenType::MinusAssign
                : TokenType::Minus
        );
        case '+': return make_token(
            match_char('+')
                ? TokenType::PlusPlus
                : match_char('=')
                ? TokenType::PlusAssign
                : TokenType::Plus
        );
        case '/': return make_token(
            match_char('=')
                ? TokenType::DivideAssign
                : TokenType::Divide
        );
        case '*': return make_token(
            match_char('=')
                ? TokenType::MultiplyAssign
                : TokenType::Multiply
        );
        case '~': return make_token(TokenType::BwNot);
        case '^': return make_token(TokenType::BwXor);
        case '!': return make_token(
            match_char('=')
                ? TokenType::NonEquality
                : TokenType::Bang
        );
        case '=': return make_token(
            match_char('=')
                ? TokenType::Equality
                : TokenType::Assign
        );
        case '<': return make_token(
            match_char('=')
                ? TokenType::LessThanEqual
                : match_char('<')
                ? TokenType::ShiftLeft
                : TokenType::LessThan
        );
        case '>': return make_token(
            match_char('=')
                ? TokenType::GreaterThanEqual
                : match_char('>')
                ? TokenType::ShiftRight
                : TokenType::GreaterThan
        );
        case '&': return make_token(
            match_char('&')
                ? TokenType::And
                : TokenType::BwAnd
        );
        case '|': return make_token(
            match_char('|')
                ? TokenType::Or
                : TokenType::BwOr
        );
        case '"':
        case '\'': return string(c);
        default: return error_token("Unexpected character.");
    }
}

bool Scanner::is_at_end() const noexcept {
    return m_current >= m_source.size();
}

char Scanner::advance() noexcept {
    const auto index = m_current++;
    return m_source[index];
}

char Scanner::peek() const noexcept {
    if (is_at_end()) return '\0';
    return m_source[m_current];
}

char Scanner::peek_next() const noexcept {
    if (is_at_end() || m_current + 1 >= m_source.size()) return '\0';
    return m_source[m_current + 1];
}

bool Scanner::match_char(char expected) noexcept {
    if (is_at_end()) return false;
    if (m_source[m_current] != expected) return false;

    m_current++;
    return true;
}

Token Scanner::make_token(TokenType token_type) const noexcept {
    return Token(
        token_type,
        m_source.substr(m_start, m_current - m_start),
        m_line
    );
}

Token Scanner::error_token(std::string_view message) const noexcept {
    return Token(
        TokenType::Error,
        message,
        m_line
    );
}

void Scanner::skip_whitespace() noexcept {
    while (true) {
        switch (peek()) {
            case ' ':
            case '\r':
            case '\t':
                (void)advance(); break;
            case '\n':
                (void)advance();
                m_line++;
                break;
            case '/': {
                if (peek_next() != '/') return;
                while (peek() != '\n' && !is_at_end()) {
                    (void)advance();
                }
                break;
            }
            default: return;
        }
    }
}

Token Scanner::string(char quote) {
    while (peek() != quote && !is_at_end()) {
        if (peek() == '\n') m_line++;
        (void)advance();
    }

    if (is_at_end()) return error_token("Unterminated string.");

    (void)advance();

    return make_token(TokenType::String);
}

Token Scanner::number() {
    while (std::isdigit(static_cast<unsigned char>(peek()))) {
        (void)advance();
    }

    if (peek() == '.' && std::isdigit(static_cast<unsigned char>(peek_next()))) {
        (void)advance();

        while (std::isdigit(static_cast<unsigned char>(peek()))) {
            (void)advance();
        }
    }

    return make_token(TokenType::Number);
}

Token Scanner::identifier() {
    while (std::isalnum(static_cast<unsigned char>(peek())) || peek() == '_') {
        (void)advance();
    }

    const auto token_type = identifier_type();
    return make_token(token_type);
}

TokenType Scanner::identifier_type() const noexcept {
    const auto text = m_source.substr(m_start, m_current - m_start);
    const auto it = keywords.find(text);

    if (it != keywords.end()) return it->second;

    return TokenType::Identifier;
}

}
