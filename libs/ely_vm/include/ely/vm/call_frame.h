#ifndef ELY_VM_CALL_FRAME_H
#define ELY_VM_CALL_FRAME_H

#include <ely/core/chunk.h>
#include <ely/core/value.h>

#include <stdint.h>
#include <stddef.h>
#include <assert.h>

/// Invalid closure id to compare for closure validity.
#define ELY_INVALID_CLOSURE_ID SIZE_MAX

/**
 * Represents an active call frame in the virtual machine call stack.
 *
 * Each frame tracks the currently executing bytecode chunk, the instruction
 * pointer (IP), the base index into the VM value stack for local variables,
 * and an optional identifier of the associated closure environment.
 */
typedef struct ElyCallFrame {
    /// Non-owning pointer to the immutable bytecode chunk being executed.
    const ElyChunk* chunk;
    /// Bytecode instruction pointer offset within the chunk.
    size_t ip;
    /// Base index in the VM stack where this frame's local variables begin.
    size_t slots;
    /// Optional index referencing the closure heap object when calling a function.
    size_t closure_id;
} ElyCallFrame;

/**
 * Initialises a call frame for top-level script execution or non-closure functions.
 *
 * @param frame Pointer to the call frame to initialise.
 * @param chunk Non-owning pointer to the bytecode chunk to execute.
 * @param initial_slots Base index offset into the VM stack for locals.
 */
static inline void ely_call_frame_init(ElyCallFrame* frame, const ElyChunk* chunk, size_t initial_slots) {
    assert(frame != NULL && "Call frame pointer is NULL.");
    assert(chunk != NULL && "Chunk pointer is NULL.");

    frame->chunk = chunk;
    frame->ip = 0;
    frame->slots = initial_slots;
    frame->closure_id = ELY_INVALID_CLOSURE_ID;
}

/**
 * Initialises a call frame associated with a specific closure instance.
 *
 * @param frame Pointer to the call frame to initialise.
 * @param chunk Non-owning pointer to the bytecode chunk to execute.
 * @param initial_slots Base index offset into the VM stack for locals.
 * @param closure_id Identifier of the closure in the VM heap.
 */
static inline void ely_call_frame_init_with_closure(ElyCallFrame* frame, const ElyChunk* chunk, size_t initial_slots, size_t closure_id) {
    assert(frame != NULL && "Call frame pointer is NULL.");
    assert(chunk != NULL && "Chunk pointer is NULL.");

    ely_call_frame_init(frame, chunk, initial_slots);
    frame->closure_id = closure_id;
}

/**
 * Reads the next byte from the instruction stream and advances the instruction pointer.
 *
 * @param frame Active call frame.
 * @return Next instruction byte or operand.
 */
ELY_NODISCARD static inline uint8_t ely_call_frame_read_byte(ElyCallFrame* frame) {
    assert(frame != NULL && "Call frame pointer is NULL.");
    assert(frame->chunk != NULL && "Chunk pointer is NULL.");
    assert(frame->ip < frame->chunk->code_count && "Instruction pointer out of bounds.");

    return frame->chunk->code[frame->ip++];
}

/**
 * Reads an unsigned 16-bit big-endian integer from the instruction stream.
 *
 * Advances the instruction pointer by 2 bytes (used for jump offsets).
 *
 * @param frame Active call frame.
 * @return 16-bit unsigned integer operand.
 */
ELY_NODISCARD static inline uint16_t ely_call_frame_read_short(ElyCallFrame* frame) {
    assert(frame != NULL && "Call frame pointer is NULL.");
    assert(frame->chunk != NULL && "Chunk pointer is NULL.");
    assert(frame->ip + 1 < frame->chunk->code_count && "Short operand out of bounds.");

    frame->ip += 2;
    const uint16_t left = (uint16_t)((uint16_t)frame->chunk->code[frame->ip - 2] << 8);
    const uint16_t right = (uint16_t)frame->chunk->code[frame->ip - 1];
    return (uint16_t)(left | right);
}

/**
 * Reads an 8-bit constant index operand and retrieves the corresponding Value from the pool.
 *
 * @param frame Active call frame.
 * @return Value constant from the chunk's constant pool.
 */
ELY_NODISCARD static inline ElyValue ely_call_frame_read_constant(ElyCallFrame* frame) {
    const size_t index = (size_t)ely_call_frame_read_byte(frame);
    assert(index < frame->chunk->constants_count && "Constant index out of bounds.");
    return frame->chunk->constants[index];
}

#endif // ELY_VM_CALL_FRAME_H
