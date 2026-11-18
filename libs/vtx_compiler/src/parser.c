#include "vtx/compiler/scanner.h"
#include <vtx/compiler/token.h>
#include <vtx/compiler/parser.h>
#include <stdio.h>
#include <string.h>

#define LOCATION_BUFSIZE 128

/**
 * Initializes a new VtxParser instance over the provided source string.
 */
void vtx_parser_init(VtxParser* parser, const char* source) {
    assert(parser != NULL && "Parser pointer is NULL.");
    parser->current = (VtxToken) { .token_type = VTX_TT_EOF, .lexeme = "", .length = 0, .line = 1 };
    parser->previous = (VtxToken) { .token_type = VTX_TT_EOF, .lexeme = "", .length = 0, .line = 1 };
    parser->panic_mode = false;
    parser->had_error = false;
    vtx_scanner_init(&parser->scanner, source);
}

/**
 * Emits a formatted syntax error at the given token location to stderr.
 */
void vtx_parser_error_at(VtxParser* parser, const VtxToken token, const char* message) {
    assert(parser != NULL && "Parser pointer is NULL.");
    if (parser->panic_mode) return;

    parser->panic_mode = true;
    parser->had_error = true;

    char fmt_loc[LOCATION_BUFSIZE];
    memset(fmt_loc, 0, sizeof(char) * LOCATION_BUFSIZE);

    if (token.token_type == VTX_TT_EOF) {
        strncpy(fmt_loc, " at end", LOCATION_BUFSIZE);
    } else if (token.token_type != VTX_TT_ERROR) {
        snprintf(fmt_loc, LOCATION_BUFSIZE, " at '%.*s'", (int)token.length, token.lexeme);
    }

    fprintf(stderr, "[line %u] Error%s: %s\n", token.line, fmt_loc, message);
}

/**
 * Reports a syntax error located at the previous consumed token.
 */
void vtx_parser_error(VtxParser* parser, const char* message) {
    assert(parser != NULL && "Parser pointer is NULL.");
    vtx_parser_error_at(parser, parser->previous, message);
}

/**
 * Reports a syntax error located at the current lookahead token.
 */
void vtx_parser_error_at_current(VtxParser* parser, const char* message) {
    assert(parser != NULL && "Parser pointer is NULL.");
    vtx_parser_error_at(parser, parser->current, message);
}

/**
 * Advances the parser cursor by consuming the next non-error token.
 */
void vtx_parser_advance(VtxParser* parser) {
    assert(parser != NULL && "Parser pointer is NULL.");
    parser->previous = parser->current;

    while (true) {
        parser->current = vtx_scanner_scan_token(&parser->scanner);
        if (parser->current.token_type != VTX_TT_ERROR) break;
        vtx_parser_error_at_current(parser, parser->current.lexeme);
    }
}

/**
 * Consumes the current token if it matches the expected type, otherwise emits an error.
 */
void vtx_parser_consume(VtxParser* parser, VtxTokenType expected, const char* message) {
    assert(parser != NULL && "Parser pointer is NULL.");

    if (parser->current.token_type == expected) {
        vtx_parser_advance(parser);
        return;
    }

    vtx_parser_error_at_current(parser, message);
}

/**
 * Returns true if the current token matches the expected type without consuming it.
 */
VTX_NODISCARD bool vtx_parser_check(const VtxParser* parser, const VtxTokenType expected) {
    assert(parser != NULL && "Parser pointer is NULL.");
    return parser->current.token_type == expected;
}

/**
 * Consumes the current token if it matches expected, returning true if matched.
 */
VTX_NODISCARD bool vtx_parser_match_token(VtxParser* parser, VtxTokenType expected) {
    assert(parser != NULL && "Parser pointer is NULL.");

    if (!vtx_parser_check(parser, expected)) return false;
    vtx_parser_advance(parser);
    return true;
}
