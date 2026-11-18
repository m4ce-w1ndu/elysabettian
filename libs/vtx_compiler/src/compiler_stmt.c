#include "compiler_internal.h"
#include <vtx/core/chunk.h>
#include <vtx/core/opcode.h>
#include <vtx/compiler/parser.h>
#include <vtx/compiler/token.h>
#include <vtx/compiler/compiler.h>

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static inline void var_declaration(VtxCompiler* compiler);
static inline void function_declaration(VtxCompiler* compiler);
static inline void import_statement(VtxCompiler* compiler);
static inline void synchronize(VtxCompiler* compiler);
static inline void define_variable(VtxCompiler* compiler, uint8_t global);
static inline uint8_t parse_variable(VtxCompiler* compiler, const char* error_message);

static inline void print_statement(VtxCompiler* compiler) {
    assert(compiler != NULL && "Compiler pointer is NULL.");

    vtx_compiler_expression(compiler);
    vtx_parser_consume(&compiler->parser, VTX_TT_SEMICOLON, "Expected ';' to terminate print statement.");
    vtx_compiler_emit_opcode(compiler, VTX_OP_PRINT);
}

static inline void expression_statement(VtxCompiler* compiler) {
    assert(compiler != NULL && "Compiler pointer is NULL.");

    vtx_compiler_expression(compiler);
    vtx_parser_consume(&compiler->parser, VTX_TT_SEMICOLON, "Expected ';' to terminate expression statement.");
    vtx_compiler_emit_opcode(compiler, VTX_OP_POP);
}

static inline void declaration(VtxCompiler* compiler) {
    assert(compiler != NULL && "Compiler pointer is NULL.");

    if (vtx_parser_match_token(&compiler->parser, VTX_TT_VAR)) {
        var_declaration(compiler);
    } else if (vtx_parser_match_token(&compiler->parser, VTX_TT_FUNC)) {
        function_declaration(compiler);
    } else {
        vtx_compiler_statement(compiler);
    }

    if (compiler->parser.panic_mode) {
        synchronize(compiler);
    }
}

static inline void import_statement(VtxCompiler* compiler) {
    assert(compiler != NULL && "Compiler is not NULL.");

    // Parentheses are optional to support both function-like and statement-like import system.
    const bool has_paren = vtx_parser_match_token(&compiler->parser, VTX_TT_OPENPAREN);
    vtx_parser_consume(
        &compiler->parser,
        VTX_TT_STRING,
        "Expected module name string literal following 'import' keyword."
    );

    // Strip quotes from the module name literal
    const VtxToken* name_token = &compiler->parser.previous;
    const char* module_str = name_token->length >= 2 ? name_token->lexeme + 1 : "";
    const size_t module_len = name_token->length >= 2 ? name_token->length - 2 : 0;

    const size_t string_idx = vtx_chunk_add_string_constant(
        &compiler->chunk,
        module_str,
        module_len
    );

    if (has_paren) {
        vtx_parser_consume(
            &compiler->parser,
            VTX_TT_CLOSEPAREN,
            "Expected closing ')' after module name in 'import' statement."
        );
    }

    vtx_parser_consume(
        &compiler->parser,
        VTX_TT_SEMICOLON,
        "Expected ';' to terminate import statement."
    );

    vtx_compiler_emit_opcode(compiler, VTX_OP_IMPORT);
    vtx_compiler_emit_byte(compiler, (uint8_t)string_idx);
}

static inline void define_variable(VtxCompiler* compiler, uint8_t global) {
    assert(compiler != NULL && "Compiler is not NULL.");

    if (compiler->context.scope_depth > 0) {
        vtx_compiler_mark_initialized(compiler);
        const uint8_t slot = (uint8_t)(compiler->context.locals_count - 1);
        vtx_compiler_emit_opcode(compiler, VTX_OP_SETLOCAL);
        vtx_compiler_emit_byte(compiler, slot);
        return;
    }

    vtx_compiler_emit_opcode(compiler, VTX_OP_DEFINEGLOBAL);
    vtx_compiler_emit_byte(compiler, global);
}

static inline void var_declaration(VtxCompiler* compiler) {
    assert(compiler != NULL && "Compiler is not NULL.");

    const uint8_t global = parse_variable(compiler, "Expected variable identifier in declaration.");

    if (vtx_parser_match_token(&compiler->parser, VTX_TT_ASSIGN)) {
        vtx_compiler_expression(compiler);
    } else {
        vtx_compiler_emit_opcode(compiler, VTX_OP_NOP);
    }

    vtx_parser_consume(
        &compiler->parser,
        VTX_TT_SEMICOLON,
        "Expected ';' to terminate variable declaration."
    );

    define_variable(compiler, global);
}

static inline void synchronize(VtxCompiler* compiler) {
    assert(compiler != NULL && "Compiler is not NULL.");

    vtx_parser_reset_panic_mode(&compiler->parser);

    while (compiler->parser.current.token_type != VTX_TT_EOF) {
        if (compiler->parser.previous.token_type == VTX_TT_SEMICOLON) return;

        switch (compiler->parser.current.token_type) {
            case VTX_TT_CLASS:
            case VTX_TT_FUNC:
            case VTX_TT_VAR:
            case VTX_TT_FOR:
            case VTX_TT_IF:
            case VTX_TT_WHILE:
            case VTX_TT_PRINT:
            case VTX_TT_RETURN: return;
            default: break;
        }

        vtx_parser_advance(&compiler->parser);
    }
}

static inline void block(VtxCompiler* compiler) {
    assert(compiler != NULL && "Compiler is not NULL.");

    while (compiler->parser.current.token_type != VTX_TT_CLOSECURLY &&
            compiler->parser.current.token_type != VTX_TT_EOF) {
        declaration(compiler);
    }

    vtx_parser_consume(
        &compiler->parser,
        VTX_TT_CLOSECURLY,
        "Expected '}' to close block."
    );
}

static inline void if_statement(VtxCompiler* compiler) {
    assert(compiler != NULL && "Compiler is not NULL.");

    vtx_parser_consume(&compiler->parser, VTX_TT_OPENPAREN,
        "Expected '(' to open 'if' condition.");
    vtx_compiler_expression(compiler);
    vtx_parser_consume(&compiler->parser, VTX_TT_CLOSEPAREN,
        "Expected ')' to close 'if' condition.");

    const size_t then_jump = vtx_compiler_emit_jump(compiler, VTX_OP_JUMPZERO);
    vtx_compiler_emit_opcode(compiler, VTX_OP_POP);
    vtx_compiler_statement(compiler);

    const size_t else_jump = vtx_compiler_emit_jump(compiler, VTX_OP_JUMP);
    vtx_compiler_patch_jump(compiler, then_jump);
    vtx_compiler_emit_opcode(compiler, VTX_OP_POP);

    if (vtx_parser_match_token(&compiler->parser, VTX_TT_ELSE)) {
        vtx_compiler_statement(compiler);
    }

    vtx_compiler_patch_jump(compiler, else_jump);
}

static inline void while_statement(VtxCompiler* compiler) {
    assert(compiler != NULL && "Compiler is not NULL.");

    const size_t loop_start = compiler->chunk.code_count;
    vtx_parser_consume(&compiler->parser, VTX_TT_OPENPAREN,
        "Expected '(' to open 'while' condition.");
    vtx_compiler_expression(compiler);
    vtx_parser_consume(&compiler->parser, VTX_TT_CLOSEPAREN,
        "Expected ')' to close 'while' condition.");

    const size_t exit_jump = vtx_compiler_emit_jump(compiler, VTX_OP_JUMPZERO);
    vtx_compiler_emit_opcode(compiler, VTX_OP_POP);

    vtx_compiler_statement(compiler);
    vtx_compiler_emit_loop(compiler, loop_start);

    vtx_compiler_patch_jump(compiler, exit_jump);
    vtx_compiler_emit_opcode(compiler, VTX_OP_POP);
}

static inline void for_statement(VtxCompiler* compiler) {
    assert(compiler != NULL && "Compiler is not NULL.");

    vtx_compiler_begin_scope(compiler);
    vtx_parser_consume(&compiler->parser, VTX_TT_OPENPAREN,
        "Expecter '(' to open 'for' condition.");

    if (vtx_parser_match_token(&compiler->parser, VTX_TT_SEMICOLON)) {
        ;
    } else if (vtx_parser_match_token(&compiler->parser, VTX_TT_VAR)) {
        var_declaration(compiler);
    } else {
        expression_statement(compiler);
    }

    size_t loop_start = compiler->chunk.code_count;
    size_t exit_jump = 0;
    if (!vtx_parser_match_token(&compiler->parser, VTX_TT_SEMICOLON)) {
        vtx_compiler_expression(compiler);
        vtx_parser_consume(&compiler->parser, VTX_TT_SEMICOLON,
            "Expected ';' to terminate 'for' conditional.");
        exit_jump = vtx_compiler_emit_jump(compiler, VTX_OP_JUMPZERO);
        vtx_compiler_emit_opcode(compiler, VTX_OP_POP);
    }

    if (!vtx_parser_match_token(&compiler->parser, VTX_TT_CLOSEPAREN)) {
        const size_t body_jump = vtx_compiler_emit_jump(compiler, VTX_OP_JUMP);
        const size_t increment_start = compiler->chunk.code_count;
        vtx_compiler_expression(compiler);
        vtx_compiler_emit_opcode(compiler, VTX_OP_POP);

        vtx_parser_consume(&compiler->parser, VTX_TT_CLOSEPAREN,
            "Expected ')' to close 'for' conditional clauses.");
        vtx_compiler_emit_loop(compiler, loop_start);
        loop_start = increment_start;
        vtx_compiler_patch_jump(compiler, body_jump);
    }

    vtx_compiler_statement(compiler);
    vtx_compiler_emit_loop(compiler, loop_start);

    if (exit_jump) {
        vtx_compiler_patch_jump(compiler, exit_jump);
        vtx_compiler_emit_opcode(compiler, VTX_OP_POP);
    }

    vtx_compiler_end_scope(compiler);
}

VTX_NODISCARD static inline size_t function(VtxCompiler* compiler) {
    assert(compiler != NULL && "Compiler is not NULL.");

    vtx_compiler_begin_scope(compiler);

    size_t arity = 0;
    if (!vtx_parser_check(&compiler->parser, VTX_TT_CLOSEPAREN)) {
        while (true) {
            arity++;
            if (arity > VTX_ARGS_MAX) {
                vtx_parser_error(&compiler->parser,
                    "Parameter count limit exceeded: function declaration cannot exceed 255 parameters.");
                return arity;
            }

            (void)parse_variable(compiler, "Expected parameter identifier in function signature.");
            vtx_compiler_mark_initialized(compiler);

            if (!vtx_parser_match_token(&compiler->parser, VTX_TT_COMMA)) break;
        }
    }

    vtx_parser_consume(&compiler->parser, VTX_TT_CLOSEPAREN, "Expected ')' to close parameter list.");
    vtx_parser_consume(&compiler->parser, VTX_TT_OPENCURLY, "Expected '{' to open function body.");

    block(compiler);
    vtx_compiler_end_scope(compiler);
    vtx_compiler_emit_return(compiler);

    return arity;

}

static inline void return_statement(VtxCompiler* compiler) {
    assert(compiler != NULL && "Compiler is not NULL.");

    if (compiler->context.function_type == VTX_FT_SCRIPT) {
        vtx_parser_error(&compiler->parser, "Cannot return from top-level script code.");
    }

    if (vtx_parser_match_token(&compiler->parser, VTX_TT_SEMICOLON)) {
        vtx_compiler_emit_return(compiler);
    } else {
        vtx_compiler_expression(compiler);
        vtx_parser_consume(&compiler->parser, VTX_TT_SEMICOLON, "Expected ';' to terminate return statement.");
        vtx_compiler_emit_opcode(compiler, VTX_OP_RETURN);
    }
}

static inline void function_declaration(VtxCompiler* compiler) {
    assert(compiler != NULL && "Compiler is not NULL.");

    const uint8_t global = parse_variable(compiler, "Expected function identifier in function declaration.");
    const VtxToken name_token = compiler->parser.previous;

    if (compiler->context.scope_depth > 0) {
        vtx_compiler_mark_initialized(compiler);
    }

    VtxChunk prev_chunk = compiler->chunk;
    VtxCompilerContext prev_context = compiler->context;

    vtx_chunk_init(&compiler->chunk);
    vtx_compiler_context_init(&compiler->context, VTX_FT_FUNCTION);

    const size_t arity = function(compiler);

    VtxChunk func_chunk = compiler->chunk;
    compiler->chunk = prev_chunk;
    compiler->context = prev_context;

    VtxChunk* heap_chunk = (VtxChunk*)malloc(sizeof(VtxChunk));
    assert(heap_chunk != NULL && "Out of memory, cannot allocate chunk for function descriptor.");
    *heap_chunk = func_chunk;

    char* heap_name = (char*)malloc(name_token.length + 1);
    assert(heap_name != NULL && "Out of memory, cannot allocate name for function descriptor.");
    memcpy(heap_name, name_token.lexeme, name_token.length);
    heap_name[name_token.length] = '\0';

    VtxFunctionDescriptor desc = {
        .arity = arity,
        .chunk = heap_chunk,
        .name = heap_name
    };

    const size_t func_idx = vtx_chunk_add_function_constant(&compiler->chunk, desc);
    const size_t const_idx = vtx_chunk_add_constant(&compiler->chunk, vtx_value_obj((uint64_t)func_idx));

    vtx_compiler_emit_opcode(compiler, VTX_OP_CLOSURE);
    vtx_compiler_emit_byte(compiler, (uint8_t)const_idx);

    define_variable(compiler, global);
}

static inline uint8_t parse_variable(VtxCompiler* compiler, const char* error_message) {
    assert(compiler != NULL && "Compiler is not NULL.");

    vtx_parser_consume(&compiler->parser, VTX_TT_IDENTIFIER, error_message);
    vtx_compiler_declare_variable(compiler);
    if (compiler->context.scope_depth > 0) return 0;
    return vtx_compiler_identifier_constant(compiler, &compiler->parser.previous);
}

/**
 * Parses a declaration (variable, function, or falls through to a statement)
 * with error recovery via synchronization.
 */
void vtx_compiler_declaration(VtxCompiler* compiler) {
    assert(compiler != NULL && "Compiler is not NULL.");
    declaration(compiler);
}

/**
 * Parses any statement (print, control flow, import, block, or expression statement).
 */
void vtx_compiler_statement(VtxCompiler* compiler) {
    assert(compiler != NULL && "Compiler is not NULL.");

    if (vtx_parser_match_token(&compiler->parser, VTX_TT_PRINT)) {
        print_statement(compiler);
    } else if (vtx_parser_match_token(&compiler->parser, VTX_TT_IF)) {
        if_statement(compiler);
    } else if (vtx_parser_match_token(&compiler->parser, VTX_TT_RETURN)) {
        return_statement(compiler);
    } else if (vtx_parser_match_token(&compiler->parser, VTX_TT_WHILE)) {
        while_statement(compiler);
    } else if (vtx_parser_match_token(&compiler->parser, VTX_TT_FOR)) {
        for_statement(compiler);
    } else if (vtx_parser_match_token(&compiler->parser, VTX_TT_IMPORT)) {
        import_statement(compiler);
    } else if (vtx_parser_match_token(&compiler->parser, VTX_TT_OPENCURLY)) {
        vtx_compiler_begin_scope(compiler);
        block(compiler);
        vtx_compiler_end_scope(compiler);
    } else {
        expression_statement(compiler);
    }
}

/**
 * Parses an inner block body until a closing brace or EOF is reached.
 */
void vtx_compiler_block(VtxCompiler* compiler) {
    assert(compiler != NULL && "Compiler is not NULL.");
    block(compiler);
}
