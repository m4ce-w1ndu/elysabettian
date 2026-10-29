#include <cassert>
#include <ely/core/chunk.hpp>

#include <algorithm>

namespace ely::core {

void Chunk::write_byte(uint8_t byte, uint32_t line) {
    const auto offset = static_cast<uint32_t>(m_code.size());
    m_code.push_back(byte);

    if (!m_lines.empty() && m_lines.back().line == line) return;

    m_lines.push_back(LineInfo { offset, line });
}

void Chunk::write_opcode(Opcode opcode, uint32_t line) {
    const auto byte = static_cast<uint8_t>(opcode);
    write_byte(byte, line);
}

size_t Chunk::add_constant(Value value) {
    m_constants.push_back(value);
    return m_constants.size() - 1;
}

int32_t Chunk::get_line(size_t offset) const noexcept {
    if (m_lines.empty()) return 0;

    const auto target = static_cast<uint32_t>(offset);

    const auto it = std::ranges::lower_bound(
        m_lines,
        target,
        {},
        &LineInfo::offset
    );

    if (it == m_lines.begin()) return static_cast<int32_t>(it->line);
    if (it == m_lines.end() || it->offset > target) return static_cast<int32_t>(std::prev(it)->line);

    return static_cast<int32_t>(it->line);
}

void Chunk::patch_byte(size_t offset, uint8_t byte) noexcept {
    assert(offset < m_code.size() && "Patch offset out of bounds.");
    m_code[offset] = byte;
}

}
