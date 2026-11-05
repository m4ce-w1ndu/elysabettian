#ifndef ELY_CORE_CHUNK_H
#define ELY_CORE_CHUNK_H

#include "value.h"

/**
 * Represents a source line mapping checkpoint in compiled bytecode.
 *
 * Stores the starting bytecode offset where a particular source line begins,
 * enabling compact line metadata storage and fast logarithmic lookup via binary search.
 */
typedef struct ElyLineInfo {
    uint32_t offset;
    uint32_t line;
} ElyLineInfo;

/**
 * A contiguous chunk of compiled bytecode, constant pool values, and debug line mappings.
 *
 * `Chunk` acts as the primary compilation unit emitted by the frontend compiler
 * and directly consumed by the virtual machine interpreter or lowered to native machine code.
 */
typedef struct ElyChunk {
    size_t code_count;
    size_t code_capacity;
    uint8_t* code;

    size_t constants_count;
    size_t constants_capacity;
    ElyValue* constants;

    size_t lines_count;
    size_t lines_capacity;
    ElyLineInfo* lines;
} ElyChunk;

/**
 * Initialises an empty bytecode chunk with zero capacity and null buffers.
 *
 * @param chunk Pointer to the Chunk structure to initialise.
 */
void ely_chunk_init(ElyChunk* chunk);

/**
 * Releases all dynamically allocated heap memory associated with a bytecode chunk
 * and resets its counters to zero.
 *
 * @param chunk Pointer to the Chunk structure to deallocate.
 */
void ely_chunk_free(ElyChunk* chunk);

/**
 * Appends a raw byte to the instruction stream and records its source line mapping.
 *
 * To optimise memory usage, a new `ElyLineInfo` checkpoint is only recorded
 * when the source line differs from the preceding instruction.
 *
 * @param chunk Pointer to the target chunk.
 * @param byte Raw 8-bit byte or opcode operand to write.
 * @param line 1-based source code line corresponding to this byte.
 */
void ely_chunk_write(ElyChunk* chunk, uint8_t byte, size_t line);

/**
 * Appends a value to the chunk's constant pool and returns its index.
 *
 * @param chunk Pointer to the target chunk.
 * @param value The ElyValue to store in the pool.
 * @return 0-based index of the newly added constant.
 */
ELY_NODISCARD size_t ely_chunk_add_constant(ElyChunk* chunk, ElyValue value);

/**
 * Resolves the source line number for a given bytecode instruction offset
 * using logarithmic binary search over the compact checkpoint table.
 *
 * @param chunk Pointer to the target chunk.
 * @param instruction_offset Bytecode offset within the instruction stream.
 * @return Source line number associated with the given offset, or 0 if empty.
 */
ELY_NODISCARD size_t ely_chunk_get_line(const ElyChunk* chunk, size_t instruction_offset);

#endif
