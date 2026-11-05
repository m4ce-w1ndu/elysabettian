#ifndef ELY_COMPILER_TOKEN_H
#define ELY_COMPILER_TOKEN_H

#include <stddef.h>
#include <stdint.h>

/**
 * Represents the classification of lexical tokens
 * recognized by the Elysabettian tokenizer.
 */
typedef enum ElyTokenType {
    ELY_TT_OPENPAREN,
    ELY_TT_CLOSEPAREN,
    ELY_TT_OPENCURLY,
    ELY_TT_CLOSECURLY,
    ELY_TT_OPENSQUARE,
    ELY_TT_CLOSESQUARE,

    ELY_TT_COMMA,
    ELY_TT_DOT,
    ELY_TT_SEMICOLON,

    ELY_TT_MINUS,
    ELY_TT_PLUS,
    ELY_TT_DIVIDE,
    ELY_TT_MULTIPLY,
    ELY_TT_BWNOT,
    ELY_TT_BWXOR,

    ELY_TT_PLUSPLUS,
    ELY_TT_MINUSMINUS,
    ELY_TT_PLUSASSIGN,
    ELY_TT_MINUSASSIGN,
    ELY_TT_MULTIPLYASSIGN,
    ELY_TT_DIVIDEASSIGN,

    ELY_TT_BANG,
    ELY_TT_NONEQUALITY,
    ELY_TT_ASSIGN,
    ELY_TT_EQUALITY,

    ELY_TT_LESSTHAN,
    ELY_TT_LESSTHANEQUAL,
    ELY_TT_GREATERTHAN,
    ELY_TT_GREATERTHANEQUAL,

    ELY_TT_BWAND,
    ELY_TT_AND,
    ELY_TT_BWOR,
    ELY_TT_OR,
    ELY_TT_SHIFTLEFT,
    ELY_TT_SHIFTRIGHT,

    ELY_TT_IDENTIFIER,
    ELY_TT_STRING,
    ELY_TT_NUMBER,

    ELY_TT_IF,
    ELY_TT_ELSE,
    ELY_TT_WHILE,
    ELY_TT_FOR,
    ELY_TT_RETURN,
    ELY_TT_VAR,
    ELY_TT_FUNC,
    ELY_TT_CLASS,
    ELY_TT_TRUE,
    ELY_TT_FALSE,
    ELY_TT_NULL,
    ELY_TT_THIS,
    ELY_TT_SUPER,
    ELY_TT_IMPORT,

    ELY_TT_PRINT,

    ELY_TT_ERROR,
    ELY_TT_EOF
} ElyTokenType;

/**
 * Represents a lexical token emitted by the scanner, referencing a slice of the source code.
 */
typedef struct ElyToken {
    /// Category of this token.
    ElyTokenType token_type;
    /// Pointer to the beginning of the token lexeme in the original source buffer.
    const char* lexeme;
    /// Length of the token lexeme in bytes.
    size_t length;
    /// 1-based source line where the token was encountered.
    uint32_t line;
} ElyToken;

#endif
