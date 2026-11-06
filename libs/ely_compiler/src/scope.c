#include "ely/compiler/token.h"
#include <ely/compiler/scope.h>

#include <assert.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

/**
 * Initializes an ElyCompilerContext for a given function type.
 * Automatically claims stack slot 0 for 'this' (methods/initializers) or caller frame.
 */
void ely_compiler_context_init(ElyCompilerContext* ctx, ElyFunctionType type) {
    assert(ctx != NULL && "Compiler context pointer is NULL.");

    ctx->function_type = type;
    ctx->scope_depth = 0;
    ctx->upvalues_count = 0;
    ctx->locals_count = 0;
    memset(ctx->upvalues, 0, sizeof(struct ElyUpvalue) * ELY_UPVALUES_MAX);
    memset(ctx->locals, 0, sizeof(struct ElyLocal) * ELY_LOCALS_MAX);

    ElyToken reserved_token = {
            .token_type = ELY_TT_IDENTIFIER,
            .lexeme = "",
            .length = 0,
            .line = 0
    };

    if (type == ELY_FT_METHOD || type == ELY_FT_INITIALIZER) {
        reserved_token = (ElyToken) {
            .token_type = ELY_TT_THIS,
            .lexeme = "this",
            .length = 4,
            .line = 0
        };
    }

    ctx->locals[0] = (ElyLocal) {
        .name = reserved_token,
        .depth = 0,
        .is_captured = false
    };

    ctx->locals_count = 1;
}

/**
 * Enters a new nested lexical block scope.
 */
void ely_compiler_context_begin_scope(ElyCompilerContext* ctx) {
    assert(ctx != NULL && "Compiler context pointer is NULL.");
    ctx->scope_depth++;
}

/**
 * Exits current block scope and removes out-of-scope locals, returning popped count.
 */
ELY_NODISCARD size_t ely_compiler_context_pop_scope(ElyCompilerContext* ctx) {
    assert(ctx != NULL && "Compiler context pointer is NULL.");
    ctx->scope_depth--;
    size_t popped = 0;

    while (ctx->locals_count != 0 && ctx->locals[ctx->locals_count - 1].depth > (int32_t)ctx->scope_depth) {
        memset(&ctx->locals[ctx->locals_count - 1], 0, sizeof(struct ElyLocal));
        popped++;
        ctx->locals_count--;
    }

    return popped;
}

/**
 * Registers an uninitialized local variable into the current stack frame.
 */
void ely_compiler_context_add_local(ElyCompilerContext* ctx, const ElyToken* name) {
    assert(ctx != NULL && "Compiler context pointer is NULL.");
    assert(name != NULL && "Name token pointer is NULL.");

    if (ctx->locals_count + 1 > ELY_LOCALS_MAX) {
        fprintf(stderr, "Stack allows a maximum of %d local values. Aborting..\n", ELY_LOCALS_MAX);
        return;
    }

    ctx->locals[ctx->locals_count++] = (ElyLocal) {
        .name = *name,
        .depth = -1,
        .is_captured = false
    };
}

/**
 * Searches local variables backwards to resolve stack slot index matching the given name token.
 */
ELY_NODISCARD int32_t ely_compiler_context_resolve_local(const ElyCompilerContext* ctx, const ElyToken* name) {
    assert(ctx != NULL && "Compiler context pointer is NULL.");
    assert(name != NULL && "Name token pointer is NULL.");

    for (int32_t i = (int32_t)ctx->locals_count - 1; i >= 0; --i) {
        const ElyLocal* local = &ctx->locals[i];
        if (local->name.length == name->length &&
            memcmp(local->name.lexeme, name->lexeme, name->length) == 0) {
                return i;
        }
    }

    return -1;
}

/**
 * Captures an upvalue into the current compiler context with deduplication.
 */
ELY_NODISCARD int32_t ely_compiler_context_add_upvalue(ElyCompilerContext* ctx, uint8_t index, bool is_local) {
    assert(ctx != NULL && "Compiler context pointer is NULL.");

    if (ctx->upvalues_count + 1 > ELY_UPVALUES_MAX) {
        fprintf(stderr, "Stack allows a maximum of %d upvalues. Aborting..\n", ELY_UPVALUES_MAX);
        return -1;
    }

    for (size_t i = 0; i < ctx->upvalues_count; ++i) {
        if (ctx->upvalues[i].index == index && ctx->upvalues[i].is_local == is_local) {
            return (int32_t)i;
        }
    }

    int32_t upvalue_index = (int32_t)ctx->upvalues_count++;
    ctx->upvalues[upvalue_index] = (ElyUpvalue) {
        .index = index,
        .is_local = is_local
    };

    return upvalue_index;
}
