#ifndef VTX_COMPILER_PARSER_H
#define VTX_COMPILER_PARSER_H

#include <stdbool.h>

#include <vtx/compiler/token.h>
#include <vtx/core/value.h>
#include "scanner.h"

/**
 * Expression precedence levels ordered from lowest to highest for Pratt parsing.
 */
typedef enum VtxPrecedence {
    VTX_PREC_NONE = 0,
    VTX_PREC_ASSIGNMENT,
    VTX_PREC_OR,
    VTX_PREC_AND,
    VTX_PREC_EQUALITY,
    VTX_PREC_COMPARISON,
    VTX_PREC_TERM,
    VTX_PREC_FACTOR,
    VTX_PREC_UNARY,
    VTX_PREC_CALL,
    VTX_PREC_PRIMARY
} VtxPrecedence;

/**
 * Returns the immediate higher precedence level.
 *
 * @param current Current precedence level.
 * @return Next higher Precedence, clamped at Precedence::Primary.
 */
VTX_NODISCARD static inline VtxPrecedence vtx_precedence_next(VtxPrecedence current) {
    if (current >= VTX_PREC_PRIMARY) return VTX_PREC_PRIMARY;
    return current + 1;
}

/**
 * Maintains the lexical scanner and syntax error state during compilation.
 */
typedef struct VtxParser {
    /// Active lexical scanner providing the token stream.
    VtxScanner scanner;
    /// Current lookahead token in the stream.
    VtxToken current;
    /// Most recently consumed token.
    VtxToken previous;
    /// Set to true if a syntax or compilation error occurred.
    bool had_error;
    /// Panic mode flag suppressing cascaded error reports until synchronization.
    bool panic_mode;
} VtxParser;

/**
 * Constructs a new parser wrapping the given Scanner.
 *
 * @param parser Pointer to the current parser object.
 * @param scanner Initialized Scanner instance.
 */
void vtx_parser_init(VtxParser* parser, const char* source);

/**
 * Emits a syntax error at the given token location unless currently in panic mode.
 *
 * @param parser Pointer to the current parser object.
 * @param token Token where the syntax violation occurred.
 * @param message Descriptive error message.
 */
void vtx_parser_error_at(VtxParser* parser, const VtxToken token, const char* message);

/**
 * Emits a syntax error at the previous token position.
 *
 * @param parser Pointer to the current parser object.
 * @param message Descriptive error message.
 */
void vtx_parser_error(VtxParser* parser, const char* message);

/**
 * Emits a syntax error at the current lookahead token position.
 *
 * @param parser Pointer to the current parser object.
 * @param message Descriptive error message.
 */
void vtx_parser_error_at_current(VtxParser* parser, const char* message);

/**
 * Advances the parser by consuming the next valid token from the scanner.
 *
 * @param parser Pointer to the current parser object.
 */
void vtx_parser_advance(VtxParser* parser);

/**
 * Consumes the current token if it matches `expected`, otherwise emits a syntax error.
 *
 * @param parser Pointer to the current parser object.
 * @param expected Expected token type.
 * @param message Error message to report if the token does not match.
 */
void vtx_parser_consume(VtxParser* parser, VtxTokenType expected, const char* message);

/**
 * Checks whether the current token matches the given type without consuming it.
 *
 * @param parser Pointer to the current parser object.
 * @param expected Token type to check.
 * @return True if current token matches expected, false otherwise.
 */
VTX_NODISCARD bool vtx_parser_check(const VtxParser* parser, const VtxTokenType expected);

/**
 * Consumes the current token if it matches `expected`, returning whether it matched.
 *
 * @param parser Pointer to the current parser object.
 * @param expected Token type to match and consume.
 * @return True if matched and advanced, false otherwise.
 */
VTX_NODISCARD bool vtx_parser_match_token(VtxParser* parser, VtxTokenType expected);

/**
 * Resets the panic mode to false, resuming parser operation.
 *
 * @param parser Pointer to the current parser object.
 */
static inline void vtx_parser_reset_panic_mode(VtxParser* parser) {
    assert(parser != NULL && "Parser pointer is NULL.");
    parser->panic_mode = false;
}

#endif
