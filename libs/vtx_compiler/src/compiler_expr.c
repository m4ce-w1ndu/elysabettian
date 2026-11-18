#include "compiler_internal.h"
#include "vtx/core/chunk.h"
#include <vtx/compiler/compiler.h>
#include <vtx/core/opcode.h>
#include <vtx/core/value.h>
#include <vtx/compiler/parser.h>
#include <vtx/compiler/token.h>

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

static inline void number(VtxCompiler* compiler) {
    assert(compiler != NULL && "Compiler pointer is NULL.");

    double value = 0.0;
    const VtxToken token = compiler->parser.previous;
    const VtxValue boxed_value = parse_double(token.lexeme, token.length, &value)
        ? vtx_value_number(value)
        : vtx_value_number(0.0);

    vtx_compiler_emit_constant(compiler, boxed_value);
}

static inline void grouping(VtxCompiler* compiler) {
    assert(compiler != NULL && "Compiler pointer is NULL.");

    vtx_compiler_expression(compiler);
    vtx_parser_consume(&compiler->parser, VTX_TT_CLOSEPAREN, "Expected ')' to close grouping expression.");
}

static inline void unary(VtxCompiler* compiler) {
    assert(compiler != NULL && "Compiler pointer is NULL.");

    const VtxTokenType operator_type = compiler->parser.previous.token_type;
    vtx_compiler_parse_precedence(compiler, VTX_PREC_UNARY);

    switch (operator_type) {
        case VTX_TT_MINUS: vtx_compiler_emit_opcode(compiler, VTX_OP_NEGATE); break;
        case VTX_TT_BANG: vtx_compiler_emit_opcode(compiler, VTX_OP_NOT); break;
        case VTX_TT_BWNOT: vtx_compiler_emit_opcode(compiler, VTX_OP_BWNOT); break;
        default: return;
    }
}

static inline void emit_inc_dec(VtxCompiler* compiler, VtxOpcode get_op, VtxOpcode set_op, uint8_t arg,
                                bool is_inc, bool is_prefix) {
    if (is_prefix) {
        vtx_compiler_emit_opcode(compiler, get_op);
        vtx_compiler_emit_byte(compiler, arg);

        vtx_compiler_emit_constant(compiler, vtx_value_number(1.0));
        vtx_compiler_emit_opcode(compiler, is_inc ? VTX_OP_ADD : VTX_OP_SUBTRACT);

        vtx_compiler_emit_opcode(compiler, set_op);
        vtx_compiler_emit_byte(compiler, arg);
    } else {
        // Postfix trick: push the current value twice.
        // The first copy stays on the stack as the expression result (old value),
        // while the second copy is modified and saved back into the variable.
        vtx_compiler_emit_opcode(compiler, get_op);
        vtx_compiler_emit_byte(compiler, arg);

        vtx_compiler_emit_opcode(compiler, get_op);
        vtx_compiler_emit_byte(compiler, arg);

        vtx_compiler_emit_constant(compiler, vtx_value_number(1.0));
        vtx_compiler_emit_opcode(compiler, is_inc ? VTX_OP_ADD : VTX_OP_SUBTRACT);

        vtx_compiler_emit_opcode(compiler, set_op);
        vtx_compiler_emit_byte(compiler, arg);
        vtx_compiler_emit_opcode(compiler, VTX_OP_POP);
    }
}

static inline void prefix_inc_dec(VtxCompiler* compiler) {
    assert(compiler != NULL && "Compiler pointer is NULL.");

    const bool is_inc = compiler->parser.previous.token_type == VTX_TT_PLUSPLUS;
    vtx_parser_consume(&compiler->parser, VTX_TT_IDENTIFIER, "Expected variable identifier as operand for increment/decrement operator.");
    const VtxToken name = compiler->parser.previous;

    // Figure out if the variable is local or global once to avoid multiple scope lookups
    uint8_t slot = 0;
    VtxOpcode set_op, get_op;
    uint8_t arg;

    if (vtx_compiler_resolve_local(compiler, &name, &slot)) {
        set_op = VTX_OP_SETLOCAL;
        get_op = VTX_OP_GETLOCAL;
        arg = slot;
    } else {
        set_op = VTX_OP_SETGLOBAL;
        get_op = VTX_OP_GETGLOBAL;
        arg = vtx_compiler_identifier_constant(compiler, &name);
    }

    emit_inc_dec(compiler, get_op, set_op, arg, is_inc, true);
}

static inline void postfix_inc_dec(VtxCompiler* compiler, VtxOpcode get_op, VtxOpcode set_op, uint8_t arg) {
    assert(compiler != NULL && "Compiler pointer is NULL.");

    const bool is_inc = compiler->parser.previous.token_type == VTX_TT_PLUSPLUS;
    emit_inc_dec(compiler, get_op, set_op, arg, is_inc, false);
}

static inline void binary(VtxCompiler* compiler) {
    assert(compiler != NULL && "Compiler pointer is NULL.");

    const VtxTokenType operator_type = compiler->parser.previous.token_type;
    const VtxPrecedence precedence = vtx_compiler_get_precedence(operator_type);
    vtx_compiler_parse_precedence(compiler, (VtxPrecedence)(precedence + 1));

    switch (operator_type) {
        case VTX_TT_PLUS: vtx_compiler_emit_opcode(compiler, VTX_OP_ADD); break;
        case VTX_TT_MINUS: vtx_compiler_emit_opcode(compiler, VTX_OP_SUBTRACT); break;
        case VTX_TT_MULTIPLY: vtx_compiler_emit_opcode(compiler, VTX_OP_MULTIPLY); break;
        case VTX_TT_DIVIDE: vtx_compiler_emit_opcode(compiler, VTX_OP_DIVIDE); break;
        case VTX_TT_EQUALITY: vtx_compiler_emit_opcode(compiler, VTX_OP_EQUAL); break;
        case VTX_TT_NONEQUALITY: {
            vtx_compiler_emit_opcode(compiler, VTX_OP_EQUAL);
            vtx_compiler_emit_opcode(compiler, VTX_OP_NOT);
            break;
        }
        case VTX_TT_GREATERTHAN: vtx_compiler_emit_opcode(compiler, VTX_OP_GREATER); break;
        case VTX_TT_LESSTHAN: vtx_compiler_emit_opcode(compiler, VTX_OP_LESS); break;
        case VTX_TT_GREATERTHANEQUAL: {
            vtx_compiler_emit_opcode(compiler, VTX_OP_LESS);
            vtx_compiler_emit_opcode(compiler, VTX_OP_NOT);
            break;
        }
        case VTX_TT_LESSTHANEQUAL: {
            vtx_compiler_emit_opcode(compiler, VTX_OP_GREATER);
            vtx_compiler_emit_opcode(compiler, VTX_OP_NOT);
            break;
        }
        case VTX_TT_BWAND: vtx_compiler_emit_opcode(compiler, VTX_OP_BWAND); break;
        case VTX_TT_BWOR: vtx_compiler_emit_opcode(compiler, VTX_OP_BWOR); break;
        case VTX_TT_BWXOR: vtx_compiler_emit_opcode(compiler, VTX_OP_BWXOR); break;
        case VTX_TT_SHIFTLEFT: vtx_compiler_emit_opcode(compiler, VTX_OP_SHIFTLEFT); break;
        case VTX_TT_SHIFTRIGHT: vtx_compiler_emit_opcode(compiler, VTX_OP_SHIFTRIGHT); break;
        default: return;
    }
}

static inline void literal(VtxCompiler* compiler) {
    assert(compiler != NULL && "Compiler pointer is NULL.");

    switch (compiler->parser.previous.token_type) {
        case VTX_TT_TRUE: vtx_compiler_emit_opcode(compiler, VTX_OP_TRUE); break;
        case VTX_TT_FALSE: vtx_compiler_emit_opcode(compiler, VTX_OP_FALSE); break;
        case VTX_TT_NULL: vtx_compiler_emit_opcode(compiler, VTX_OP_NOP); break;
        default: return;
    }
}

static inline void string(VtxCompiler* compiler) {
    assert(compiler != NULL && "Compiler pointer is NULL.");

    const VtxToken* token = &compiler->parser.previous;
    const size_t string_idx = vtx_chunk_add_string_constant(
        &compiler->chunk,
        token->lexeme + 1,
        token->length - 2
    );

    vtx_compiler_emit_constant(compiler, vtx_value_obj((uint64_t)string_idx));
}

static inline void named_variable(VtxCompiler* compiler, const VtxToken* name, bool can_assign) {
    assert(compiler != NULL && "Compiler pointer is NULL.");
    assert(name != NULL && "Name token pointer is NULL.");

    uint8_t slot;
    VtxOpcode get_op, set_op;
    uint8_t arg;

    if (vtx_compiler_resolve_local(compiler, name, &slot)) {
        get_op = VTX_OP_GETLOCAL;
        set_op = VTX_OP_SETLOCAL;
        arg = slot;
    } else {
        get_op = VTX_OP_GETGLOBAL;
        set_op = VTX_OP_SETGLOBAL;
        arg = vtx_compiler_identifier_constant(compiler, name);
    }

    if (can_assign && vtx_parser_match_token(&compiler->parser, VTX_TT_ASSIGN)) {
        vtx_compiler_expression(compiler);
        vtx_compiler_emit_opcode(compiler, set_op);
        vtx_compiler_emit_byte(compiler, arg);
    } else if (can_assign
        && (vtx_parser_match_token(&compiler->parser, VTX_TT_PLUSASSIGN)
            || vtx_parser_match_token(&compiler->parser, VTX_TT_MINUSASSIGN)
            || vtx_parser_match_token(&compiler->parser, VTX_TT_MULTIPLYASSIGN)
            || vtx_parser_match_token(&compiler->parser, VTX_TT_DIVIDEASSIGN))) {

        // Save the operator
        const VtxTokenType op = compiler->parser.previous.token_type;

        // Read value
        vtx_compiler_emit_opcode(compiler, get_op);
        vtx_compiler_emit_byte(compiler, arg);

        vtx_compiler_expression(compiler);

        // Emit opcode based on operator type
        switch (op) {
            case VTX_TT_PLUSASSIGN: vtx_compiler_emit_opcode(compiler, VTX_OP_ADD); break;
            case VTX_TT_MINUSASSIGN: vtx_compiler_emit_opcode(compiler, VTX_OP_SUBTRACT); break;
            case VTX_TT_MULTIPLYASSIGN: vtx_compiler_emit_opcode(compiler, VTX_OP_MULTIPLY); break;
            case VTX_TT_DIVIDEASSIGN: vtx_compiler_emit_opcode(compiler, VTX_OP_DIVIDE); break;
            default: break;
        }

        // Store result
        vtx_compiler_emit_opcode(compiler, set_op);
        vtx_compiler_emit_byte(compiler, arg);
    } else if (vtx_parser_match_token(&compiler->parser, VTX_TT_PLUSPLUS)
        || vtx_parser_match_token(&compiler->parser, VTX_TT_MINUSMINUS)) {
        postfix_inc_dec(compiler, get_op, set_op, arg);
    } else {
        vtx_compiler_emit_opcode(compiler, get_op);
        vtx_compiler_emit_byte(compiler, arg);
    }
}

static inline void variable(VtxCompiler* compiler, bool can_assign) {
    assert(compiler != NULL && "Compiler pointer is NULL.");
    named_variable(compiler, &compiler->parser.previous, can_assign);
}

static inline void and_op(VtxCompiler* compiler) {
    assert(compiler != NULL && "Compiler pointer is NULL.");

    const size_t end_jump = vtx_compiler_emit_jump(compiler, VTX_OP_JUMPZERO);
    vtx_compiler_emit_opcode(compiler, VTX_OP_POP);
    vtx_compiler_parse_precedence(compiler, VTX_PREC_AND);
    vtx_compiler_patch_jump(compiler, end_jump);
}

static inline void or_op(VtxCompiler* compiler) {
    assert(compiler != NULL && "Compiler pointer is NULL.");

    const size_t else_jump = vtx_compiler_emit_jump(compiler, VTX_OP_JUMPZERO);
    const size_t end_jump = vtx_compiler_emit_jump(compiler, VTX_OP_JUMP);
    vtx_compiler_patch_jump(compiler, else_jump);
    vtx_compiler_emit_opcode(compiler, VTX_OP_POP);
    vtx_compiler_parse_precedence(compiler, VTX_PREC_OR);
    vtx_compiler_patch_jump(compiler, end_jump);
}

VTX_NODISCARD static inline uint8_t argument_list(VtxCompiler* compiler) {
    assert(compiler != NULL && "Compiler pointer is NULL.");

    uint8_t arg_count = 0;

    if (!vtx_parser_check(&compiler->parser, VTX_TT_CLOSEPAREN)) {
        while (true) {
            vtx_compiler_expression(compiler);

            if (arg_count == VTX_ARGS_MAX) {
                vtx_parser_error(&compiler->parser, "Argument count limit exceeded: call site cannot exceed 255 arguments.");
            }

            arg_count++;

            if (!vtx_parser_match_token(&compiler->parser, VTX_TT_COMMA)) break;
        }
    }

    vtx_parser_consume(&compiler->parser, VTX_TT_CLOSEPAREN, "Expected closing ')' after function arguments.");
    return arg_count;
}

static inline void call(VtxCompiler* compiler) {
    assert(compiler != NULL && "Compiler pointer is NULL.");

    const uint8_t arg_count = argument_list(compiler);
    vtx_compiler_emit_opcode(compiler, VTX_OP_CALL);
    vtx_compiler_emit_byte(compiler, arg_count);
}

/**
 * Parses a top-level expression starting at assignment precedence.
 */
void vtx_compiler_expression(VtxCompiler* compiler) {
    assert(compiler != NULL && "Compiler pointer is NULL.");
    vtx_compiler_parse_precedence(compiler, VTX_PREC_ASSIGNMENT);
}

/**
 * Pratt parser driver: parses prefix rules, followed by infix rules whose
 * precedence is equal to or greater than the given threshold.
 */
void vtx_compiler_parse_precedence(VtxCompiler* compiler, VtxPrecedence precedence) {
    assert(compiler != NULL && "Compiler pointer is NULL.");

    vtx_parser_advance(&compiler->parser);

    const bool can_assign = precedence <= VTX_PREC_ASSIGNMENT;
    switch (compiler->parser.previous.token_type) {
        case VTX_TT_OPENPAREN: grouping(compiler); break;
        case VTX_TT_MINUS:
        case VTX_TT_BANG:
        case VTX_TT_BWNOT: unary(compiler); break;
        case VTX_TT_PLUSPLUS:
        case VTX_TT_MINUSMINUS: prefix_inc_dec(compiler); break;
        case VTX_TT_FALSE:
        case VTX_TT_TRUE:
        case VTX_TT_NULL: literal(compiler); break;
        case VTX_TT_NUMBER: number(compiler); break;
        case VTX_TT_STRING: string(compiler); break;
        case VTX_TT_IDENTIFIER: variable(compiler, can_assign); break;
        default: {
            vtx_parser_error(&compiler->parser, "Syntax error: expected expression.");
            return;
        }
    }

    while (precedence <= vtx_compiler_get_precedence(compiler->parser.current.token_type)) {
        vtx_parser_advance(&compiler->parser);
        switch (compiler->parser.previous.token_type) {
            case VTX_TT_AND: and_op(compiler); break;
            case VTX_TT_OR: or_op(compiler); break;
            case VTX_TT_OPENPAREN: call(compiler); break;
            default: binary(compiler); break;
        }
    }

    if (can_assign
        && (vtx_parser_match_token(&compiler->parser, VTX_TT_ASSIGN)
            || vtx_parser_match_token(&compiler->parser, VTX_TT_PLUSASSIGN)
            || vtx_parser_match_token(&compiler->parser, VTX_TT_MINUSASSIGN)
            || vtx_parser_match_token(&compiler->parser, VTX_TT_MULTIPLYASSIGN)
            || vtx_parser_match_token(&compiler->parser, VTX_TT_DIVIDEASSIGN))) {
        vtx_parser_error(&compiler->parser, "Invalid target for assignment expression.");
    }
}

/**
 * Maps a token type to its infix precedence level.
 */
VtxPrecedence vtx_compiler_get_precedence(VtxTokenType token_type) {
    switch (token_type) {
        case VTX_TT_MULTIPLY:
        case VTX_TT_DIVIDE: return VTX_PREC_FACTOR;
        case VTX_TT_PLUS:
        case VTX_TT_MINUS: return VTX_PREC_TERM;
        case VTX_TT_LESSTHAN:
        case VTX_TT_LESSTHANEQUAL:
        case VTX_TT_GREATERTHAN:
        case VTX_TT_GREATERTHANEQUAL: return VTX_PREC_COMPARISON;
        case VTX_TT_EQUALITY:
        case VTX_TT_NONEQUALITY: return VTX_PREC_EQUALITY;
        case VTX_TT_BWOR:
        case VTX_TT_BWXOR:
        case VTX_TT_BWAND:
        case VTX_TT_SHIFTLEFT:
        case VTX_TT_SHIFTRIGHT: return VTX_PREC_TERM;
        case VTX_TT_AND: return VTX_PREC_AND;
        case VTX_TT_OR: return VTX_PREC_OR;
        case VTX_TT_OPENPAREN: return VTX_PREC_CALL;
        default: return VTX_PREC_NONE;
    }
}
