#include "compiler_internal.h"
#include <vtx/compiler/parser.h>
#include <vtx/core/chunk.h>
#include <vtx/compiler/scope.h>
#include <vtx/core/opcode.h>

/**
 * Enters a new nested lexical block scope.
 */
void vtx_compiler_begin_scope(VtxCompiler* compiler) {
    assert(compiler != NULL && "Compiler pointer is NULL.");
    vtx_compiler_context_begin_scope(&compiler->context);
}

/**
 * Exits current block scope and emits POP opcodes for all discarded local variables.
 */
void vtx_compiler_end_scope(VtxCompiler* compiler) {
    assert(compiler != NULL && "Compiler pointer is NULL.");

    const size_t to_pop = vtx_compiler_context_pop_scope(&compiler->context);
    for (size_t i = 0; i < to_pop; ++i) {
        vtx_compiler_emit_opcode(compiler, VTX_OP_POP);
    }
}

/**
 * Declares a local variable in the current scope, checking for re-declaration conflicts.
 */
void vtx_compiler_declare_variable(VtxCompiler* compiler) {
    assert(compiler != NULL && "Compiler pointer is NULL.");

    if (compiler->context.scope_depth == 0) return;
    const VtxToken name = compiler->parser.previous;

    for (size_t i = compiler->context.locals_count; i-- > 0;) {
        if (compiler->context.locals[i].depth != -1 &&
            compiler->context.locals[i].depth < (int32_t)(compiler->context.scope_depth)) break;

        if (name.length == compiler->context.locals[i].name.length &&
            memcmp(name.lexeme, compiler->context.locals[i].name.lexeme, name.length) == 0) {
            vtx_parser_error(&compiler->parser, "Identifier already declared in this scope.");
            return;
        }
    }

    vtx_compiler_context_add_local(&compiler->context, &name);
}

/**
 * Marks the innermost uninitialized local variable as ready and initialized.
 */
void vtx_compiler_mark_initialized(VtxCompiler* compiler) {
    assert(compiler != NULL && "Compiler pointer is NULL.");

    if (compiler->context.scope_depth == 0) return;
    VtxLocal* last = &compiler->context.locals[compiler->context.locals_count - 1];
    last->depth = (int32_t)compiler->context.scope_depth;
}

/**
 * Resolves whether an identifier token refers to an initialized local variable.
 */
bool vtx_compiler_resolve_local(VtxCompiler* compiler, const VtxToken* name, uint8_t* out_slot) {
    assert(compiler != NULL && "Compiler pointer is NULL.");
    assert(out_slot != NULL && "Output parameter is NULL.");

    size_t slot;
    for (slot = compiler->context.locals_count; slot-- > 0;) {
        VtxLocal* var = &compiler->context.locals[slot];
        if (name->length == var->name.length &&
            memcmp(name->lexeme, var->name.lexeme, name->length) == 0) {
            if (var->depth == -1) {
                vtx_parser_error(
                    &compiler->parser,
                    "Cannot access local variable before initialization: self-reference in variable initializer."
                );
                return false;
            }
            *out_slot = (uint8_t)slot;
            return true;
        }
    }

    return false;
}

/**
 * Stores an identifier string literal into the chunk string constant pool and returns its index.
 */
uint8_t vtx_compiler_identifier_constant(VtxCompiler* compiler, const VtxToken* name) {
    assert(compiler != NULL && "Compiler pointer is NULL.");
    const size_t string_idx = vtx_chunk_add_string_constant(
        &compiler->chunk,
        name->lexeme,
        name->length
    );

    return (uint8_t)string_idx;
}
