#ifndef ELY_RT_TOKENIZER_H
#define ELY_RT_TOKENIZER_H

#include <string>

enum class TokenType {
    // Single-character tokens.
    OpenParen, CloseParen,
    OpenCurly, CloseCurly,
    Comma, Dot, Minus, Plus,
    Semicolon, Slash, Star,
    
    // One or two character tokens.
    Excl, ExclEqual,
    Equal, EqualEqual,
    Greater, GreaterEqual,
    Less, LessEqual,
    
    // Literals.
    Identifier, String, Number,
    
    // Keywords.
    And, Class, Else, False,
    Func, For, If, Null, Or,
    Print, Return, Super, This,
    True, Var, While,
    
    Error,
    Eof,
    
    // Bitwise operations
    BwAnd, BwOr, BwXor, BwNot,

    // Array index notation
    OpenSquare, CloseSquare,
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
