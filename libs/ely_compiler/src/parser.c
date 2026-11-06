#include "ely/compiler/scanner.h"
#include <ely/compiler/token.h>
#include <ely/compiler/parser.h>
#include <stdio.h>
#include <string.h>

#define LOCATION_BUFSIZE 128

/**
 * Initializes a new ElyParser instance over the provided source string.
 */
void ely_parser_init(ElyParser* parser, const char* source) {
    assert(parser != NULL && "Parser pointer is NULL.");
    parser->current = (ElyToken) { .token_type = ELY_TT_EOF, .lexeme = "", .length = 0, .line = 1 };
    parser->previous = (ElyToken) { .token_type = ELY_TT_EOF, .lexeme = "", .length = 0, .line = 1 };
    parser->panic_mode = false;
    parser->had_error = false;
    ely_scanner_init(&parser->scanner, source);
}

/**
 * Emits a formatted syntax error at the given token location to stderr.
 */
void ely_parser_error_at(ElyParser* parser, const ElyToken token, const char* message) {
    assert(parser != NULL && "Parser pointer is NULL.");
    if (parser->panic_mode) return;

    parser->panic_mode = true;
    parser->had_error = true;

    char fmt_loc[LOCATION_BUFSIZE];
    memset(fmt_loc, 0, sizeof(char) * LOCATION_BUFSIZE);

    if (token.token_type == ELY_TT_EOF) {
        strncpy(fmt_loc, " at end", LOCATION_BUFSIZE);
    } else if (token.token_type != ELY_TT_ERROR) {
        snprintf(fmt_loc, LOCATION_BUFSIZE, " at '%.*s'", (int)token.length, token.lexeme);
    }

    fprintf(stderr, "[line %u] Error%s: %s\n", token.line, fmt_loc, message);
}

/**
 * Reports a syntax error located at the previous consumed token.
 */
void ely_parser_error(ElyParser* parser, const char* message) {
    assert(parser != NULL && "Parser pointer is NULL.");
    ely_parser_error_at(parser, parser->previous, message);
}

/**
 * Reports a syntax error located at the current lookahead token.
 */
void ely_parser_error_at_current(ElyParser* parser, const char* message) {
    assert(parser != NULL && "Parser pointer is NULL.");
    ely_parser_error_at(parser, parser->current, message);
}

/**
 * Advances the parser cursor by consuming the next non-error token.
 */
void ely_parser_advance(ElyParser* parser) {
    assert(parser != NULL && "Parser pointer is NULL.");
    parser->previous = parser->current;

    while (true) {
        parser->current = ely_scanner_scan_token(&parser->scanner);
        if (parser->current.token_type != ELY_TT_ERROR) break;
        ely_parser_error_at_current(parser, parser->current.lexeme);
    }
}

/**
 * Consumes the current token if it matches the expected type, otherwise emits an error.
 */
void ely_parser_consume(ElyParser* parser, ElyTokenType expected, const char* message) {
    assert(parser != NULL && "Parser pointer is NULL.");

    if (parser->current.token_type == expected) {
        ely_parser_advance(parser);
        return;
    }

    ely_parser_error_at_current(parser, message);
}

/**
 * Returns true if the current token matches the expected type without consuming it.
 */
ELY_NODISCARD bool ely_parser_check(const ElyParser* parser, const ElyTokenType expected) {
    assert(parser != NULL && "Parser pointer is NULL.");
    return parser->current.token_type == expected;
}

/**
 * Consumes the current token if it matches expected, returning true if matched.
 */
ELY_NODISCARD bool ely_parser_match_token(ElyParser* parser, ElyTokenType expected) {
    assert(parser != NULL && "Parser pointer is NULL.");

    if (!ely_parser_check(parser, expected)) return false;
    ely_parser_advance(parser);
    return true;
}
