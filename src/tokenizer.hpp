#ifndef SCANNER_HPP
#define SCANNER_HPP

#include <string>

enum class token_type_t {
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

class token_t {
    token_type_t type;
    std::string_view text;
    int line;
    
public:
    token_t(token_type_t type, std::string_view text, int line):
        type(type), text(text), line(line) {};
    
    token_type_t get_type() const { return type; };
    std::string_view get_text() const { return text; };
    int get_line() const { return line; };
};

class tokenizer_t {
    
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
    
    token_t make_token(token_type_t type);
    token_t error_token(const std::string& message) const;
    
    void skip_whitespace();
    token_type_t check_keyword(size_t pos, size_t len, const std::string_view& rest, token_type_t type) const;
    token_type_t identifier_type();
    token_t identifier();
    token_t number();
    token_t string_(const char open_char = '"');
    
public:
    explicit tokenizer_t(const std::string& source):
        source{source},
        start{start_default},
        current{current_default},
        line{line_default} {};
    
    token_t scan_token();
};

#endif /* scanner_hpp */
