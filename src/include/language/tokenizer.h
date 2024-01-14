#ifndef ELY_RT_TOKENIZER_H
#define ELY_RT_TOKENIZER_H

// Standard C++ headers.
#include <string>

/**
 * @brief Describes the type of a Token.
*/
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

    // Bitwise shifts
    GreaterGreater, LessLess
};

/**
 * @brief Describes a Token in the Elysabettian language.
*/
class Token {
    /**
     * @brief Type of this token.
    */
    TokenType type;

    /**
     * @brief Text content of this token.
    */
    std::string_view text;

    /**
     * @brief Position in lines of this token.
    */
    int line;
    
public:
    /**
     * @brief Create a new Token.
     * @param type Type of this new token.
     * @param text Text content of this new token.
     * @param line Line number of this new token.
    */
    Token(TokenType type, std::string_view text, int line):
        type(type), text(text), line(line) {};
    
    /**
     * @brief Get the type of this token.
     * @return Type of this token.
    */
    TokenType get_type() const { return type; };

    /**
     * @brief Get the text content of this token.
     * @return Text of this token as std::string_view.
    */
    std::string_view get_text() const { return text; };

    /**
     * @brief Get the line number of this token.
     * @return Line number of this token.
    */
    int get_line() const { return line; };
};

/**
 * @brief Tokenizes the source code for this 
*/
class Tokenizer {
public:
    /**
     * @brief Create a new Tokenizer.
     * @param source Source code to tokenize.
    */
    explicit Tokenizer(const std::string& source)
        : source(source), start(0u), current(0u), line(1) {}
    
    /**
     * @brief Scan the next token.
     * @return Scanned token.
    */
    Token scan_token();

private:
    /**
     * @brief Source code.
    */
    std::string source;

    /**
     * @brief Start of the tokenizer.
    */
    std::string::size_type start;

    /**
     * @brief Current position of the tokenizer.
    */
    std::string::size_type current;

    /**
     * @brief Current line of the tokenizer.
    */
    int line;

    /**
     * @brief Check if the tokenizer reached end of source code.
    */
    bool is_at_end() const;

    /**
     * @brief Advance the tokenizer.
    */
    char advance();

    /**
     * @brief Read current character.
    */
    char peek();

    /**
     * @brief Read next character.
    */
    char peek_next();

    /**
     * @brief Match current character.
    */
    bool match(char expected);

    /**
     * @brief Make a new token.
    */
    Token make_token(TokenType type);

    /**
     * @brief Make an error token.
    */
    Token error_token(const std::string& message) const;

    /**
     * @brief Skip whitespaces in source code.
    */
    void skip_whitespace();

    /**
     * @brief Check for a keyword.
    */
    TokenType check_keyword(size_t pos, size_t len, const std::string_view& rest, TokenType type) const;

    /**
     * @brief Check the type of the indentifier.
    */
    TokenType identifier_type();

    /**
     * @brief Tokenize an identifier.
    */
    Token identifier();

    /**
     * @brief Tokenize a number.
    */
    Token number();

    /**
     * @brief Tokenize a string.
    */
    Token string_(const char open_char = '"');
};

#endif /* scanner_hpp */
