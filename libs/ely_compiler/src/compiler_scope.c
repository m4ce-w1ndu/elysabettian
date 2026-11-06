#include "compiler_internal.h"
#include <ely/compiler/parser.h>
#include <ely/core/chunk.h>
#include <ely/compiler/scope.h>
#include <ely/core/opcode.h>

/**
 * Enters a new nested lexical block scope.
 */
void ely_compiler_begin_scope(ElyCompiler* compiler) {
    assert(compiler != NULL && "Compiler pointer is NULL.");
    ely_compiler_context_begin_scope(&compiler->context);
}

/**
 * Exits current block scope and emits POP opcodes for all discarded local variables.
 */
void ely_compiler_end_scope(ElyCompiler* compiler) {
    assert(compiler != NULL && "Compiler pointer is NULL.");

    const size_t to_pop = ely_compiler_context_pop_scope(&compiler->context);
    for (size_t i = 0; i < to_pop; ++i) {
        ely_compiler_emit_opcode(compiler, ELY_OP_POP);
    }
}

/**
 * Declares a local variable in the current scope, checking for re-declaration conflicts.
 */
void ely_compiler_declare_variable(ElyCompiler* compiler) {
    assert(compiler != NULL && "Compiler pointer is NULL.");

    if (compiler->context.scope_depth == 0) return;
    const ElyToken name = compiler->parser.previous;

    for (size_t i = compiler->context.locals_count; i-- > 0;) {
        if (compiler->context.locals[i].depth != -1 &&
            compiler->context.locals[i].depth < (int32_t)(compiler->context.scope_depth)) break;

        if (name.length == compiler->context.locals[i].name.length &&
            memcmp(name.lexeme, compiler->context.locals[i].name.lexeme, name.length) == 0) {
            ely_parser_error(&compiler->parser, "Identifier already declared in this scope.");
            return;
        }
    }

    ely_compiler_context_add_local(&compiler->context, &name);
}

/**
 * Marks the innermost uninitialized local variable as ready and initialized.
 */
void ely_compiler_mark_initialized(ElyCompiler* compiler) {
    assert(compiler != NULL && "Compiler pointer is NULL.");

    if (compiler->context.scope_depth == 0) return;
    ElyLocal* last = &compiler->context.locals[compiler->context.locals_count - 1];
    last->depth = (int32_t)compiler->context.scope_depth;
}

/**
 * Resolves whether an identifier token refers to an initialized local variable.
 */
bool ely_compiler_resolve_local(ElyCompiler* compiler, const ElyToken* name, uint8_t* out_slot) {
    assert(compiler != NULL && "Compiler pointer is NULL.");
    assert(out_slot != NULL && "Output parameter is NULL.");

    size_t slot;
    for (slot = compiler->context.locals_count; slot-- > 0;) {
        ElyLocal* var = &compiler->context.locals[slot];
        if (name->length == var->name.length &&
            memcmp(name->lexeme, var->name.lexeme, name->length) == 0) {
            if (var->depth == -1) {
                ely_parser_error(
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
uint8_t ely_compiler_identifier_constant(ElyCompiler* compiler, const ElyToken* name) {
    assert(compiler != NULL && "Compiler pointer is NULL.");
    const size_t string_idx = ely_chunk_add_string_constant(
        &compiler->chunk,
        name->lexeme,
        name->length
    );

    return (uint8_t)string_idx;
}
