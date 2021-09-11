//
//  Tokenizer.cpp
//  CElysabettian
//
//  Created by Simone Rolando on 11/07/21.
//

#include "Tokenizer.hpp"

#include <cctype>

Token Tokenizer::ScanToken()
{
    SkipWhitespace();
    
    start = current;
    
    if (IsAtEnd()) return MakeToken(TokenType::_EOF);
    
    auto c = Advance();
    
    if (isdigit(c)) return Number();
    if (isalpha(c)) return Identifier();
    
    switch (c) {
        case '(': return MakeToken(TokenType::OPEN_PAREN);
        case ')': return MakeToken(TokenType::CLOSE_PAREN);
        case '{': return MakeToken(TokenType::OPEN_CURLY);
        case '}': return MakeToken(TokenType::CLOSE_CURLY);
        case ';': return MakeToken(TokenType::SEMICOLON);
        case ',': return MakeToken(TokenType::COMMA);
        case '.': return MakeToken(TokenType::DOT);
        case '-': return MakeToken(TokenType::MINUS);
        case '+': return MakeToken(TokenType::PLUS);
        case '/': return MakeToken(TokenType::SLASH);
        case '*': return MakeToken(TokenType::STAR);
        case '&': return MakeToken(Match('&') ? TokenType::AND : TokenType::BW_AND);
        case '|': return MakeToken(Match('|') ? TokenType::OR : TokenType::BW_OR);
        case '!':
            return MakeToken(Match('=') ? TokenType::EXCL_EQUAL : TokenType::EXCL);
        case '=':
            return MakeToken(Match('=') ? TokenType::EQUAL_EQUAL : TokenType::EQUAL);
        case '<':
            return MakeToken(Match('=') ? TokenType::LESS_EQUAL : TokenType::LESS);
        case '>':
            return MakeToken(Match('=') ?
                             TokenType::GREATER_EQUAL : TokenType::GREATER);
            
        case '"': return String();
    }
    
    return ErrorToken("Unexpected character.");
}

bool Tokenizer::IsAtEnd()
{
    return current == source.length();
}

char Tokenizer::Advance()
{
    current++;
    return source[current - 1];
}

char Tokenizer::Peek()
{
    if (IsAtEnd()) return '\0';
    return source[current];
}

char Tokenizer::PeekNext()
{
    if ((current + 1) >= source.length()) return '\0';
    return source[current + 1];
}

bool Tokenizer::Match(char expected)
{
    if (IsAtEnd()) return false;
    if (source[current] != expected) return false;
    
    current++;
    return true;
}

Token Tokenizer::MakeToken(TokenType type)
{
    auto text = std::string_view(&source[start], current - start);
    return Token(type, text, line);
}

Token Tokenizer::ErrorToken(const std::string& message)
{
    return Token(TokenType::ERROR, message, line);
}

void Tokenizer::SkipWhitespace()
{
    while (true) {
        auto c = Peek();
        switch (c) {
            case ' ':
            case '\r':
            case '\t':
                Advance();
                break;
                
            case '\n':
                line++;
                Advance();
                break;
                
            case '/':
                if (PeekNext() == '/') {
                    while (Peek() != '\n' && !IsAtEnd()) Advance();
                } else {
                    return;
                }
                break;
                
            default:
                return;
        }
    }
}

TokenType Tokenizer::CheckKeyword(size_t pos, size_t len, std::string rest, TokenType type)
{
    if (current - start == pos + len && source.compare(start + pos, len, rest) == 0) {
        return type;
    }
    
    return TokenType::IDENTIFIER;
}

TokenType Tokenizer::IdentifierType()
{
    switch (source[start]) {
        case 'c': return CheckKeyword(1, 4, "lass", TokenType::CLASS);
        case 'e': return CheckKeyword(1, 3, "lse", TokenType::ELSE);
        case 'f':
            if (current - start > 1) {
                switch (source[start + 1]) {
                    case 'a': return CheckKeyword(2, 3, "lse", TokenType::FALSE);
                    case 'o': return CheckKeyword(2, 1, "r", TokenType::FOR);
                    case 'u': return CheckKeyword(2, 2, "nc", TokenType::FUN);
                }
            }
            break;
        case 'i': return CheckKeyword(1, 1, "f", TokenType::IF);
        case 'n': return CheckKeyword(1, 3, "ull", TokenType::NULLVAL);
        case 'p': return CheckKeyword(1, 4, "rint", TokenType::PRINT);
        case 'r': return CheckKeyword(1, 5, "eturn", TokenType::RETURN);
        case 's': return CheckKeyword(1, 4, "uper", TokenType::SUPER);
        case 't':
            if (current - start > 1) {
                switch (source[start + 1]) {
                    case 'h': return CheckKeyword(2, 2, "is", TokenType::THIS);
                    case 'r': return CheckKeyword(2, 2, "ue", TokenType::TRUE);
                }
            }
            break;
        case 'v': return CheckKeyword(1, 2, "ar", TokenType::VAR);
        case 'w': return CheckKeyword(1, 4, "hile", TokenType::WHILE);
    }
    
    return TokenType::IDENTIFIER;
}

Token Tokenizer::Identifier()
{
    while (isalnum(Peek())) Advance();
    
    return MakeToken(IdentifierType());
}

Token Tokenizer::Number()
{
    while (isdigit(Peek())) Advance();
    
    if (Peek() == '.' && isdigit(PeekNext())) {
        Advance();
        while (isdigit(Peek())) Advance();
    }
    
    return MakeToken(TokenType::NUMBER);
}

Token Tokenizer::String()
{
    while (Peek() != '"' && !IsAtEnd()) {
        if (Peek() == '\n') line++;
        Advance();
    }
    
    if (IsAtEnd()) return ErrorToken("Unterminated string.");
    
    // The closing ".
    Advance();
    return MakeToken(TokenType::STRING);
}
