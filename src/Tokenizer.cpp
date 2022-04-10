//
//  Tokenizer.cpp
//  CElysabettian
//
//  Created by Simone Rolando on 11/07/21.
//

#include "Tokenizer.hpp"

#include <cctype>

Token Tokenizer::scan_token()
{
    skip_whitespace();
    
    start = current;
    
    if (is_at_end()) return make_token(TokenType::_EOF);
    
    auto c = advance();
    
    if (isdigit(c)) return number();
    if (isalpha(c)) return identifier();
    
    switch (c) {
        case '(': return make_token(TokenType::OPEN_PAREN);
        case ')': return make_token(TokenType::CLOSE_PAREN);
        case '{': return make_token(TokenType::OPEN_CURLY);
        case '}': return make_token(TokenType::CLOSE_CURLY);
        case ';': return make_token(TokenType::SEMICOLON);
        case ',': return make_token(TokenType::COMMA);
        case '.': return make_token(TokenType::DOT);
        case '-': return make_token(TokenType::MINUS);
        case '+': return make_token(TokenType::PLUS);
        case '/': return make_token(TokenType::SLASH);
        case '*': return make_token(TokenType::STAR);
        case '&': return make_token(match('&') ? TokenType::AND : TokenType::BW_AND);
        case '|': return make_token(match('|') ? TokenType::OR : TokenType::BW_OR);
        case '!':
            return make_token(match('=') ? TokenType::EXCL_EQUAL : TokenType::EXCL);
        case '=':
            return make_token(match('=') ? TokenType::EQUAL_EQUAL : TokenType::EQUAL);
        case '<':
            return make_token(match('=') ? TokenType::LESS_EQUAL : TokenType::LESS);
        case '>':
            return make_token(match('=') ?
                             TokenType::GREATER_EQUAL : TokenType::GREATER);
            
        case '"': return string_();
        case '\'': return string_('\'');
    }
    
    return error_token("Unexpected character.");
}

bool Tokenizer::is_at_end()
{
    return current == source.length();
}

char Tokenizer::advance()
{
    current++;
    return source[current - 1];
}

char Tokenizer::peek()
{
    if (is_at_end()) return '\0';
    return source[current];
}

char Tokenizer::peek_next()
{
    if ((current + 1) >= source.length()) return '\0';
    return source[current + 1];
}

bool Tokenizer::match(char expected)
{
    if (is_at_end()) return false;
    if (source[current] != expected) return false;
    
    current++;
    return true;
}

Token Tokenizer::make_token(TokenType type)
{
    auto text = std::string_view(&source[start], current - start);
    return Token(type, text, line);
}

Token Tokenizer::error_token(const std::string& message)
{
    return Token(TokenType::ERROR, message, line);
}

void Tokenizer::skip_whitespace()
{
    while (true) {
        auto c = peek();
        switch (c) {
            case ' ':
            case '\r':
            case '\t':
                advance();
                break;
                
            case '\n':
                line++;
                advance();
                break;
                
            case '/':
                if (peek_next() == '/') {
                    while (peek() != '\n' && !is_at_end()) advance();
                } else {
                    return;
                }
                break;
                
            default:
                return;
        }
    }
}

TokenType Tokenizer::check_keyword(size_t pos, size_t len, std::string rest, TokenType type)
{
    if (current - start == pos + len && source.compare(start + pos, len, rest) == 0) {
        return type;
    }
    
    return TokenType::IDENTIFIER;
}

TokenType Tokenizer::identifier_type()
{
    switch (source[start]) {
        case 'c': return check_keyword(1, 4, "lass", TokenType::CLASS);
        case 'e': return check_keyword(1, 3, "lse", TokenType::ELSE);
        case 'f':
            if (current - start > 1) {
                switch (source[start + 1]) {
                    case 'a': return check_keyword(2, 3, "lse", TokenType::FALSE);
                    case 'o': return check_keyword(2, 1, "r", TokenType::FOR);
                    case 'u': return check_keyword(2, 2, "nc", TokenType::FUN);
                }
            }
            break;
        case 'i': return check_keyword(1, 1, "f", TokenType::IF);
        case 'n': return check_keyword(1, 3, "ull", TokenType::NULLVAL);
        case 'p': return check_keyword(1, 4, "rint", TokenType::PRINT);
        case 'r': return check_keyword(1, 5, "eturn", TokenType::RETURN);
        case 's': return check_keyword(1, 4, "uper", TokenType::SUPER);
        case 't':
            if (current - start > 1) {
                switch (source[start + 1]) {
                    case 'h': return check_keyword(2, 2, "is", TokenType::THIS);
                    case 'r': return check_keyword(2, 2, "ue", TokenType::TRUE);
                }
            }
            break;
        case 'v': return check_keyword(1, 2, "ar", TokenType::VAR);
        case 'w': return check_keyword(1, 4, "hile", TokenType::WHILE);
    }
    
    return TokenType::IDENTIFIER;
}

Token Tokenizer::identifier()
{
    while (isalnum(peek())) advance();
    
    return make_token(identifier_type());
}

Token Tokenizer::number()
{
    while (isdigit(peek())) advance();
    
    if (peek() == '.' && isdigit(peek_next())) {
        advance();
        while (isdigit(peek())) advance();
    }
    
    return make_token(TokenType::NUMBER);
}

Token Tokenizer::string_(const char open_char)
{
    while (peek() != open_char && !is_at_end()) {
        if (peek() == '\n') line++;
        advance();
    }
    
    if (is_at_end()) return error_token("Unterminated string.");
    
    // The closing ".
    advance();
    return make_token(TokenType::STRING);
}
