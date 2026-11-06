#include "compiler_internal.h"
#include "ely/core/chunk.h"
#include <ely/compiler/compiler.h>
#include <ely/core/opcode.h>
#include <ely/core/value.h>
#include <ely/compiler/parser.h>
#include <ely/compiler/token.h>

#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

/**
 * Parses a double floating-point value from a token slice.
 *
 * @param str Pointer to the start of the token characters.
 * @param length Length of the token lexeme.
 * @param out_value Pointer to the output double variable.
 * @return True if parsing succeeded without underflow/overflow; false otherwise.
 */
static inline bool parse_double(const char* str, size_t length, double* out_value) {
    assert(str != NULL && "String pointer is NULL.");
    assert(out_value != NULL && "Conversion output parameter is NULL.");

    char stack_buffer[64];
    char* target = stack_buffer;
    char* heap_buffer = NULL;

    if (length >= sizeof(stack_buffer)) {
        heap_buffer = (char*)malloc(length + 1);
        if (heap_buffer == NULL) return false;
        target = heap_buffer;
    }

    memcpy(target, str, length);
    target[length] = '\0';

    char* endptr;
    errno = 0;
    const double value = strtod(target, &endptr);

    const bool success = (endptr != target) && (*endptr == '\0') && (errno != ERANGE);
    if (heap_buffer != NULL) free(heap_buffer);

    if (success) {
        *out_value = value;
    }
    return success;
}

static inline void number(ElyCompiler* compiler) {
    assert(compiler != NULL && "Compiler pointer is NULL.");

    double value = 0.0;
    const ElyToken token = compiler->parser.previous;
    const ElyValue boxed_value = parse_double(token.lexeme, token.length, &value)
        ? ely_value_number(value)
        : ely_value_number(0.0);

    ely_compiler_emit_constant(compiler, boxed_value);
}

static inline void grouping(ElyCompiler* compiler) {
    assert(compiler != NULL && "Compiler pointer is NULL.");

    ely_compiler_expression(compiler);
    ely_parser_consume(&compiler->parser, ELY_TT_CLOSEPAREN, "Expected ')' to close grouping expression.");
}

static inline void unary(ElyCompiler* compiler) {
    assert(compiler != NULL && "Compiler pointer is NULL.");

    const ElyTokenType operator_type = compiler->parser.previous.token_type;
    ely_compiler_parse_precedence(compiler, ELY_PREC_UNARY);

    switch (operator_type) {
        case ELY_TT_MINUS: ely_compiler_emit_opcode(compiler, ELY_OP_NEGATE); break;
        case ELY_TT_BANG: ely_compiler_emit_opcode(compiler, ELY_OP_NOT); break;
        case ELY_TT_BWNOT: ely_compiler_emit_opcode(compiler, ELY_OP_BWNOT); break;
        default: return;
    }
}

static inline void emit_inc_dec(ElyCompiler* compiler, ElyOpcode get_op, ElyOpcode set_op, uint8_t arg,
                                bool is_inc, bool is_prefix) {
    if (is_prefix) {
        ely_compiler_emit_opcode(compiler, get_op);
        ely_compiler_emit_byte(compiler, arg);

        ely_compiler_emit_constant(compiler, ely_value_number(1.0));
        ely_compiler_emit_opcode(compiler, is_inc ? ELY_OP_ADD : ELY_OP_SUBTRACT);

        ely_compiler_emit_opcode(compiler, set_op);
        ely_compiler_emit_byte(compiler, arg);
    } else {
        // Postfix trick: push the current value twice.
        // The first copy stays on the stack as the expression result (old value),
        // while the second copy is modified and saved back into the variable.
        ely_compiler_emit_opcode(compiler, get_op);
        ely_compiler_emit_byte(compiler, arg);

        ely_compiler_emit_opcode(compiler, get_op);
        ely_compiler_emit_byte(compiler, arg);

        ely_compiler_emit_constant(compiler, ely_value_number(1.0));
        ely_compiler_emit_opcode(compiler, is_inc ? ELY_OP_ADD : ELY_OP_SUBTRACT);

        ely_compiler_emit_opcode(compiler, set_op);
        ely_compiler_emit_byte(compiler, arg);
        ely_compiler_emit_opcode(compiler, ELY_OP_POP);
    }
}

static inline void prefix_inc_dec(ElyCompiler* compiler) {
    assert(compiler != NULL && "Compiler pointer is NULL.");

    const bool is_inc = compiler->parser.previous.token_type == ELY_TT_PLUSPLUS;
    ely_parser_consume(&compiler->parser, ELY_TT_IDENTIFIER, "Expected variable identifier as operand for increment/decrement operator.");
    const ElyToken name = compiler->parser.previous;

    // Figure out if the variable is local or global once to avoid multiple scope lookups
    uint8_t slot = 0;
    ElyOpcode set_op, get_op;
    uint8_t arg;

    if (ely_compiler_resolve_local(compiler, &name, &slot)) {
        set_op = ELY_OP_SETLOCAL;
        get_op = ELY_OP_GETLOCAL;
        arg = slot;
    } else {
        set_op = ELY_OP_SETGLOBAL;
        get_op = ELY_OP_GETGLOBAL;
        arg = ely_compiler_identifier_constant(compiler, &name);
    }

    emit_inc_dec(compiler, get_op, set_op, arg, is_inc, true);
}

static inline void postfix_inc_dec(ElyCompiler* compiler, ElyOpcode get_op, ElyOpcode set_op, uint8_t arg) {
    assert(compiler != NULL && "Compiler pointer is NULL.");

    const bool is_inc = compiler->parser.previous.token_type == ELY_TT_PLUSPLUS;
    emit_inc_dec(compiler, get_op, set_op, arg, is_inc, false);
}

static inline void binary(ElyCompiler* compiler) {
    assert(compiler != NULL && "Compiler pointer is NULL.");

    const ElyTokenType operator_type = compiler->parser.previous.token_type;
    const ElyPrecedence precedence = ely_compiler_get_precedence(operator_type);
    ely_compiler_parse_precedence(compiler, (ElyPrecedence)(precedence + 1));

    switch (operator_type) {
        case ELY_TT_PLUS: ely_compiler_emit_opcode(compiler, ELY_OP_ADD); break;
        case ELY_TT_MINUS: ely_compiler_emit_opcode(compiler, ELY_OP_SUBTRACT); break;
        case ELY_TT_MULTIPLY: ely_compiler_emit_opcode(compiler, ELY_OP_MULTIPLY); break;
        case ELY_TT_DIVIDE: ely_compiler_emit_opcode(compiler, ELY_OP_DIVIDE); break;
        case ELY_TT_EQUALITY: ely_compiler_emit_opcode(compiler, ELY_OP_EQUAL); break;
        case ELY_TT_NONEQUALITY: {
            ely_compiler_emit_opcode(compiler, ELY_OP_EQUAL);
            ely_compiler_emit_opcode(compiler, ELY_OP_NOT);
            break;
        }
        case ELY_TT_GREATERTHAN: ely_compiler_emit_opcode(compiler, ELY_OP_GREATER); break;
        case ELY_TT_LESSTHAN: ely_compiler_emit_opcode(compiler, ELY_OP_LESS); break;
        case ELY_TT_GREATERTHANEQUAL: {
            ely_compiler_emit_opcode(compiler, ELY_OP_LESS);
            ely_compiler_emit_opcode(compiler, ELY_OP_NOT);
            break;
        }
        case ELY_TT_LESSTHANEQUAL: {
            ely_compiler_emit_opcode(compiler, ELY_OP_GREATER);
            ely_compiler_emit_opcode(compiler, ELY_OP_NOT);
            break;
        }
        case ELY_TT_BWAND: ely_compiler_emit_opcode(compiler, ELY_OP_BWAND); break;
        case ELY_TT_BWOR: ely_compiler_emit_opcode(compiler, ELY_OP_BWOR); break;
        case ELY_TT_BWXOR: ely_compiler_emit_opcode(compiler, ELY_OP_BWXOR); break;
        case ELY_TT_SHIFTLEFT: ely_compiler_emit_opcode(compiler, ELY_OP_SHIFTLEFT); break;
        case ELY_TT_SHIFTRIGHT: ely_compiler_emit_opcode(compiler, ELY_OP_SHIFTRIGHT); break;
        default: return;
    }
}

static inline void literal(ElyCompiler* compiler) {
    assert(compiler != NULL && "Compiler pointer is NULL.");

    switch (compiler->parser.previous.token_type) {
        case ELY_TT_TRUE: ely_compiler_emit_opcode(compiler, ELY_OP_TRUE); break;
        case ELY_TT_FALSE: ely_compiler_emit_opcode(compiler, ELY_OP_FALSE); break;
        case ELY_TT_NULL: ely_compiler_emit_opcode(compiler, ELY_OP_NOP); break;
        default: return;
    }
}

static inline void string(ElyCompiler* compiler) {
    assert(compiler != NULL && "Compiler pointer is NULL.");

    const ElyToken* token = &compiler->parser.previous;
    const size_t string_idx = ely_chunk_add_string_constant(
        &compiler->chunk,
        token->lexeme + 1,
        token->length - 2
    );

    ely_compiler_emit_constant(compiler, ely_value_obj((uint64_t)string_idx));
}

static inline void named_variable(ElyCompiler* compiler, const ElyToken* name, bool can_assign) {
    assert(compiler != NULL && "Compiler pointer is NULL.");
    assert(name != NULL && "Name token pointer is NULL.");

    uint8_t slot;
    ElyOpcode get_op, set_op;
    uint8_t arg;

    if (ely_compiler_resolve_local(compiler, name, &slot)) {
        get_op = ELY_OP_GETLOCAL;
        set_op = ELY_OP_SETLOCAL;
        arg = slot;
    } else {
        get_op = ELY_OP_GETGLOBAL;
        set_op = ELY_OP_SETGLOBAL;
        arg = ely_compiler_identifier_constant(compiler, name);
    }

    if (can_assign && ely_parser_match_token(&compiler->parser, ELY_TT_ASSIGN)) {
        ely_compiler_expression(compiler);
        ely_compiler_emit_opcode(compiler, set_op);
        ely_compiler_emit_byte(compiler, arg);
    } else if (can_assign
        && (ely_parser_match_token(&compiler->parser, ELY_TT_PLUSASSIGN)
            || ely_parser_match_token(&compiler->parser, ELY_TT_MINUSASSIGN)
            || ely_parser_match_token(&compiler->parser, ELY_TT_MULTIPLYASSIGN)
            || ely_parser_match_token(&compiler->parser, ELY_TT_DIVIDEASSIGN))) {

        // Save the operator
        const ElyTokenType op = compiler->parser.previous.token_type;

        // Read value
        ely_compiler_emit_opcode(compiler, get_op);
        ely_compiler_emit_byte(compiler, arg);

        ely_compiler_expression(compiler);

        // Emit opcode based on operator type
        switch (op) {
            case ELY_TT_PLUSASSIGN: ely_compiler_emit_opcode(compiler, ELY_OP_ADD); break;
            case ELY_TT_MINUSASSIGN: ely_compiler_emit_opcode(compiler, ELY_OP_SUBTRACT); break;
            case ELY_TT_MULTIPLYASSIGN: ely_compiler_emit_opcode(compiler, ELY_OP_MULTIPLY); break;
            case ELY_TT_DIVIDEASSIGN: ely_compiler_emit_opcode(compiler, ELY_OP_DIVIDE); break;
            default: break;
        }

        // Store result
        ely_compiler_emit_opcode(compiler, set_op);
        ely_compiler_emit_byte(compiler, arg);
    } else if (ely_parser_match_token(&compiler->parser, ELY_TT_PLUSPLUS)
        || ely_parser_match_token(&compiler->parser, ELY_TT_MINUSMINUS)) {
        postfix_inc_dec(compiler, get_op, set_op, arg);
    } else {
        ely_compiler_emit_opcode(compiler, get_op);
        ely_compiler_emit_byte(compiler, arg);
    }
}

static inline void variable(ElyCompiler* compiler, bool can_assign) {
    assert(compiler != NULL && "Compiler pointer is NULL.");
    named_variable(compiler, &compiler->parser.previous, can_assign);
}

static inline void and_op(ElyCompiler* compiler) {
    assert(compiler != NULL && "Compiler pointer is NULL.");

    const size_t end_jump = ely_compiler_emit_jump(compiler, ELY_OP_JUMPZERO);
    ely_compiler_emit_opcode(compiler, ELY_OP_POP);
    ely_compiler_parse_precedence(compiler, ELY_PREC_AND);
    ely_compiler_patch_jump(compiler, end_jump);
}

static inline void or_op(ElyCompiler* compiler) {
    assert(compiler != NULL && "Compiler pointer is NULL.");

    const size_t else_jump = ely_compiler_emit_jump(compiler, ELY_OP_JUMPZERO);
    const size_t end_jump = ely_compiler_emit_jump(compiler, ELY_OP_JUMP);
    ely_compiler_patch_jump(compiler, else_jump);
    ely_compiler_emit_opcode(compiler, ELY_OP_POP);
    ely_compiler_parse_precedence(compiler, ELY_PREC_OR);
    ely_compiler_patch_jump(compiler, end_jump);
}

ELY_NODISCARD static inline uint8_t argument_list(ElyCompiler* compiler) {
    assert(compiler != NULL && "Compiler pointer is NULL.");

    uint8_t arg_count = 0;

    if (!ely_parser_check(&compiler->parser, ELY_TT_CLOSEPAREN)) {
        while (true) {
            ely_compiler_expression(compiler);

            if (arg_count == ELY_ARGS_MAX) {
                ely_parser_error(&compiler->parser, "Argument count limit exceeded: call site cannot exceed 255 arguments.");
            }

            arg_count++;

            if (!ely_parser_match_token(&compiler->parser, ELY_TT_COMMA)) break;
        }
    }

    ely_parser_consume(&compiler->parser, ELY_TT_CLOSEPAREN, "Expected closing ')' after function arguments.");
    return arg_count;
}

static inline void call(ElyCompiler* compiler) {
    assert(compiler != NULL && "Compiler pointer is NULL.");

    const uint8_t arg_count = argument_list(compiler);
    ely_compiler_emit_opcode(compiler, ELY_OP_CALL);
    ely_compiler_emit_byte(compiler, arg_count);
}

/**
 * Parses a top-level expression starting at assignment precedence.
 */
void ely_compiler_expression(ElyCompiler* compiler) {
    assert(compiler != NULL && "Compiler pointer is NULL.");
    ely_compiler_parse_precedence(compiler, ELY_PREC_ASSIGNMENT);
}

/**
 * Pratt parser driver: parses prefix rules, followed by infix rules whose
 * precedence is equal to or greater than the given threshold.
 */
void ely_compiler_parse_precedence(ElyCompiler* compiler, ElyPrecedence precedence) {
    assert(compiler != NULL && "Compiler pointer is NULL.");

    ely_parser_advance(&compiler->parser);

    const bool can_assign = precedence <= ELY_PREC_ASSIGNMENT;
    switch (compiler->parser.previous.token_type) {
        case ELY_TT_OPENPAREN: grouping(compiler); break;
        case ELY_TT_MINUS:
        case ELY_TT_BANG:
        case ELY_TT_BWNOT: unary(compiler); break;
        case ELY_TT_PLUSPLUS:
        case ELY_TT_MINUSMINUS: prefix_inc_dec(compiler); break;
        case ELY_TT_FALSE:
        case ELY_TT_TRUE:
        case ELY_TT_NULL: literal(compiler); break;
        case ELY_TT_NUMBER: number(compiler); break;
        case ELY_TT_STRING: string(compiler); break;
        case ELY_TT_IDENTIFIER: variable(compiler, can_assign); break;
        default: {
            ely_parser_error(&compiler->parser, "Syntax error: expected expression.");
            return;
        }
    }

    while (precedence <= ely_compiler_get_precedence(compiler->parser.current.token_type)) {
        ely_parser_advance(&compiler->parser);
        switch (compiler->parser.previous.token_type) {
            case ELY_TT_AND: and_op(compiler); break;
            case ELY_TT_OR: or_op(compiler); break;
            case ELY_TT_OPENPAREN: call(compiler); break;
            default: binary(compiler); break;
        }
    }

    if (can_assign
        && (ely_parser_match_token(&compiler->parser, ELY_TT_ASSIGN)
            || ely_parser_match_token(&compiler->parser, ELY_TT_PLUSASSIGN)
            || ely_parser_match_token(&compiler->parser, ELY_TT_MINUSASSIGN)
            || ely_parser_match_token(&compiler->parser, ELY_TT_MULTIPLYASSIGN)
            || ely_parser_match_token(&compiler->parser, ELY_TT_DIVIDEASSIGN))) {
        ely_parser_error(&compiler->parser, "Invalid target for assignment expression.");
    }
}

/**
 * Maps a token type to its infix precedence level.
 */
ElyPrecedence ely_compiler_get_precedence(ElyTokenType token_type) {
    switch (token_type) {
        case ELY_TT_MULTIPLY:
        case ELY_TT_DIVIDE: return ELY_PREC_FACTOR;
        case ELY_TT_PLUS:
        case ELY_TT_MINUS: return ELY_PREC_TERM;
        case ELY_TT_LESSTHAN:
        case ELY_TT_LESSTHANEQUAL:
        case ELY_TT_GREATERTHAN:
        case ELY_TT_GREATERTHANEQUAL: return ELY_PREC_COMPARISON;
        case ELY_TT_EQUALITY:
        case ELY_TT_NONEQUALITY: return ELY_PREC_EQUALITY;
        case ELY_TT_BWOR:
        case ELY_TT_BWXOR:
        case ELY_TT_BWAND:
        case ELY_TT_SHIFTLEFT:
        case ELY_TT_SHIFTRIGHT: return ELY_PREC_TERM;
        case ELY_TT_AND: return ELY_PREC_AND;
        case ELY_TT_OR: return ELY_PREC_OR;
        case ELY_TT_OPENPAREN: return ELY_PREC_CALL;
        default: return ELY_PREC_NONE;
    }
}
