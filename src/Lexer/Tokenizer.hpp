//
//  Tokenizer.hpp
//  CElysabettian
//
//  Created by Simone Rolando on 11/07/21.
//

#ifndef SCANNER_HPP
#define SCANNER_HPP

#include <string>

enum class TokenType {
    // Single-character tokens.
    OPEN_PAREN, CLOSE_PAREN,
    OPEN_CURLY, CLOSE_CURLY,
    COMMA, DOT, MINUS, PLUS,
    SEMICOLON, SLASH, STAR,
    
    // One or two character tokens.
    EXCL, EXCL_EQUAL,
    EQUAL, EQUAL_EQUAL,
    GREATER, GREATER_EQUAL,
    LESS, LESS_EQUAL,
    
    // Literals.
    IDENTIFIER, STRING, NUMBER,
    
    // Keywords.
    AND, CLASS, ELSE, FALSE,
    FUN, FOR, IF, NULLVAL, OR,
    PRINT, RETURN, SUPER, THIS,
    TRUE, VAR, WHILE,
    
    ERROR,
    _EOF,
    
    // Bitwise operations
    BW_AND, BW_OR, BW_NOT, BW_XOR
};

class Token {
    TokenType type;
    std::string_view text;
    int line;
    
public:
    Token(TokenType type, std::string_view text, int line):
        type(type), text(text), line(line) {};
    
    TokenType GetType() const { return type; };
    std::string_view GetText() const { return text; };
    int GetLine() const { return line; };
};

class Tokenizer {
    
    std::string source;
    std::string::size_type start;
    std::string::size_type current;
    int line;
    
    bool IsAtEnd();
    char Advance();
    char Peek();
    char PeekNext();
    bool Match(char expected);
    
    Token MakeToken(TokenType type);
    Token ErrorToken(const std::string& message);
    
    void SkipWhitespace();
    TokenType CheckKeyword(size_t pos, size_t len, std::string rest, TokenType type);
    TokenType IdentifierType();
    Token Identifier();
    Token Number();
    Token String();
    
public:
    Tokenizer(std::string source):
        source(source),
        start(0),
        current(0),
        line(1) {};
    
    Token ScanToken();
};

#endif /* scanner_hpp */
