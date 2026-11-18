#include "vtx/compiler/token.h"
#include <vtx/compiler/scanner.h>

#include <ctype.h>
#include <string.h>
#include <stdbool.h>
#include <stddef.h>

/**
 * Contains a keyword description.
 */
typedef struct Keyword {
    const char* name;
    size_t length;
    VtxTokenType type;
} Keyword;

/**
 * Constant map for language keywords conversion to VtxTokenType.
 */
static const Keyword KEYWORDS[] = {
    { "class", 5, VTX_TT_CLASS },
    { "else", 4, VTX_TT_ELSE },
    { "false", 5, VTX_TT_FALSE },
    { "for", 3, VTX_TT_FOR },
    { "func", 4, VTX_TT_FUNC },
    { "if", 2, VTX_TT_IF },
    { "import", 6, VTX_TT_IMPORT },
    { "null", 4, VTX_TT_NULL },
    { "print", 5, VTX_TT_PRINT },
    { "return", 6, VTX_TT_RETURN },
    { "super", 5, VTX_TT_SUPER },
    { "this", 4, VTX_TT_THIS },
    { "true", 4, VTX_TT_TRUE },
    { "while", 5, VTX_TT_WHILE },
    { "var", 3, VTX_TT_VAR }
};

void vtx_scanner_init(VtxScanner* scanner, const char* source) {
    assert(scanner != NULL && "Pointer to scanner is NULL.");
    assert(source != NULL && "Pointer to source code is NULL.");

    // Setup scanenr status
    scanner->start = source;
    scanner->current = source;
    scanner->line = 1;
}

/**
 * Returns true if the scanner read cursor has reached the end of the source string.
 */
static inline bool is_at_end(const VtxScanner* scanner) {
    assert(scanner != NULL && "Pointer to scanner is NULL.");
    return *scanner->current == '\0';
}

/**
 * Consumes and returns the current character, advancing the cursor forward.
 */
static inline char advance(VtxScanner* scanner) {
    assert(scanner != NULL && "Pointer to scanner is NULL.");
    const char* curr = scanner->current++;
    return *curr;
}

/**
 * Returns the current character without advancing the cursor.
 */
static inline char peek(const VtxScanner* scanner) {
    assert(scanner != NULL && "Pointer to scanner is NULL.");
    if (is_at_end(scanner)) return '\0';
    return *scanner->current;
}

/**
 * Returns the character immediately following the current cursor without advancing.
 */
static inline char peek_next(const VtxScanner* scanner) {
    assert(scanner != NULL && "Pointer to scanner is NULL.");
    if (is_at_end(scanner)) return '\0';
    return *(scanner->current + 1);
}

/**
 * Matches the current character against an expected character.
 * Advances the cursor and returns true if matched, otherwise returns false.
 */
static inline bool match_char(VtxScanner* scanner, char expected) {
    assert(scanner != NULL && "Pointer to scanner is NULL.");
    if (is_at_end(scanner)) return false;
    if (*scanner->current != expected) return false;

    scanner->current++;
    return true;
}

/**
 * Constructs an VtxToken of the specified type from the current scanned lexeme window.
 */
static inline VtxToken make_token(const VtxScanner* scanner, VtxTokenType token_type) {
    assert(scanner != NULL && "Pointer to scanner is NULL.");
    return (VtxToken) {
        .token_type = token_type,
        .lexeme = scanner->start,
        .length = (size_t)(scanner->current - scanner->start),
        .line = scanner->line
    };
}

/**
 * Constructs an error token containing an explanatory static error message string.
 */
static inline VtxToken error_token(const VtxScanner* scanner, const char* message) {
    assert(scanner != NULL && "Pointer to scanner is NULL.");
    return (VtxToken) {
        .token_type = VTX_TT_ERROR,
        .lexeme = message,
        .length = strlen(message),
        .line = scanner->line
    };
}

/**
 * Advances past whitespace characters and single-line comment blocks ('//').
 */
static inline void skip_whitespace(VtxScanner* scanner) {
    assert(scanner != NULL && "Pointer to scanner is NULL.");

    while (true) {
        switch (peek(scanner)) {
            case ' ':
            case '\r':
            case '\t':
                (void)advance(scanner); break;
            case '\n':
                (void)advance(scanner);
                scanner->line++;
                break;
            case '/': {
                if (peek_next(scanner) != '/') return;
                while (peek(scanner) != '\n' && !is_at_end(scanner)) {
                    (void)advance(scanner);
                }
                break;
            }
            default: return;
        }
    }
}

/**
 * Scans a string literal enclosed by single or double quotes.
 */
static inline VtxToken string(VtxScanner* scanner, char quote) {
    assert(scanner != NULL && "Pointer to scanner is NULL.");

    while (peek(scanner) != quote && !is_at_end(scanner)) {
        if (peek(scanner) == '\n') scanner->line++;
        (void)advance(scanner);
    }

    if (is_at_end(scanner)) return error_token(scanner, "Unterminated string.");

    (void)advance(scanner);

    return make_token(scanner, VTX_TT_STRING);
}

/**
 * Scans integer or floating point numeric literals (e.g. 123 or 123.456).
 */
static inline VtxToken number(VtxScanner* scanner) {
    assert(scanner != NULL && "Pointer to scanner is NULL.");

    while (isdigit((unsigned char)peek(scanner))) {
        (void)advance(scanner);
    }

    if (peek(scanner) == '.' && isdigit((unsigned char)peek_next(scanner))) {
        (void)advance(scanner);

        while (isdigit((unsigned char)peek(scanner))) {
            (void)advance(scanner);
        }
    }

    return make_token(scanner, VTX_TT_NUMBER);
}

/**
 * Identifies whether the current identifier matches any language reserved keywords.
 */
static inline VtxTokenType identifier_type(const VtxScanner* scanner) {
    assert(scanner != NULL && "Pointer to scanner is NULL.");

    size_t length = (size_t)(scanner->current - scanner->start);

    for (size_t index = 0; index < sizeof(KEYWORDS) / sizeof(struct Keyword); ++index) {
        if (length != KEYWORDS[index].length) continue;
        if (scanner->start[0] != KEYWORDS[index].name[0]) continue;
        if (memcmp(scanner->start, KEYWORDS[index].name, length) == 0) {
            return KEYWORDS[index].type;
        }
    }

    return VTX_TT_IDENTIFIER;
}

/**
 * Scans an identifier or keyword token.
 */
static inline VtxToken identifier(VtxScanner* scanner) {
    assert(scanner != NULL && "Pointer to scanner is NULL.");

    while (isalnum(peek(scanner)) || peek(scanner) == '_') {
        (void)advance(scanner);
    }

    const VtxTokenType token_type = identifier_type(scanner);
    return make_token(scanner, token_type);
}

VTX_NODISCARD VtxToken vtx_scanner_scan_token(VtxScanner* scanner) {
    assert(scanner != NULL && "Pointer to scanner is NULL.");

    // Skip all white spaces
    skip_whitespace(scanner);
    scanner->start = scanner->current;

    if (is_at_end(scanner)) return make_token(scanner, VTX_TT_EOF);

    const char ch = advance(scanner);
    if (isalpha((unsigned char)ch) || ch == '_') return identifier(scanner);
    if (isdigit((unsigned char)ch)) return number(scanner);

    switch (ch) {
        case '(': return make_token(scanner, VTX_TT_OPENPAREN);
        case ')': return make_token(scanner, VTX_TT_CLOSEPAREN);
        case '{': return make_token(scanner, VTX_TT_OPENCURLY);
        case '}': return make_token(scanner, VTX_TT_CLOSECURLY);
        case '[': return make_token(scanner, VTX_TT_OPENSQUARE);
        case ']': return make_token(scanner, VTX_TT_CLOSESQUARE);
        case ';': return make_token(scanner, VTX_TT_SEMICOLON);
        case ',': return make_token(scanner, VTX_TT_COMMA);
        case '.': return make_token(scanner, VTX_TT_DOT);
        case '-': return make_token(scanner, match_char(scanner, '-')
            ? VTX_TT_MINUSMINUS
            : match_char(scanner, '=')
                ? VTX_TT_MINUSASSIGN
                : VTX_TT_MINUS
        );
        case '+': return make_token(scanner, match_char(scanner, '+')
            ? VTX_TT_PLUSPLUS
            : match_char(scanner, '=')
                ? VTX_TT_PLUSASSIGN
                : VTX_TT_PLUS
        );
        case '/': return make_token(scanner, match_char(scanner, '=')
            ? VTX_TT_DIVIDEASSIGN
            : VTX_TT_DIVIDE
        );
        case '*': return make_token(scanner, match_char(scanner, '=')
            ? VTX_TT_MULTIPLYASSIGN
            : VTX_TT_MULTIPLY
        );
        case '~': return make_token(scanner, VTX_TT_BWNOT);
        case '^': return make_token(scanner, VTX_TT_BWXOR);
        case '!': return make_token(scanner, match_char(scanner, '=')
            ? VTX_TT_NONEQUALITY
            : VTX_TT_BANG
        );
        case '=': return make_token(scanner, match_char(scanner, '=')
            ? VTX_TT_EQUALITY
            : VTX_TT_ASSIGN
        );
        case '<': return make_token(scanner, match_char(scanner, '=')
            ? VTX_TT_LESSTHANEQUAL
            : match_char(scanner, '<')
                ? VTX_TT_SHIFTLEFT
                : VTX_TT_LESSTHAN
        );
        case '>': return make_token(scanner, match_char(scanner, '=')
            ? VTX_TT_GREATERTHANEQUAL
            : match_char(scanner, '>')
                ? VTX_TT_SHIFTRIGHT
                : VTX_TT_GREATERTHAN
        );
        case '&': return make_token(scanner, match_char(scanner, '&')
            ? VTX_TT_AND
            : VTX_TT_BWAND
        );
        case '|': return make_token(scanner, match_char(scanner, '|')
            ? VTX_TT_OR
            : VTX_TT_BWOR
        );
        case '"':
        case '\'': return string(scanner, ch);
        default: return error_token(scanner, "Unexpected character.");
    }
}
