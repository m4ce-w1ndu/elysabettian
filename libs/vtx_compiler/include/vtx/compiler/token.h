#ifndef VTX_COMPILER_TOKEN_H
#define VTX_COMPILER_TOKEN_H

#include <stddef.h>
#include <stdint.h>

/**
 * Represents the classification of lexical tokens
 * recognized by the Elysabettian tokenizer.
 */
typedef enum VtxTokenType {
    VTX_TT_OPENPAREN,
    VTX_TT_CLOSEPAREN,
    VTX_TT_OPENCURLY,
    VTX_TT_CLOSECURLY,
    VTX_TT_OPENSQUARE,
    VTX_TT_CLOSESQUARE,

    VTX_TT_COMMA,
    VTX_TT_DOT,
    VTX_TT_SEMICOLON,

    VTX_TT_MINUS,
    VTX_TT_PLUS,
    VTX_TT_DIVIDE,
    VTX_TT_MULTIPLY,
    VTX_TT_BWNOT,
    VTX_TT_BWXOR,

    VTX_TT_PLUSPLUS,
    VTX_TT_MINUSMINUS,
    VTX_TT_PLUSASSIGN,
    VTX_TT_MINUSASSIGN,
    VTX_TT_MULTIPLYASSIGN,
    VTX_TT_DIVIDEASSIGN,

    VTX_TT_BANG,
    VTX_TT_NONEQUALITY,
    VTX_TT_ASSIGN,
    VTX_TT_EQUALITY,

    VTX_TT_LESSTHAN,
    VTX_TT_LESSTHANEQUAL,
    VTX_TT_GREATERTHAN,
    VTX_TT_GREATERTHANEQUAL,

    VTX_TT_BWAND,
    VTX_TT_AND,
    VTX_TT_BWOR,
    VTX_TT_OR,
    VTX_TT_SHIFTLEFT,
    VTX_TT_SHIFTRIGHT,

    VTX_TT_IDENTIFIER,
    VTX_TT_STRING,
    VTX_TT_NUMBER,

    VTX_TT_IF,
    VTX_TT_ELSE,
    VTX_TT_WHILE,
    VTX_TT_FOR,
    VTX_TT_RETURN,
    VTX_TT_VAR,
    VTX_TT_FUNC,
    VTX_TT_CLASS,
    VTX_TT_TRUE,
    VTX_TT_FALSE,
    VTX_TT_NULL,
    VTX_TT_THIS,
    VTX_TT_SUPER,
    VTX_TT_IMPORT,

    VTX_TT_PRINT,

    VTX_TT_ERROR,
    VTX_TT_EOF
} VtxTokenType;

/**
 * Represents a lexical token emitted by the scanner, referencing a slice of the source code.
 */
typedef struct VtxToken {
    /// Category of this token.
    VtxTokenType token_type;
    /// Pointer to the beginning of the token lexeme in the original source buffer.
    const char* lexeme;
    /// Length of the token lexeme in bytes.
    size_t length;
    /// 1-based source line where the token was encountered.
    uint32_t line;
} VtxToken;

#endif
