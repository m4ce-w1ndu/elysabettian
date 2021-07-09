//
//  scanner.h
//  Elysabettian
//
//  Created by Simone Rolando on 07/07/21.
//

#ifndef scanner_h
#define scanner_h

typedef enum TokenType {
    OPEN_PAREN, CLOSE_PAREN, OPEN_CURLY, CLOSE_CURLY,
    COMMA, DOT, SUB, ADD, SEMICOLON, DIV, MUL, IDIV,
    
    // Dual or single char tokens
    NOT, NON_EQUALITY, ASSIGN, EQUALITY, GREATER_THAN,
    LESS_THAN, GREATER_EQUAL, LESS_EQUAL, AND, OR, MOD,
    BITWISE_NOT, BITWISE_XOR, BITWISE_AND, BITWISE_OR,
    
    // Literals
    IDENTIFIER, STRING, NUMBER,
    
    // Keywords
    CLASS, ELSE, IF, FALSEVAL, FOR, FUNC, NULLVAL, PRINT,
    RETURN, SUPER, THIS, TRUEVAL, VAR, CONST, WHILE,
    
    // Runtime control
    ERROR, T_EOF
} TokenType;

typedef struct Token {
    TokenType type;
    const char* start;
    int length;
    int line;
} Token;

void init_scanner(const char* source);
Token scan_token(void);

#endif /* scanner_h */
