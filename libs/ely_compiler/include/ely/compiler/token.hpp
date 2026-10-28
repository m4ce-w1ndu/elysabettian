#pragma once

#include <cstdint>
#include <string_view>

namespace ely::compiler {

/**
 * Represents the classification of lexical tokens
 * recognized by the Elysabettian tokenizer.
 */
enum class TokenType : uint8_t {
    OpenParen,
    CloseParen,
    OpenCurly,
    CloseCurly,
    OpenSquare,
    CloseSquare,

    Comma,
    Dot,
    Semicolon,

    Minus,
    Plus,
    Divide,
    Multiply,
    BwNot,
    BwXor,

    PlusPlus,
    MinusMinus,
    PlusAssign,
    MinusAssign,
    MultiplyAssign,
    DivideAssign,

    Bang,
    NonEquality,
    Assign,
    Equality,

    LessThan,
    LessThanEqual,
    GreaterThan,
    GreaterThanEqual,

    BwAnd,
    And,
    BwOr,
    Or,
    ShiftLeft,
    ShiftRight,

    Identifier,
    String,
    Number,

    If,
    Else,
    While,
    For,
    Return,
    Var,
    Func,
    Class,
    True,
    False,
    Null,
    This,
    Super,
    Import,

    Print,

    Error,
    Eof,
};

/**
 * Represents a lexical token emitted by the scanner, referencing a slice of the source code.
 */
struct Token {
    /// Category of this token.
    TokenType token_type;
    /// Exact textual slice in the original source code.
    std::string_view lexeme;
    /// 1-based source line where the token was encountered.
    uint32_t line;

    /**
     * Constructs a new lexical token.
     */
    constexpr Token(TokenType token_type, std::string_view lexeme, uint32_t line) noexcept
        : token_type(token_type), lexeme(lexeme), line(line) {}

    /**
     * Default constructor.
     */
    constexpr Token() noexcept = default;
};

}
