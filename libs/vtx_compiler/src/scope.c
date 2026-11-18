#include "vtx/compiler/token.h"
#include <vtx/compiler/scope.h>

#include <assert.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

/**
 * Initializes an VtxCompilerContext for a given function type.
 * Automatically claims stack slot 0 for 'this' (methods/initializers) or caller frame.
 */
void vtx_compiler_context_init(VtxCompilerContext* ctx, VtxFunctionType type) {
    assert(ctx != NULL && "Compiler context pointer is NULL.");

    ctx->function_type = type;
    ctx->scope_depth = 0;
    ctx->upvalues_count = 0;
    ctx->locals_count = 0;
    memset(ctx->upvalues, 0, sizeof(struct VtxUpvalue) * VTX_UPVALUES_MAX);
    memset(ctx->locals, 0, sizeof(struct VtxLocal) * VTX_LOCALS_MAX);

    VtxToken reserved_token = {
            .token_type = VTX_TT_IDENTIFIER,
            .lexeme = "",
            .length = 0,
            .line = 0
    };

    if (type == VTX_FT_METHOD || type == VTX_FT_INITIALIZER) {
        reserved_token = (VtxToken) {
            .token_type = VTX_TT_THIS,
            .lexeme = "this",
            .length = 4,
            .line = 0
        };
    }

    ctx->locals[0] = (VtxLocal) {
        .name = reserved_token,
        .depth = 0,
        .is_captured = false
    };

    ctx->locals_count = 1;
}

/**
 * Enters a new nested lexical block scope.
 */
void vtx_compiler_context_begin_scope(VtxCompilerContext* ctx) {
    assert(ctx != NULL && "Compiler context pointer is NULL.");
    ctx->scope_depth++;
}

/**
 * Exits current block scope and removes out-of-scope locals, returning popped count.
 */
VTX_NODISCARD size_t vtx_compiler_context_pop_scope(VtxCompilerContext* ctx) {
    assert(ctx != NULL && "Compiler context pointer is NULL.");
    ctx->scope_depth--;
    size_t popped = 0;

    while (ctx->locals_count != 0 && ctx->locals[ctx->locals_count - 1].depth > (int32_t)ctx->scope_depth) {
        memset(&ctx->locals[ctx->locals_count - 1], 0, sizeof(struct VtxLocal));
        popped++;
        ctx->locals_count--;
    }

    return popped;
}

/**
 * Registers an uninitialized local variable into the current stack frame.
 */
void vtx_compiler_context_add_local(VtxCompilerContext* ctx, const VtxToken* name) {
    assert(ctx != NULL && "Compiler context pointer is NULL.");
    assert(name != NULL && "Name token pointer is NULL.");

    if (ctx->locals_count + 1 > VTX_LOCALS_MAX) {
        fprintf(stderr, "Stack allows a maximum of %d local values. Aborting..\n", VTX_LOCALS_MAX);
        return;
    }

    ctx->locals[ctx->locals_count++] = (VtxLocal) {
        .name = *name,
        .depth = -1,
        .is_captured = false
    };
}

/**
 * Searches local variables backwards to resolve stack slot index matching the given name token.
 */
VTX_NODISCARD int32_t vtx_compiler_context_resolve_local(const VtxCompilerContext* ctx, const VtxToken* name) {
    assert(ctx != NULL && "Compiler context pointer is NULL.");
    assert(name != NULL && "Name token pointer is NULL.");

    for (int32_t i = (int32_t)ctx->locals_count - 1; i >= 0; --i) {
        const VtxLocal* local = &ctx->locals[i];
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
VTX_NODISCARD int32_t vtx_compiler_context_add_upvalue(VtxCompilerContext* ctx, uint8_t index, bool is_local) {
    assert(ctx != NULL && "Compiler context pointer is NULL.");

    if (ctx->upvalues_count + 1 > VTX_UPVALUES_MAX) {
        fprintf(stderr, "Stack allows a maximum of %d upvalues. Aborting..\n", VTX_UPVALUES_MAX);
        return -1;
    }

    for (size_t i = 0; i < ctx->upvalues_count; ++i) {
        if (ctx->upvalues[i].index == index && ctx->upvalues[i].is_local == is_local) {
            return (int32_t)i;
        }
    }

    int32_t upvalue_index = (int32_t)ctx->upvalues_count++;
    ctx->upvalues[upvalue_index] = (VtxUpvalue) {
        .index = index,
        .is_local = is_local
    };

    return upvalue_index;
}
