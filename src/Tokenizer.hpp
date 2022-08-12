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
    BW_AND, BW_OR, BW_XOR, BW_NOT,

    // Array index notation
    OPEN_SQUARE, CLOSE_SQUARE,
};

class Token {
    TokenType type;
    std::string_view text;
    int line;
    
public:
    Token(TokenType type, std::string_view text, int line):
        type(type), text(text), line(line) {};
    
    TokenType get_type() const { return type; };
    std::string_view get_text() const { return text; };
    int get_line() const { return line; };
};

class Tokenizer {
    
    const std::string::size_type start_default { 0u };
    const std::string::size_type current_default { 0u };
    const int line_default { 1u };

    std::string source;
    std::string::size_type start;
    std::string::size_type current;
    int line;
    
    bool is_at_end() const;
    char advance();
    char peek();
    char peek_next();
    bool match(char expected);
    
    Token make_token(TokenType type);
    Token error_token(const std::string& message) const;
    
    void skip_whitespace();
    TokenType check_keyword(size_t pos, size_t len, const std::string_view& rest, TokenType type) const;
    TokenType identifier_type();
    Token identifier();
    Token number();
    Token string_(const char open_char = '"');
    
public:
    explicit Tokenizer(const std::string& source):
        source{source},
        start{start_default},
        current{current_default},
        line{line_default} {};
    
    Token scan_token();
};

#endif /* scanner_hpp */