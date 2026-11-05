#include "ely/compiler/token.h"
#include <ely/compiler/scanner.h>

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
    ElyTokenType type;
} Keyword;

/**
 * Constant map for language keywords conversion to ElyTokenType.
 */
static const Keyword KEYWORDS[] = {
    { "class", 5, ELY_TT_CLASS },
    { "else", 4, ELY_TT_ELSE },
    { "false", 5, ELY_TT_FALSE },
    { "for", 3, ELY_TT_FOR },
    { "func", 4, ELY_TT_FUNC },
    { "if", 2, ELY_TT_IF },
    { "import", 6, ELY_TT_IMPORT },
    { "null", 4, ELY_TT_NULL },
    { "print", 5, ELY_TT_PRINT },
    { "return", 6, ELY_TT_RETURN },
    { "super", 5, ELY_TT_SUPER },
    { "this", 4, ELY_TT_THIS },
    { "true", 4, ELY_TT_TRUE },
    { "while", 5, ELY_TT_WHILE },
    { "var", 3, ELY_TT_VAR }
};

void ely_scanner_init(ElyScanner* scanner, const char* source) {
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
static inline bool is_at_end(const ElyScanner* scanner) {
    assert(scanner != NULL && "Pointer to scanner is NULL.");
    return *scanner->current == '\0';
}

/**
 * Consumes and returns the current character, advancing the cursor forward.
 */
static inline char advance(ElyScanner* scanner) {
    assert(scanner != NULL && "Pointer to scanner is NULL.");
    const char* curr = scanner->current++;
    return *curr;
}

/**
 * Returns the current character without advancing the cursor.
 */
static inline char peek(const ElyScanner* scanner) {
    assert(scanner != NULL && "Pointer to scanner is NULL.");
    if (is_at_end(scanner)) return '\0';
    return *scanner->current;
}

/**
 * Returns the character immediately following the current cursor without advancing.
 */
static inline char peek_next(const ElyScanner* scanner) {
    assert(scanner != NULL && "Pointer to scanner is NULL.");
    if (is_at_end(scanner)) return '\0';
    return *(scanner->current + 1);
}

/**
 * Matches the current character against an expected character.
 * Advances the cursor and returns true if matched, otherwise returns false.
 */
static inline bool match_char(ElyScanner* scanner, char expected) {
    assert(scanner != NULL && "Pointer to scanner is NULL.");
    if (is_at_end(scanner)) return false;
    if (*scanner->current != expected) return false;

    scanner->current++;
    return true;
}

/**
 * Constructs an ElyToken of the specified type from the current scanned lexeme window.
 */
static inline ElyToken make_token(const ElyScanner* scanner, ElyTokenType token_type) {
    assert(scanner != NULL && "Pointer to scanner is NULL.");
    return (ElyToken) {
        .token_type = token_type,
        .lexeme = scanner->start,
        .length = (size_t)(scanner->current - scanner->start),
        .line = scanner->line
    };
}

/**
 * Constructs an error token containing an explanatory static error message string.
 */
static inline ElyToken error_token(const ElyScanner* scanner, const char* message) {
    assert(scanner != NULL && "Pointer to scanner is NULL.");
    return (ElyToken) {
        .token_type = ELY_TT_ERROR,
        .lexeme = message,
        .length = strlen(message),
        .line = scanner->line
    };
}

/**
 * Advances past whitespace characters and single-line comment blocks ('//').
 */
static inline void skip_whitespace(ElyScanner* scanner) {
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
static inline ElyToken string(ElyScanner* scanner, char quote) {
    assert(scanner != NULL && "Pointer to scanner is NULL.");

    while (peek(scanner) != quote && !is_at_end(scanner)) {
        if (peek(scanner) == '\n') scanner->line++;
        (void)advance(scanner);
    }

    if (is_at_end(scanner)) return error_token(scanner, "Unterminated string.");

    (void)advance(scanner);

    return make_token(scanner, ELY_TT_STRING);
}

/**
 * Scans integer or floating point numeric literals (e.g. 123 or 123.456).
 */
static inline ElyToken number(ElyScanner* scanner) {
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

    return make_token(scanner, ELY_TT_NUMBER);
}

/**
 * Identifies whether the current identifier matches any language reserved keywords.
 */
static inline ElyTokenType identifier_type(const ElyScanner* scanner) {
    assert(scanner != NULL && "Pointer to scanner is NULL.");

    size_t length = (size_t)(scanner->current - scanner->start);

    for (size_t index = 0; index < sizeof(KEYWORDS) / sizeof(struct Keyword); ++index) {
        if (length != KEYWORDS[index].length) continue;
        if (scanner->start[0] != KEYWORDS[index].name[0]) continue;
        if (memcmp(scanner->start, KEYWORDS[index].name, length) == 0) {
            return KEYWORDS[index].type;
        }
    }

    return ELY_TT_IDENTIFIER;
}

/**
 * Scans an identifier or keyword token.
 */
static inline ElyToken identifier(ElyScanner* scanner) {
    assert(scanner != NULL && "Pointer to scanner is NULL.");

    while (isalnum(peek(scanner)) || peek(scanner) == '_') {
        (void)advance(scanner);
    }

    const ElyTokenType token_type = identifier_type(scanner);
    return make_token(scanner, token_type);
}

ELY_NODISCARD ElyToken ely_scanner_scan_token(ElyScanner* scanner) {
    assert(scanner != NULL && "Pointer to scanner is NULL.");

    // Skip all white spaces
    skip_whitespace(scanner);
    scanner->start = scanner->current;

    if (is_at_end(scanner)) return make_token(scanner, ELY_TT_EOF);

    const char ch = advance(scanner);
    if (isalpha((unsigned char)ch) || ch == '_') return identifier(scanner);
    if (isdigit((unsigned char)ch)) return number(scanner);

    switch (ch) {
        case '(': return make_token(scanner, ELY_TT_OPENPAREN);
        case ')': return make_token(scanner, ELY_TT_CLOSEPAREN);
        case '{': return make_token(scanner, ELY_TT_OPENCURLY);
        case '}': return make_token(scanner, ELY_TT_CLOSECURLY);
        case '[': return make_token(scanner, ELY_TT_OPENSQUARE);
        case ']': return make_token(scanner, ELY_TT_CLOSESQUARE);
        case ';': return make_token(scanner, ELY_TT_SEMICOLON);
        case ',': return make_token(scanner, ELY_TT_COMMA);
        case '.': return make_token(scanner, ELY_TT_DOT);
        case '-': return make_token(scanner, match_char(scanner, '-')
            ? ELY_TT_MINUSMINUS
            : match_char(scanner, '=')
                ? ELY_TT_MINUSASSIGN
                : ELY_TT_MINUS
        );
        case '+': return make_token(scanner, match_char(scanner, '+')
            ? ELY_TT_PLUSPLUS
            : match_char(scanner, '=')
                ? ELY_TT_PLUSASSIGN
                : ELY_TT_PLUS
        );
        case '/': return make_token(scanner, match_char(scanner, '=')
            ? ELY_TT_DIVIDEASSIGN
            : ELY_TT_DIVIDE
        );
        case '*': return make_token(scanner, match_char(scanner, '=')
            ? ELY_TT_MULTIPLYASSIGN
            : ELY_TT_MULTIPLY
        );
        case '~': return make_token(scanner, ELY_TT_BWNOT);
        case '^': return make_token(scanner, ELY_TT_BWXOR);
        case '!': return make_token(scanner, match_char(scanner, '=')
            ? ELY_TT_NONEQUALITY
            : ELY_TT_BANG
        );
        case '=': return make_token(scanner, match_char(scanner, '=')
            ? ELY_TT_EQUALITY
            : ELY_TT_ASSIGN
        );
        case '<': return make_token(scanner, match_char(scanner, '=')
            ? ELY_TT_LESSTHANEQUAL
            : match_char(scanner, '<')
                ? ELY_TT_SHIFTLEFT
                : ELY_TT_LESSTHAN
        );
        case '>': return make_token(scanner, match_char(scanner, '=')
            ? ELY_TT_GREATERTHANEQUAL
            : match_char(scanner, '>')
                ? ELY_TT_SHIFTRIGHT
                : ELY_TT_GREATERTHAN
        );
        case '&': return make_token(scanner, match_char(scanner, '&')
            ? ELY_TT_AND
            : ELY_TT_BWAND
        );
        case '|': return make_token(scanner, match_char(scanner, '|')
            ? ELY_TT_OR
            : ELY_TT_BWOR
        );
        case '"':
        case '\'': return string(scanner, ch);
        default: return error_token(scanner, "Unexpected character.");
    }
}
