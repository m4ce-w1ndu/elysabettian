#ifndef ELY_COMPILER_SCOPE_H
#define ELY_COMPILER_SCOPE_H

#include <ely/core/value.h>
#include "token.h"

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

/// Max number of local variables allowed in any scope.
#define ELY_LOCALS_MAX 256

/// Max number of upvalues allowed in any context.
#define ELY_UPVALUES_MAX 256

/**
 * Local variable representation tracked on the virtual machine stack frame.
 */
typedef struct ElyLocal {
    /// Identifier token of this local variable.
    ElyToken name;
    /// Scope depth where this variable was declared (-1 if uninitialised).
    int32_t depth;
    /// Whether this local variable has been closed over as an upvalue.
    bool is_captured;
} ElyLocal;

/**
 * Upvalue descriptor tracking closed-over variables across scopes.
 */
typedef struct ElyUpvalue {
    /// Stack slot index (if is_local) or enclosing upvalue index.
    uint8_t index;
    /// True if capturing a local in the immediately enclosing scope, false if transitive.
    bool is_local;
} ElyUpvalue;

/**
 * Classifies the type of function or script currently being compiled.
 */
typedef enum ElyFunctionType {
    /// Top-level script block.
    ELY_FT_SCRIPT,
    /// Standard named or anonymous function.
    ELY_FT_FUNCTION,
    /// Class member method (binds slot 0 to 'this').
    ELY_FT_METHOD,
    /// Class constructor/initialised (returns 'this').
    ELY_FT_INITIALIZER
} ElyFunctionType;

/**
 * Holds lexical scope depth, local variable allocation, and upvalue tracking.
 */
typedef struct ElyCompilerContext {
    /// Tracks all local variables in the current call frame.
    ElyLocal locals[ELY_LOCALS_MAX];
    /// Number of local variables.
    size_t locals_count;
    /// Tracks upvalues captured from enclosing scopes.
    ElyUpvalue upvalues[ELY_UPVALUES_MAX];
    /// Number of upvalues.
    size_t upvalues_count;
    /// Current block scope nesting level.
    uint32_t scope_depth;
    /// Current function kind.
    ElyFunctionType function_type;
} ElyCompilerContext;

/**
 * Initialises a new CompilerContext for the given function type.
 *
 * Automatically claims slot 0 for 'this' (methods/initializers) or function name.
 *
 * @param ctx Compiler context.
 * @param type Function category being compiled.
 */
void ely_compiler_context_init(ElyCompilerContext* ctx, ElyFunctionType type);

/**
 * Increments lexical block nesting depth.
 *
 * @param ctx Compiler context.
 */
void ely_compiler_context_begin_scope(ElyCompilerContext* ctx);

/**
 * Decrements scope depth and pops all out-of-scope locals from tracking.
 *
 * @param ctx Compiler context.
 * @return Number of local variables removed from the frame.
 */
ELY_NODISCARD size_t ely_compiler_context_pop_scope(ElyCompilerContext* ctx);

/**
 * Registers a new uninitialized local variable into the current scope frame.
 *
 * @param ctx Compiler context.
 * @param name Identifier token of the local variable.
 */
void ely_compiler_context_add_local(ElyCompilerContext* ctx, const ElyToken* name);

/**
 * Resolves the stack slot index of a local variable matching the given identifier.
 *
 * Searches backwards from newest to oldest local in scope.
 *
 * @param ctx Compiler context.
 * @param name Identifier token to search for.
 * @return 0-based stack slot index if found and initialized, -1 if not found.
 */
ELY_NODISCARD int32_t ely_compiler_context_resolve_local(const ElyCompilerContext* ctx, const ElyToken* name);

/**
 * Adds an upvalue to the current function context, or returns existing index if already captured.
 *
 * @param ctx Compiler context.
 * @param index Stack slot index (if local to enclosing frame) or enclosing upvalue index.
 * @param is_local True if capturing from immediate enclosing frame, false if transitive.
 * @return 0-based upvalue index in the context's upvalue array, or -1 if capacity exceeded.
 */
ELY_NODISCARD int32_t ely_compiler_context_add_upvalue(ElyCompilerContext* ctx, uint8_t index, bool is_local);

#endif
