#pragma once

#include "chunk.hpp"
#include <string_view>

namespace ely::core {

/**
 * Disassembles all instructions contained in a Chunk with a header label.
 */
void disassemble_chunk(const Chunk& chunk, std::string_view name);

/**
 * Disassembles a single instruction at the specified bytecode offset.
 *
 * Prints the byte offset, source line information, instruction name, and operand details,
 * returning the bytecode offset of the subsequent instruction.
 */
size_t disassemble_instruction(const Chunk& chunk, size_t offset);

}
