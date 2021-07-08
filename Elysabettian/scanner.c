//
//  scanner.c
//  Elysabettian
//
//  Created by Simone Rolando on 07/07/21.
//

#include "scanner.h"
#include "common.h"

#include <string.h>
#include <ctype.h>

typedef struct Scanner {
    const char* start;
    const char* current;
    int line;
} Scanner;

Scanner scanner;

void init_scanner(const char* source)
{
    scanner.start = source;
    scanner.current = source;
    scanner.line = 1;
}

static bool is_at_end()
{
    return *scanner.current == '\0';
}

static char advance()
{
    scanner.current++;
    return scanner.current[-1];
}

static char peek()
{
    return *scanner.current;
}

static char peek_next()
{
    if (is_at_end()) return '\0';
    return scanner.current[1];
}

static bool match(char exp)
{
    if (is_at_end()) return false;
    if (*scanner.current != exp) return false;
    scanner.current++;
    return true;
}

static Token make_token(TokenType type)
{
    Token token;
    token.type = type;
    token.start = scanner.start;
    token.length = (int)(scanner.current - scanner.start);
    token.line = scanner.line;
    return token;
}

static Token error_token(const char* message)
{
    Token token;
    token.type = ERROR;
    token.start = message;
    token.length = (int)strlen(message);
    token.line = scanner.line;
    return token;
}

static void skip_whitespace()
{
    for (;;) {
        char c = peek();
        switch (c) {
            case ' ':
            case '\r':
            case '\t':
                advance();
                break;
            case '\n':
                scanner.line++;
                advance();
                break;
            case '/':
                if (peek_next() == '/') {
                    while (peek() != '\n' && !is_at_end()) advance();
                } else return;
                break;
            default:
                return;
        }
    }
}

static TokenType check_keyword(int start, int length, const char* rest, TokenType type)
{
    if (scanner.current - scanner.start == start + length &&
        memcmp(scanner.start + start, rest, length) == 0)
        return type;
    return IDENTIFIER;
}

static TokenType identifier_type()
{
    switch (scanner.start[0]) {
        case 'c':
            if (scanner.current - scanner.start > 1) {
                switch (scanner.start[1]) {
                    case 'l': return check_keyword(2, 3, "ass", CLASS);
                    case 'o': return check_keyword(2, 3, "nst", CONST);
                }
            }
            break;
        case 'e': return check_keyword(1, 3, "lse", ELSE);
        case 'f':
            if (scanner.current - scanner.start > 1) {
                switch (scanner.start[1]) {
                    case 'a': return check_keyword(2, 3, "lse", FALSEVAL);
                    case 'o': return check_keyword(2, 1, "r", FOR);
                    case 'u': return check_keyword(2, 2, "nc", FUNC);
                }
            }
            break;
        case 'i': return check_keyword(1, 1, "f", IF);
        case 'n': return check_keyword(1, 3, "ull", NULLVAL);
        case 'p': return check_keyword(1, 4, "rint", PRINT);
        case 'r': return check_keyword(1, 5, "eturn", RETURN);
        case 's': return check_keyword(1, 4, "uper", SUPER);
        case 't':
            if (scanner.current - scanner.start > 1) {
                switch (scanner.start[1]) {
                    case 'h': return check_keyword(2, 2, "is", THIS);
                    case 'r': return check_keyword(2, 2, "ue", TRUEVAL);
                }
            }
            break;
        case 'v': return check_keyword(1, 2, "ar", VAR);
        case 'w': return check_keyword(1, 4, "hile", WHILE);
    }
    
    return IDENTIFIER;
}

static Token identifier()
{
    while (isalnum(peek())) advance();
    return make_token(identifier_type());
}

static Token number()
{
    while (isdigit(peek())) advance();
    if (peek() == '.' && isdigit(peek_next())) {
        advance();
        while (isdigit(peek())) advance();
    }
    
    return make_token(NUMBER);
}

static Token string()
{
    while (peek() != '"' && !is_at_end()) {
        if (peek() == '\n') scanner.line++;
        advance();
    }
    
    if (is_at_end()) return error_token("Unterminated string.");
    advance();
    return make_token(STRING);
}

Token scan_token()
{
    skip_whitespace();
    scanner.start = scanner.current;
    
    if (is_at_end()) return make_token(T_EOF);
    char c = advance();
    
    if (isalpha(c)) return identifier();
    if (isdigit(c)) return number();
    
    switch (c) {
        case '(': return make_token(OPEN_PAREN);
        case ')': return make_token(CLOSE_PAREN);
        case '{': return make_token(OPEN_CURLY);
        case '}': return make_token(CLOSE_CURLY);
        case ';': return make_token(SEMICOLON);
        case ',': return make_token(COMMA);
        case '.': return make_token(DOT);
        case '-': return make_token(SUB);
        case '+': return make_token(ADD);
        case '/': return make_token(DIV);
        case '*': return make_token(MUL);
        case '%': return make_token(MOD);
        case '&':
            return make_token(match('&') ? AND : BITWISE_AND);
        case '|':
            return make_token(match('|') ? OR : BITWISE_OR);
        case '!':
            return make_token(match('=') ? NON_EQUALITY : NOT);
        case '=':
            return make_token(match('=') ? EQUALITY : ASSIGN);
        case '<':
            return make_token(match('=') ? LESS_EQUAL : LESS_THAN);
        case '>':
            return make_token(match('=') ? GREATER_EQUAL : GREATER_THAN);
        case '"': return string();
    }
    
    return error_token("Unexpected character.");
}
