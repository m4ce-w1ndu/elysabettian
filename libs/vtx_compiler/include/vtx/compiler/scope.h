#ifndef VTX_COMPILER_SCOPE_H
#define VTX_COMPILER_SCOPE_H

#include <vtx/core/value.h>
#include "token.h"

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

/// Max number of local variables allowed in any scope.
#define VTX_LOCALS_MAX 256

/// Max number of upvalues allowed in any context.
#define VTX_UPVALUES_MAX 256

/**
 * Local variable representation tracked on the virtual machine stack frame.
 */
typedef struct VtxLocal {
    /// Identifier token of this local variable.
    VtxToken name;
    /// Scope depth where this variable was declared (-1 if uninitialised).
    int32_t depth;
    /// Whether this local variable has been closed over as an upvalue.
    bool is_captured;
} VtxLocal;

/**
 * Upvalue descriptor tracking closed-over variables across scopes.
 */
typedef struct VtxUpvalue {
    /// Stack slot index (if is_local) or enclosing upvalue index.
    uint8_t index;
    /// True if capturing a local in the immediately enclosing scope, false if transitive.
    bool is_local;
} VtxUpvalue;

/**
 * Classifies the type of function or script currently being compiled.
 */
typedef enum VtxFunctionType {
    /// Top-level script block.
    VTX_FT_SCRIPT,
    /// Standard named or anonymous function.
    VTX_FT_FUNCTION,
    /// Class member method (binds slot 0 to 'this').
    VTX_FT_METHOD,
    /// Class constructor/initialised (returns 'this').
    VTX_FT_INITIALIZER
} VtxFunctionType;

/**
 * Holds lexical scope depth, local variable allocation, and upvalue tracking.
 */
typedef struct VtxCompilerContext {
    /// Tracks all local variables in the current call frame.
    VtxLocal locals[VTX_LOCALS_MAX];
    /// Number of local variables.
    size_t locals_count;
    /// Tracks upvalues captured from enclosing scopes.
    VtxUpvalue upvalues[VTX_UPVALUES_MAX];
    /// Number of upvalues.
    size_t upvalues_count;
    /// Current block scope nesting level.
    uint32_t scope_depth;
    /// Current function kind.
    VtxFunctionType function_type;
} VtxCompilerContext;

/**
 * Initialises a new CompilerContext for the given function type.
 *
 * Automatically claims slot 0 for 'this' (methods/initializers) or function name.
 *
 * @param ctx Compiler context.
 * @param type Function category being compiled.
 */
void vtx_compiler_context_init(VtxCompilerContext* ctx, VtxFunctionType type);

/**
 * Increments lexical block nesting depth.
 *
 * @param ctx Compiler context.
 */
void vtx_compiler_context_begin_scope(VtxCompilerContext* ctx);

/**
 * Decrements scope depth and pops all out-of-scope locals from tracking.
 *
 * @param ctx Compiler context.
 * @return Number of local variables removed from the frame.
 */
VTX_NODISCARD size_t vtx_compiler_context_pop_scope(VtxCompilerContext* ctx);

/**
 * Registers a new uninitialized local variable into the current scope frame.
 *
 * @param ctx Compiler context.
 * @param name Identifier token of the local variable.
 */
void vtx_compiler_context_add_local(VtxCompilerContext* ctx, const VtxToken* name);

/**
 * Resolves the stack slot index of a local variable matching the given identifier.
 *
 * Searches backwards from newest to oldest local in scope.
 *
 * @param ctx Compiler context.
 * @param name Identifier token to search for.
 * @return 0-based stack slot index if found and initialized, -1 if not found.
 */
VTX_NODISCARD int32_t vtx_compiler_context_resolve_local(const VtxCompilerContext* ctx, const VtxToken* name);

/**
 * Adds an upvalue to the current function context, or returns existing index if already captured.
 *
 * @param ctx Compiler context.
 * @param index Stack slot index (if local to enclosing frame) or enclosing upvalue index.
 * @param is_local True if capturing from immediate enclosing frame, false if transitive.
 * @return 0-based upvalue index in the context's upvalue array, or -1 if capacity exceeded.
 */
VTX_NODISCARD int32_t vtx_compiler_context_add_upvalue(VtxCompilerContext* ctx, uint8_t index, bool is_local);

#endif
