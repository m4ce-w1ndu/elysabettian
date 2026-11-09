#ifndef ELY_VM_HEAP_OBJECT_H
#define ELY_VM_HEAP_OBJECT_H

#include <ely/core/value.h>
#include <ely/core/chunk.h>

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

/**
 * Represents a method bound to a receiver object instance.
 */
typedef struct ElyObjBoundMethod {
    /// The target instance acting as the `this`/receiver value.
    ElyValue receiver;
    /// Heap identifier of the compiled method or closure.
    size_t method_id;
} ElyObjBoundMethod;

/**
 * Represents a class blueprint with a table of method references.
 */
typedef struct ElyObjClass {
    /// Display name of the class.
    const char* name;

    /// Table mapping method name hash to function/closure heap ID.
    struct {
        uint32_t hash;
        size_t function_id;
    }* methods;

    /// Number of methods currently stored in the class.
    size_t method_count;
    /// Allocated capacity of the methods table.
    size_t capacity;
} ElyObjClass;


/**
 * Represents an instantiated class object with dynamic instance fields.
 */
typedef struct ElyObjInstance {
    /// Heap identifier of the class definition this instance belongs to.
    size_t class_id;

    /// Dynamic field storage mapping property name hashes to Values.
    struct {
        uint32_t hash;
        ElyValue value;
    }* fields;

    /// Number of fields currently stored in the instance.
    size_t field_count;
    /// Allocated capacity of the fields table.
    size_t field_capacity;
} ElyObjInstance;

/**
 * Represents a compiled function object tracked on the heap.
 *
 * Stores executable bytecode chunk, arity, upvalue counts, and profiling
 * metrics for JIT tier-up compilation.
 */
typedef struct ElyObjFunction {
    /// Expected number of parameters.
    size_t arity;
    /// Number of upvalues captured by closures wrapping this function.
    size_t upvalue_count;
    /// Shared reference to the bytecode Chunk executing this function.
    ElyChunk* chunk;
    /// Optional debug name of the function.
    const char* name;
    /// Execution counter used by the tier-up policy to trigger JIT compilation.
    size_t invocation_count;
    /// Entry pointer to native code compiled by the AsmJit backend (nullptr if interpreted).
    const uint8_t* jit_entry;
} ElyObjFunction;

/**
 * Tracks a variable captured by a closure across execution scopes.
 *
 * An upvalue begins in an "open" state pointing to a stack slot. When the
 * enclosing stack frame exits, the variable is "closed" by moving the Value
 * into the `closed` member.
 */
typedef struct ElyObjUpvalue {
    /// Stack slot offset where the local variable resides while open.
    size_t location;
    /// Stored value once the upvalue is closed and no longer on the stack.
    ElyValue closed;
    /// Checks if this upvalue is still pointing to the active VM stack.
    bool is_closed;
} ElyObjUpvalue;

/**
 * Represents a first-class closure pairing a function with its captured upvalues.
 */
typedef struct ElyObjClosure {
    /// Heap identifier of the underlying ObjFunction.
    size_t function_id;
    /// Heap identifiers of the ObjUpvalues captured by this closure instance.
    size_t* upvalue_ids;
    /// Number of upvalues currently stored in the closure.
    size_t upvalue_count;
    /// Allocated capacity of the upvalue_ids array.
    size_t upvalue_capacity;
} ElyObjClosure;

/**
 * Represents a dynamically allocated, null-terminated string object on the heap.
 */
typedef struct ElyObjString {
    /// Number of bytes in the string (excluding null-terminator).
    size_t length;
    /// Pointer to dynamically allocated character buffer.
    char* chars;
    /// Cached FNV-1a hash of the string for fast hash-table lookup.
    uint32_t hash;
} ElyObjString;

/**
 * Represents a dynamically resizable array of Elysabettian Values on the heap.
 */
typedef struct ElyObjArray {
    /// Current number of elements stored in the array.
    size_t length;
    /// Maximum capacity of the dynamically allocated buffer.
    size_t capacity;
    /// Contiguous buffer of stored Values.
    ElyValue* values;
} ElyObjArray;

/**
 * Classifies the concrete type of an allocated heap object.
 */
typedef enum ElyHeapObjectType {
    ELY_HEAP_OBJECT_STRING,
    ELY_HEAP_OBJECT_ARRAY,
    ELY_HEAP_OBJECT_FUNCTION,
    ELY_HEAP_OBJECT_CLOSURE,
    ELY_HEAP_OBJECT_UPVALUE,
    ELY_HEAP_OBJECT_CLASS,
    ELY_HEAP_OBJECT_INSTANCE,
    ELY_HEAP_OBJECT_BOUND_METHOD
} ElyHeapObjectType;

/**
 * Tagged union representing any dynamically allocated object managed by the VM heap.
 *
 * Each object is addressable via a 48-bit index encoded inside a NaN-boxed Value.
 */
typedef struct ElyHeapObject {
    /// Concrete discriminator indicating which union variant is active.
    ElyHeapObjectType type;
    /// Anonymous union holding the variant object data.
    union {
        ElyObjString string;
        ElyObjArray array;
        ElyObjFunction function;
        ElyObjClosure closure;
        ElyObjUpvalue upvalue;
        ElyObjClass class_obj;
        ElyObjInstance instance;
        ElyObjBoundMethod bound_method;
    };
} ElyHeapObject;

#endif // ELY_VM_HEAP_OBJECT_H

