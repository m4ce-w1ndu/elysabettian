#ifndef ELY_CORE_DEBUG_H
#define ELY_CORE_DEBUG_H

#include "chunk.h"
#include "value.h"
#include <stddef.h>

/**
 * Disassembles all instructions contained in a Chunk with a header label.
 *
 * @param chunk Pointer to the Chunk structure to disassemble.
 * @param name Header title or function name to display.
 */
void ely_debug_disassemble_chunk(const ElyChunk* chunk, const char* name);

/**
 * Disassembles a single instruction at the specified bytecode offset.
 *
 * Prints the byte offset, source line information, instruction name, and operand details,
 * returning the bytecode offset of the subsequent instruction.
 *
 * @param chunk Pointer to the Chunk containing the bytecode.
 * @param offset Bytecode offset of the instruction to disassemble.
 * @return Bytecode offset of the next instruction.
 */
ELY_NODISCARD size_t ely_debug_disassemble_instruction(const ElyChunk* chunk, size_t offset);

#endif
