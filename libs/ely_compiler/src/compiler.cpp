#include "ely/core/opcode.hpp"
#include <ely/compiler/compiler.hpp>
#include <limits>

namespace ely::compiler {

Compiler::Compiler(std::string_view source, FunctionType type)
    : m_parser(Scanner(source)),
      m_chunk(),
      m_context(type) {}

std::optional<core::Chunk> Compiler::compile() {
    m_parser.advance();

    while (!m_parser.match_token(TokenType::Eof)) {
        declaration();
    }

    emit_return();

    if (m_parser.had_error()) return std::nullopt;
    return std::move(m_chunk);
}

void Compiler::emit_byte(uint8_t byte) noexcept {
    m_chunk.write_byte(byte, current_line());
}

void Compiler::emit_opcode(core::Opcode op) noexcept {
    m_chunk.write_opcode(op, current_line());
}

void Compiler::emit_bytes(uint8_t byte1, uint8_t byte2) noexcept {
    emit_byte(byte1); emit_byte(byte2);
}

void Compiler::emit_return() noexcept {
    emit_opcode(core::Opcode::Nop);
    emit_opcode(core::Opcode::Return);
}

void Compiler::emit_constant(core::Value value) noexcept {
    const auto index = m_chunk.add_constant(std::move(value));

    if (index > std::numeric_limits<uint8_t>::max()) {
        m_parser.error("Too many constants in one chunk.");
        return;
    }

    emit_opcode(core::Opcode::Constant);
    emit_byte(static_cast<uint8_t>(index));
}

size_t Compiler::emit_jump(core::Opcode jump_op) noexcept {
    emit_opcode(jump_op);
    emit_byte(0xFF);
    emit_byte(0xFF);
    return m_chunk.size() - 2;
}

void Compiler::patch_jump(size_t offset) noexcept {
    const auto jump = m_chunk.size() - offset - 2;

    if (jump > std::numeric_limits<uint16_t>::max()) {
        m_parser.error("Too much code to jump over.");
    }

    m_chunk.patch_byte(offset, static_cast<uint8_t>((jump >> 8) & 0xFF));
    m_chunk.patch_byte(offset + 1, static_cast<uint8_t>(jump & 0xFF));
}

void Compiler::emit_loop(size_t loop_start) noexcept {
    emit_opcode(core::Opcode::Loop);

    const auto offset = m_chunk.size() - loop_start + 2;
    if (offset > std::numeric_limits<uint16_t>::max()) {
        m_parser.error("Loop body too large.");
    }

    emit_byte(static_cast<uint8_t>((offset >> 8) & 0xFF));
    emit_byte(static_cast<uint8_t>(offset & 0xFF));
}

}
