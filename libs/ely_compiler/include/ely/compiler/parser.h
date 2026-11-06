#ifndef ELY_COMPILER_PARSER_H
#define ELY_COMPILER_PARSER_H

#include <stdbool.h>

#include <ely/compiler/token.h>
#include <ely/core/value.h>
#include "scanner.h"

/**
 * Expression precedence levels ordered from lowest to highest for Pratt parsing.
 */
typedef enum ElyPrecedence {
    ELY_PREC_NONE = 0,
    ELY_PREC_ASSIGNMENT,
    ELY_PREC_OR,
    ELY_PREC_AND,
    ELY_PREC_EQUALITY,
    ELY_PREC_COMPARISON,
    ELY_PREC_TERM,
    ELY_PREC_FACTOR,
    ELY_PREC_UNARY,
    ELY_PREC_CALL,
    ELY_PREC_PRIMARY
} ElyPrecedence;

/**
 * Returns the immediate higher precedence level.
 *
 * @param current Current precedence level.
 * @return Next higher Precedence, clamped at Precedence::Primary.
 */
ELY_NODISCARD static inline ElyPrecedence ely_precedence_next(ElyPrecedence current) {
    if (current >= ELY_PREC_PRIMARY) return ELY_PREC_PRIMARY;
    return current + 1;
}

/**
 * Maintains the lexical scanner and syntax error state during compilation.
 */
typedef struct ElyParser {
    /// Active lexical scanner providing the token stream.
    ElyScanner scanner;
    /// Current lookahead token in the stream.
    ElyToken current;
    /// Most recently consumed token.
    ElyToken previous;
    /// Set to true if a syntax or compilation error occurred.
    bool had_error;
    /// Panic mode flag suppressing cascaded error reports until synchronization.
    bool panic_mode;
} ElyParser;

/**
 * Constructs a new parser wrapping the given Scanner.
 *
 * @param parser Pointer to the current parser object.
 * @param scanner Initialized Scanner instance.
 */
void ely_parser_init(ElyParser* parser, const char* source);

/**
 * Emits a syntax error at the given token location unless currently in panic mode.
 *
 * @param parser Pointer to the current parser object.
 * @param token Token where the syntax violation occurred.
 * @param message Descriptive error message.
 */
void ely_parser_error_at(ElyParser* parser, const ElyToken token, const char* message);

/**
 * Emits a syntax error at the previous token position.
 *
 * @param parser Pointer to the current parser object.
 * @param message Descriptive error message.
 */
void ely_parser_error(ElyParser* parser, const char* message);

/**
 * Emits a syntax error at the current lookahead token position.
 *
 * @param parser Pointer to the current parser object.
 * @param message Descriptive error message.
 */
void ely_parser_error_at_current(ElyParser* parser, const char* message);

/**
 * Advances the parser by consuming the next valid token from the scanner.
 *
 * @param parser Pointer to the current parser object.
 */
void ely_parser_advance(ElyParser* parser);

/**
 * Consumes the current token if it matches `expected`, otherwise emits a syntax error.
 *
 * @param parser Pointer to the current parser object.
 * @param expected Expected token type.
 * @param message Error message to report if the token does not match.
 */
void ely_parser_consume(ElyParser* parser, ElyTokenType expected, const char* message);

/**
 * Checks whether the current token matches the given type without consuming it.
 *
 * @param parser Pointer to the current parser object.
 * @param expected Token type to check.
 * @return True if current token matches expected, false otherwise.
 */
ELY_NODISCARD bool ely_parser_check(const ElyParser* parser, const ElyTokenType expected);

/**
 * Consumes the current token if it matches `expected`, returning whether it matched.
 *
 * @param parser Pointer to the current parser object.
 * @param expected Token type to match and consume.
 * @return True if matched and advanced, false otherwise.
 */
ELY_NODISCARD bool ely_parser_match_token(ElyParser* parser, ElyTokenType expected);

/**
 * Resets the panic mode to false, resuming parser operation.
 *
 * @param parser Pointer to the current parser object.
 */
static inline void ely_parser_reset_panic_mode(ElyParser* parser) {
    assert(parser != NULL && "Parser pointer is NULL.");
    parser->panic_mode = false;
}

#endif
