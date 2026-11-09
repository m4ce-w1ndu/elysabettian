#ifndef ELY_VM_VM_H
#define ELY_VM_VM_H

#include <ely/core/chunk.h>
#include <ely/core/value.h>
#include <ely/stdlib/native.h>
#include <ely/vm/call_frame.h>
#include <ely/vm/heap.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/**
 * Expresses the result of an execution: the execution could stop at
 * compile time (syntax errors, etc..) or at runtime (logical error
 * that generates an exception, etc...).
 */
typedef enum ElyInterpretResult {
    ELY_INTERPRET_OK,
    ELY_INTERPRET_COMPILE_ERROR,
    ELY_INTERPRET_RUNTIME_ERROR
} ElyInterpretResult;

/// Maximum call stack depth before triggering a stack overflow runtime error.
#define ELY_VM_FRAMES_MAX 64

/// Initial capacity pre-allocated for the operand evaluation stack.
#define ELY_VM_STACK_INITIAL_CAPACITY 256

/// Invocations threshold before an interpreted function is tiered up to JIT.
#define ELY_VM_JIT_HOT_THRESHOLD 50

/**
 * Virtual machine responsible for executing compiled Elysabettian bytecode chunks.
 *
 * Implements a stack-based runtime with call frame management, managed object heap,
 * lexical closure and upvalue resolution, and dynamic native standard library integration.
 */
typedef struct ElyVM {
    /// Evaluation stack for operand values and local variables.
    ElyValue* stack;
    /// Current number of values pushed onto the evaluation stack.
    size_t stack_count;
    /// Total capacity pre-allocated for the operand evaluation stack.
    size_t stack_capacity;

    /// Active call frame stack tracking execution state.
    ElyCallFrame frames[ELY_VM_FRAMES_MAX];

    /// Managed object heap storing dynamic entities (strings, arrays, functions, classes).
    ElyHeap heap;

    /// Identifiers of currently open upvalues pointing to stack slots.
    size_t* open_upvalues;
    /// Number of active open upvalues.
    size_t open_upvalues_count;
    /// Allocated capacity of the open upvalues array.
    size_t open_upvalues_capacity;

    /// Global variable table mapping name identifiers to Values.
    ElyTable globals;

    /// Native functions table mapping name identifiers to native callables.
    ElyTable natives;

    /// Current number of active call frames in the frames stack.
    uint8_t frame_count;

    /// Whether dynamic JIT tier-up optimization is active.
    bool jit_enabled;
    /// Whether to dump intermediate representation during JIT compilation.
    bool dump_ir;
    /// Whether to dump machine code disassembly during JIT compilation.
    bool dump_asm;
} ElyVM;

/**
 * Initialises a virtual machine instance.
 *
 * Pre-allocates initial storage for the evaluation stack, initialises the object heap,
 * global/native tables, and resets call frames and active upvalues.
 *
 * @param vm Pointer to the ElyVM instance to initialise.
 */
void ely_vm_init(ElyVM* vm);

/**
 * Releases all resources held by the virtual machine.
 *
 * Deallocates the evaluation stack, the managed heap, global and native tables,
 * and clears open upvalues.
 *
 * @param vm Pointer to the ElyVM instance to deallocate.
 */
void ely_vm_free(ElyVM* vm);

/**
 * Compiles and executes an Elysabettian source code string.
 *
 * @param vm Pointer to the virtual machine executing the source.
 * @param source Null-terminated UTF-8 source script text.
 * @return Execution outcome (Ok, CompileError, or RuntimeError).
 */
ELY_NODISCARD ElyInterpretResult ely_vm_interpret_from_source(ElyVM* vm, const char* source);

/**
 * Interprets a compiled bytecode chunk by setting up the top-level script frame.
 *
 * @param vm Pointer to the virtual machine executing the chunk.
 * @param chunk Compiled bytecode chunk to execute.
 * @return Execution outcome (Ok, CompileError, or RuntimeError).
 */
ELY_NODISCARD ElyInterpretResult ely_vm_interpret(ElyVM* vm, const ElyChunk* chunk);

/**
 * Pushes a NaN-boxed Value onto the evaluation stack.
 *
 * Dynamically resizes the stack if current capacity is exceeded.
 *
 * @param vm Pointer to the active virtual machine.
 * @param value IEEE-754 NaN-boxed Value to push.
 */
void ely_vm_push(ElyVM* vm, ElyValue value);

/**
 * Pops and returns the top value from the evaluation stack.
 *
 * Asserts that the stack is non-empty before decrementing.
 *
 * @param vm Pointer to the active virtual machine.
 * @return The popped NaN-boxed Value.
 */
ELY_NODISCARD ElyValue ely_vm_pop(ElyVM* vm);

/**
 * Peeks at a value on the evaluation stack at a given offset from the top without removing it.
 *
 * @param vm Pointer to the active virtual machine.
 * @param distance 0-based distance from the top element (0 = top, 1 = second from top).
 * @return The peeked NaN-boxed Value.
 */
ELY_NODISCARD ElyValue ely_vm_peek(const ElyVM* vm, size_t distance);

/**
 * Clears the evaluation stack, call frames, and active upvalues.
 *
 * Resets the runtime execution state while preserving heap objects, globals, and natives.
 *
 * @param vm Pointer to the active virtual machine.
 */
void ely_vm_reset_stack(ElyVM* vm);

/**
 * Allocates a heap object and returns its NaN-boxed object Value handle.
 *
 * @param vm Pointer to the active virtual machine.
 * @param obj Pointer to the HeapObject variant to store.
 * @return NaN-boxed object Value pointing to the new heap entity.
 */
ELY_NODISCARD ElyValue ely_vm_allocate_object(ElyVM* vm, const ElyHeapObject* obj);

/**
 * Retrieves an immutable read-only pointer to a heap object referenced by a Value handle.
 *
 * Asserts that the value is an object handle and points within valid heap bounds.
 *
 * @param vm Read-only pointer to the active virtual machine.
 * @param value NaN-boxed Value referencing a heap object.
 * @return Read-only pointer to the managed ElyHeapObject.
 */
ELY_NODISCARD const ElyHeapObject* ely_vm_get_object_const(const ElyVM* vm, ElyValue value);

/**
 * Retrieves a mutable pointer to a heap object referenced by a Value handle.
 *
 * Asserts that the value is an object handle and points within valid heap bounds.
 *
 * @param vm Pointer to the active virtual machine.
 * @param value NaN-boxed Value referencing a heap object.
 * @return Mutable pointer to the managed ElyHeapObject.
 */
ELY_NODISCARD ElyHeapObject* ely_vm_get_object(ElyVM* vm, ElyValue value);

/**
 * Binds a native standard function pointer to a global identifier.
 *
 * @param vm Pointer to the active virtual machine.
 * @param name_id Identifier index of the native symbol.
 * @param native Function pointer to the native callable.
 */
void ely_vm_define_native(ElyVM* vm, uint32_t name_id, ElyNativeFn native);

#endif // ELY_VM_VM_H
