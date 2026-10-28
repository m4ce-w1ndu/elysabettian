#include <ely/core/chunk.hpp>

namespace ely::core {

void Chunk::write_byte(uint8_t byte, uint32_t line) {
    uint32_t offset = static_cast<uint32_t>(m_code.size());
    m_code.push_back(byte);

    if (!m_lines.empty() && m_lines.back().line == line) return;

    m_lines.push_back(LineInfo { offset, line });
}

}
