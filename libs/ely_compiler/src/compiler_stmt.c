#include "compiler_internal.h"
#include <ely/core/chunk.h>
#include <ely/core/opcode.h>
#include <ely/compiler/parser.h>
#include <ely/compiler/token.h>
#include <ely/compiler/compiler.h>

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static inline void var_declaration(ElyCompiler* compiler);
static inline void function_declaration(ElyCompiler* compiler);
static inline void import_statement(ElyCompiler* compiler);
static inline void synchronize(ElyCompiler* compiler);
static inline void define_variable(ElyCompiler* compiler, uint8_t global);
static inline uint8_t parse_variable(ElyCompiler* compiler, const char* error_message);

static inline void print_statement(ElyCompiler* compiler) {
    assert(compiler != NULL && "Compiler pointer is NULL.");

    ely_compiler_expression(compiler);
    ely_parser_consume(&compiler->parser, ELY_TT_SEMICOLON, "Expected ';' to terminate print statement.");
    ely_compiler_emit_opcode(compiler, ELY_OP_PRINT);
}

static inline void expression_statement(ElyCompiler* compiler) {
    assert(compiler != NULL && "Compiler pointer is NULL.");

    ely_compiler_expression(compiler);
    ely_parser_consume(&compiler->parser, ELY_TT_SEMICOLON, "Expected ';' to terminate expression statement.");
    ely_compiler_emit_opcode(compiler, ELY_OP_POP);
}

static inline void declaration(ElyCompiler* compiler) {
    assert(compiler != NULL && "Compiler pointer is NULL.");

    if (ely_parser_match_token(&compiler->parser, ELY_TT_VAR)) {
        var_declaration(compiler);
    } else if (ely_parser_match_token(&compiler->parser, ELY_TT_FUNC)) {
        function_declaration(compiler);
    } else {
        ely_compiler_statement(compiler);
    }

    if (compiler->parser.panic_mode) {
        synchronize(compiler);
    }
}

static inline void import_statement(ElyCompiler* compiler) {
    assert(compiler != NULL && "Compiler is not NULL.");

    // Parentheses are optional to support both function-like and statement-like import system.
    const bool has_paren = ely_parser_match_token(&compiler->parser, ELY_TT_OPENPAREN);
    ely_parser_consume(
        &compiler->parser,
        ELY_TT_STRING,
        "Expected module name string literal following 'import' keyword."
    );

    // Strip quotes from the module name literal
    const ElyToken* name_token = &compiler->parser.previous;
    const char* module_str = name_token->length >= 2 ? name_token->lexeme + 1 : "";
    const size_t module_len = name_token->length >= 2 ? name_token->length - 2 : 0;

    const size_t string_idx = ely_chunk_add_string_constant(
        &compiler->chunk,
        module_str,
        module_len
    );

    if (has_paren) {
        ely_parser_consume(
            &compiler->parser,
            ELY_TT_CLOSEPAREN,
            "Expected closing ')' after module name in 'import' statement."
        );
    }

    ely_parser_consume(
        &compiler->parser,
        ELY_TT_SEMICOLON,
        "Expected ';' to terminate import statement."
    );

    ely_compiler_emit_opcode(compiler, ELY_OP_IMPORT);
    ely_compiler_emit_byte(compiler, (uint8_t)string_idx);
}

static inline void define_variable(ElyCompiler* compiler, uint8_t global) {
    assert(compiler != NULL && "Compiler is not NULL.");

    if (compiler->context.scope_depth > 0) {
        ely_compiler_mark_initialized(compiler);
        const uint8_t slot = (uint8_t)(compiler->context.locals_count - 1);
        ely_compiler_emit_opcode(compiler, ELY_OP_SETLOCAL);
        ely_compiler_emit_byte(compiler, slot);
        return;
    }

    ely_compiler_emit_opcode(compiler, ELY_OP_DEFINEGLOBAL);
    ely_compiler_emit_byte(compiler, global);
}

static inline void var_declaration(ElyCompiler* compiler) {
    assert(compiler != NULL && "Compiler is not NULL.");

    const uint8_t global = parse_variable(compiler, "Expected variable identifier in declaration.");

    if (ely_parser_match_token(&compiler->parser, ELY_TT_ASSIGN)) {
        ely_compiler_expression(compiler);
    } else {
        ely_compiler_emit_opcode(compiler, ELY_OP_NOP);
    }

    ely_parser_consume(
        &compiler->parser,
        ELY_TT_SEMICOLON,
        "Expected ';' to terminate variable declaration."
    );

    define_variable(compiler, global);
}

static inline void synchronize(ElyCompiler* compiler) {
    assert(compiler != NULL && "Compiler is not NULL.");

    ely_parser_reset_panic_mode(&compiler->parser);

    while (compiler->parser.current.token_type != ELY_TT_EOF) {
        if (compiler->parser.previous.token_type == ELY_TT_SEMICOLON) return;

        switch (compiler->parser.current.token_type) {
            case ELY_TT_CLASS:
            case ELY_TT_FUNC:
            case ELY_TT_VAR:
            case ELY_TT_FOR:
            case ELY_TT_IF:
            case ELY_TT_WHILE:
            case ELY_TT_PRINT:
            case ELY_TT_RETURN: return;
            default: break;
        }

        ely_parser_advance(&compiler->parser);
    }
}

static inline void block(ElyCompiler* compiler) {
    assert(compiler != NULL && "Compiler is not NULL.");

    while (compiler->parser.current.token_type != ELY_TT_CLOSECURLY &&
            compiler->parser.current.token_type != ELY_TT_EOF) {
        declaration(compiler);
    }

    ely_parser_consume(
        &compiler->parser,
        ELY_TT_CLOSECURLY,
        "Expected '}' to close block."
    );
}

static inline void if_statement(ElyCompiler* compiler) {
    assert(compiler != NULL && "Compiler is not NULL.");

    ely_parser_consume(&compiler->parser, ELY_TT_OPENPAREN,
        "Expected '(' to open 'if' condition.");
    ely_compiler_expression(compiler);
    ely_parser_consume(&compiler->parser, ELY_TT_CLOSEPAREN,
        "Expected ')' to close 'if' condition.");

    const size_t then_jump = ely_compiler_emit_jump(compiler, ELY_OP_JUMPZERO);
    ely_compiler_emit_opcode(compiler, ELY_OP_POP);
    ely_compiler_statement(compiler);

    const size_t else_jump = ely_compiler_emit_jump(compiler, ELY_OP_JUMP);
    ely_compiler_patch_jump(compiler, then_jump);
    ely_compiler_emit_opcode(compiler, ELY_OP_POP);

    if (ely_parser_match_token(&compiler->parser, ELY_TT_ELSE)) {
        ely_compiler_statement(compiler);
    }

    ely_compiler_patch_jump(compiler, else_jump);
}

static inline void while_statement(ElyCompiler* compiler) {
    assert(compiler != NULL && "Compiler is not NULL.");

    const size_t loop_start = compiler->chunk.code_count;
    ely_parser_consume(&compiler->parser, ELY_TT_OPENPAREN,
        "Expected '(' to open 'while' condition.");
    ely_compiler_expression(compiler);
    ely_parser_consume(&compiler->parser, ELY_TT_CLOSEPAREN,
        "Expected ')' to close 'while' condition.");

    const size_t exit_jump = ely_compiler_emit_jump(compiler, ELY_OP_JUMPZERO);
    ely_compiler_emit_opcode(compiler, ELY_OP_POP);

    ely_compiler_statement(compiler);
    ely_compiler_emit_loop(compiler, loop_start);

    ely_compiler_patch_jump(compiler, exit_jump);
    ely_compiler_emit_opcode(compiler, ELY_OP_POP);
}

static inline void for_statement(ElyCompiler* compiler) {
    assert(compiler != NULL && "Compiler is not NULL.");

    ely_compiler_begin_scope(compiler);
    ely_parser_consume(&compiler->parser, ELY_TT_OPENPAREN,
        "Expecter '(' to open 'for' condition.");

    if (ely_parser_match_token(&compiler->parser, ELY_TT_SEMICOLON)) {
        ;
    } else if (ely_parser_match_token(&compiler->parser, ELY_TT_VAR)) {
        var_declaration(compiler);
    } else {
        expression_statement(compiler);
    }

    size_t loop_start = compiler->chunk.code_count;
    size_t exit_jump = 0;
    if (!ely_parser_match_token(&compiler->parser, ELY_TT_SEMICOLON)) {
        ely_compiler_expression(compiler);
        ely_parser_consume(&compiler->parser, ELY_TT_SEMICOLON,
            "Expected ';' to terminate 'for' conditional.");
        exit_jump = ely_compiler_emit_jump(compiler, ELY_OP_JUMPZERO);
        ely_compiler_emit_opcode(compiler, ELY_OP_POP);
    }

    if (!ely_parser_match_token(&compiler->parser, ELY_TT_CLOSEPAREN)) {
        const size_t body_jump = ely_compiler_emit_jump(compiler, ELY_OP_JUMP);
        const size_t increment_start = compiler->chunk.code_count;
        ely_compiler_expression(compiler);
        ely_compiler_emit_opcode(compiler, ELY_OP_POP);

        ely_parser_consume(&compiler->parser, ELY_TT_CLOSEPAREN,
            "Expected ')' to close 'for' conditional clauses.");
        ely_compiler_emit_loop(compiler, loop_start);
        loop_start = increment_start;
        ely_compiler_patch_jump(compiler, body_jump);
    }

    ely_compiler_statement(compiler);
    ely_compiler_emit_loop(compiler, loop_start);

    if (exit_jump) {
        ely_compiler_patch_jump(compiler, exit_jump);
        ely_compiler_emit_opcode(compiler, ELY_OP_POP);
    }

    ely_compiler_end_scope(compiler);
}

ELY_NODISCARD static inline size_t function(ElyCompiler* compiler) {
    assert(compiler != NULL && "Compiler is not NULL.");

    ely_compiler_begin_scope(compiler);

    size_t arity = 0;
    if (!ely_parser_check(&compiler->parser, ELY_TT_CLOSEPAREN)) {
        while (true) {
            arity++;
            if (arity > ELY_ARGS_MAX) {
                ely_parser_error(&compiler->parser,
                    "Parameter count limit exceeded: function declaration cannot exceed 255 parameters.");
                return arity;
            }

            (void)parse_variable(compiler, "Expected parameter identifier in function signature.");
            ely_compiler_mark_initialized(compiler);

            if (!ely_parser_match_token(&compiler->parser, ELY_TT_COMMA)) break;
        }
    }

    ely_parser_consume(&compiler->parser, ELY_TT_CLOSEPAREN, "Expected ')' to close parameter list.");
    ely_parser_consume(&compiler->parser, ELY_TT_OPENCURLY, "Expected '{' to open function body.");

    block(compiler);
    ely_compiler_end_scope(compiler);
    ely_compiler_emit_return(compiler);

    return arity;

}

static inline void return_statement(ElyCompiler* compiler) {
    assert(compiler != NULL && "Compiler is not NULL.");

    if (compiler->context.function_type == ELY_FT_SCRIPT) {
        ely_parser_error(&compiler->parser, "Cannot return from top-level script code.");
    }

    if (ely_parser_match_token(&compiler->parser, ELY_TT_SEMICOLON)) {
        ely_compiler_emit_return(compiler);
    } else {
        ely_compiler_expression(compiler);
        ely_parser_consume(&compiler->parser, ELY_TT_SEMICOLON, "Expected ';' to terminate return statement.");
        ely_compiler_emit_opcode(compiler, ELY_OP_RETURN);
    }
}

static inline void function_declaration(ElyCompiler* compiler) {
    assert(compiler != NULL && "Compiler is not NULL.");

    const uint8_t global = parse_variable(compiler, "Expected function identifier in function declaration.");
    const ElyToken name_token = compiler->parser.previous;

    if (compiler->context.scope_depth > 0) {
        ely_compiler_mark_initialized(compiler);
    }

    ElyChunk prev_chunk = compiler->chunk;
    ElyCompilerContext prev_context = compiler->context;

    ely_chunk_init(&compiler->chunk);
    ely_compiler_context_init(&compiler->context, ELY_FT_FUNCTION);

    const size_t arity = function(compiler);

    ElyChunk func_chunk = compiler->chunk;
    compiler->chunk = prev_chunk;
    compiler->context = prev_context;

    ElyChunk* heap_chunk = (ElyChunk*)malloc(sizeof(ElyChunk));
    assert(heap_chunk != NULL && "Out of memory, cannot allocate chunk for function descriptor.");
    *heap_chunk = func_chunk;

    char* heap_name = (char*)malloc(name_token.length + 1);
    assert(heap_name != NULL && "Out of memory, cannot allocate name for function descriptor.");
    memcpy(heap_name, name_token.lexeme, name_token.length);
    heap_name[name_token.length] = '\0';

    ElyFunctionDescriptor desc = {
        .arity = arity,
        .chunk = heap_chunk,
        .name = heap_name
    };

    const size_t func_idx = ely_chunk_add_function_constant(&compiler->chunk, desc);
    const size_t const_idx = ely_chunk_add_constant(&compiler->chunk, ely_value_obj((uint64_t)func_idx));

    ely_compiler_emit_opcode(compiler, ELY_OP_CLOSURE);
    ely_compiler_emit_byte(compiler, (uint8_t)const_idx);

    define_variable(compiler, global);
}

static inline uint8_t parse_variable(ElyCompiler* compiler, const char* error_message) {
    assert(compiler != NULL && "Compiler is not NULL.");

    ely_parser_consume(&compiler->parser, ELY_TT_IDENTIFIER, error_message);
    ely_compiler_declare_variable(compiler);
    if (compiler->context.scope_depth > 0) return 0;
    return ely_compiler_identifier_constant(compiler, &compiler->parser.previous);
}

/**
 * Parses a declaration (variable, function, or falls through to a statement)
 * with error recovery via synchronization.
 */
void ely_compiler_declaration(ElyCompiler* compiler) {
    assert(compiler != NULL && "Compiler is not NULL.");
    declaration(compiler);
}

/**
 * Parses any statement (print, control flow, import, block, or expression statement).
 */
void ely_compiler_statement(ElyCompiler* compiler) {
    assert(compiler != NULL && "Compiler is not NULL.");

    if (ely_parser_match_token(&compiler->parser, ELY_TT_PRINT)) {
        print_statement(compiler);
    } else if (ely_parser_match_token(&compiler->parser, ELY_TT_IF)) {
        if_statement(compiler);
    } else if (ely_parser_match_token(&compiler->parser, ELY_TT_RETURN)) {
        return_statement(compiler);
    } else if (ely_parser_match_token(&compiler->parser, ELY_TT_WHILE)) {
        while_statement(compiler);
    } else if (ely_parser_match_token(&compiler->parser, ELY_TT_FOR)) {
        for_statement(compiler);
    } else if (ely_parser_match_token(&compiler->parser, ELY_TT_IMPORT)) {
        import_statement(compiler);
    } else if (ely_parser_match_token(&compiler->parser, ELY_TT_OPENCURLY)) {
        ely_compiler_begin_scope(compiler);
        block(compiler);
        ely_compiler_end_scope(compiler);
    } else {
        expression_statement(compiler);
    }
}

/**
 * Parses an inner block body until a closing brace or EOF is reached.
 */
void ely_compiler_block(ElyCompiler* compiler) {
    assert(compiler != NULL && "Compiler is not NULL.");
    block(compiler);
}
