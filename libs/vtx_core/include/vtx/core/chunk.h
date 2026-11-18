#ifndef VTX_CORE_CHUNK_H
#define VTX_CORE_CHUNK_H

#include "value.h"

/**
 * Represents a source line mapping checkpoint in compiled bytecode.
 *
 * Stores the starting bytecode offset where a particular source line begins,
 * enabling compact line metadata storage and fast logarithmic lookup via binary search.
 */
typedef struct VtxLineInfo {
    uint32_t offset;
    uint32_t line;
} VtxLineInfo;

struct VtxChunk;

/**
 * Describes a compiled function, its parameter arity, isolated bytecode stream,
 * and optional identifier name.
 */
typedef struct VtxFunctionDescriptor {
    /// Number of parameters accepted by the function.
    size_t arity;
    /// Pointer to the function's isolated bytecode chunk.
    struct VtxChunk* chunk;
    /// Optional function name string in dynamic heap memory.
    char* name;
} VtxFunctionDescriptor;

/**
 * A contiguous chunk of compiled bytecode, constant pool values, and debug line mappings.
 *
 * `Chunk` acts as the primary compilation unit emitted by the frontend compiler
 * and directly consumed by the virtual machine interpreter or lowered to native machine code.
 */
typedef struct VtxChunk {
    /// Number of bytes in the instruction stream.
    size_t code_count;
    /// Maximum capacity of the instruction stream buffer.
    size_t code_capacity;
    /// Pointer to instruction stream buffer.
    uint8_t* code;

    /// Number of constants in this code chunk.
    size_t constants_count;
    /// Maximum capacity of this chunk's constant pool.
    size_t constants_capacity;
    /// Pointer to constants pool.
    VtxValue* constants;

    /// Number of lines in this chunk.
    size_t lines_count;
    /// Maximum capacity of this chunk's line tracking pool.
    size_t lines_capacity;
    /// Pointer to RLE line trucking pool.
    VtxLineInfo* lines;

    /// Number of string constants in this chunk.
    size_t string_constants_count;
    /// Maximum capacity of string constants pool.
    size_t string_constants_capacity;
    /// Pointer to string constants pool.
    char** string_constants;

    /// Number of function constants in this chunk.
    size_t function_constants_count;
    /// Maximum capacity of function constants pool.
    size_t function_constants_capacity;
    /// Pointer to function constants pool.
    VtxFunctionDescriptor* function_constants;
} VtxChunk;

/**
 * Initialises an empty bytecode chunk with zero capacity and null buffers.
 *
 * @param chunk Pointer to the Chunk structure to initialise.
 */
void vtx_chunk_init(VtxChunk* chunk);

/**
 * Releases all dynamically allocated heap memory associated with a bytecode chunk
 * and resets its counters to zero.
 *
 * @param chunk Pointer to the Chunk structure to deallocate.
 */
void vtx_chunk_free(VtxChunk* chunk);

/**
 * Appends a raw byte to the instruction stream and records its source line mapping.
 *
 * To optimise memory usage, a new `VtxLineInfo` checkpoint is only recorded
 * when the source line differs from the preceding instruction.
 *
 * @param chunk Pointer to the target chunk.
 * @param byte Raw 8-bit byte or opcode operand to write.
 * @param line 1-based source code line corresponding to this byte.
 */
void vtx_chunk_write(VtxChunk* chunk, uint8_t byte, size_t line);

/**
 * Appends a value to the chunk's constant pool and returns its index.
 *
 * @param chunk Pointer to the target chunk.
 * @param value The VtxValue to store in the pool.
 * @return 0-based index of the newly added constant.
 */
VTX_NODISCARD size_t vtx_chunk_add_constant(VtxChunk* chunk, VtxValue value);

/**
 * Appends a string literal constant to the chunk's string pool and returns its index.
 *
 * Duplicates the string slice in dynamic heap memory.
 *
 * @param chunk Pointer to the target chunk.
 * @param str Pointer to the beginning of the string buffer.
 * @param len Length of the string in bytes.
 * @return 0-based index of the newly added string constant.
 */
VTX_NODISCARD size_t vtx_chunk_add_string_constant(VtxChunk* chunk, const char* str, size_t len);

/**
 * Appends a function descriptor to the chunk's function pool and returns its index.
 *
 * @param chunk Pointer to the target chunk.
 * @param fd Function descriptor to store in the pool.
 * @return 0-based index of the newly added function descriptor.
 */
VTX_NODISCARD size_t vtx_chunk_add_function_constant(VtxChunk* chunk, VtxFunctionDescriptor fd);

/**
 * Resolves the source line number for a given bytecode instruction offset
 * using logarithmic binary search over the compact checkpoint table.
 *
 * @param chunk Pointer to the target chunk.
 * @param instruction_offset Bytecode offset within the instruction stream.
 * @return Source line number associated with the given offset, or 0 if empty.
 */
VTX_NODISCARD size_t vtx_chunk_get_line(const VtxChunk* chunk, size_t instruction_offset);

#endif
