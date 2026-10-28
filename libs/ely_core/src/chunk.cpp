#include <ely/core/chunk.hpp>

#include <algorithm>

namespace ely::core {

void Chunk::write_byte(uint8_t byte, uint32_t line) {
    uint32_t offset = static_cast<uint32_t>(m_code.size());
    m_code.push_back(byte);

    if (!m_lines.empty() && m_lines.back().line == line) return;

    m_lines.push_back(LineInfo { offset, line });
}

void Chunk::write_opcode(Opcode opcode, uint32_t line) {
    uint8_t byte = static_cast<uint8_t>(opcode);
    write_byte(byte, line);
}

size_t Chunk::add_constant(Value value) {
    m_constants.push_back(value);
    return m_constants.size() - 1;
}

int Chunk::get_line(size_t offset) const noexcept {
    if (m_lines.empty()) return 0;

    uint32_t target = static_cast<uint32_t>(offset);

    auto it = std::ranges::lower_bound(
        m_lines,
        target,
        {},
        &LineInfo::offset
    );

    if (it == m_lines.begin()) return it->line;
    if (it == m_lines.end() || it->offset > target) --it;

    return it->line;
}

}
