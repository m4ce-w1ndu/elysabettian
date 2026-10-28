#include "ely/core/opcode.hpp"
#include <ely/core/debug.hpp>

#include <cstddef>
#include <format>
#include <iostream>

namespace ely::core {

/**
 * Prints a zero-operand instruction and returns the offset of the next instruction.
 */
[[nodiscard]] size_t simple_instruction(std::string_view name, size_t offset) {
    std::cout << std::format("{}\n", name);
    return offset + 1;
}

/**
 * Prints an instruction that references a constant from the constant pool.
 */
[[nodiscard]] size_t constant_instruction(std::string_view name, const Chunk& chunk, size_t offset) {
    const auto constant_idx = chunk.code()[offset + 1];
    const auto value = chunk.get_constant(constant_idx);
    const auto value_str = value ? value->to_string() : "<unknown>";

    std::cout << std::format("{:<16} {:4} '{}'\n", name, constant_idx, value_str);
    return offset + 2;
}

/**
 * Prints an instruction with a single 1-byte operand (such as stack slots or argument counts).
 */
[[nodiscard]] size_t byte_instruction(std::string_view name, const Chunk& chunk, size_t offset) {
    const auto slot = chunk.code()[offset + 1];
    std::cout << std::format("{:<16} {:4}\n", name, slot);
    return offset + 2;
}

/**
 * Prints a 16-bit jump instruction with relative offset and computed target address.
 */
[[nodiscard]] size_t jump_instruction(std::string_view name, int32_t sign, const Chunk& chunk, size_t offset) {
    const auto hi = static_cast<uint16_t>(chunk.code()[offset + 1]);
    const auto low = static_cast<uint16_t>(chunk.code()[offset + 2]);
    const uint16_t jump = (hi << 8) | low;

    const auto target = static_cast<size_t>(
        static_cast<ptrdiff_t>(offset + 3) + static_cast<ptrdiff_t>(sign) * jump
    );

    std::cout << std::format("{:<16} {:4} -> {}\n", name, offset, target);
    return offset + 3;
}

size_t disassemble_instruction(const Chunk& chunk, size_t offset) {
    std::cout << std::format("{:04} ", offset);

    const auto line = chunk.get_line(offset);
    std::cout << ((offset > 0 && line == chunk.get_line(offset - 1))
        ? "   | "
        : std::format("{:4} ", line));

    if (offset >= chunk.code().size()) {
        std::cout << std::format("Unexpected end of chunk at offset {}\n", offset);
        return offset + 1;
    }

    const auto instruction = chunk.code()[offset];

    if (instruction > static_cast<uint8_t>(Opcode::Import)) {
        std::cout << std::format("Unknown opcode: {:#04x}\n", instruction);
        return offset + 1;
    }

    const auto opcode = static_cast<Opcode>(instruction);

    switch (opcode) {
        case Opcode::Constant: return constant_instruction("const", chunk, offset);
        case Opcode::Nop: return simple_instruction("nop", offset);
        case Opcode::True: return simple_instruction("true", offset);
        case Opcode::False: return simple_instruction("false", offset);
        case Opcode::Pop: return simple_instruction("pop", offset);
        case Opcode::GetLocal: return byte_instruction("getlcl", chunk, offset);
        case Opcode::GetGlobal: return constant_instruction("getglb", chunk, offset);
        case Opcode::DefineGlobal: return constant_instruction("defglb", chunk, offset);
        case Opcode::SetLocal: return byte_instruction("setlcl", chunk, offset);
        case Opcode::SetGlobal: return constant_instruction("setglb", chunk, offset);
        case Opcode::GetUpvalue: return byte_instruction("getupv", chunk, offset);
        case Opcode::SetUpvalue: return byte_instruction("setupv", chunk, offset);
        case Opcode::GetProperty: return constant_instruction("getprp", chunk, offset);
        case Opcode::SetProperty: return constant_instruction("setprp", chunk, offset);
        case Opcode::GetSuper: return constant_instruction("getsup", chunk, offset);
        case Opcode::Equal: return simple_instruction("eq", offset);
        case Opcode::Greater: return simple_instruction("gt", offset);
        case Opcode::Less: return simple_instruction("lt", offset);
        case Opcode::Add: return simple_instruction("add", offset);
        case Opcode::Subtract: return simple_instruction("sub", offset);
        case Opcode::Multiply: return simple_instruction("mul", offset);
        case Opcode::Divide: return simple_instruction("div", offset);
        case Opcode::Not: return simple_instruction("not", offset);
        case Opcode::Negate: return simple_instruction("neg", offset);
        case Opcode::Print: return simple_instruction("print", offset);
        case Opcode::Jump: return jump_instruction("jmp", 1, chunk, offset);
        case Opcode::JumpZero: return jump_instruction("jz", 1, chunk, offset);
        case Opcode::Loop: return jump_instruction("iter", -1, chunk, offset);
        case Opcode::Call: return byte_instruction("call", chunk, offset);
        case Opcode::Invoke: {
            // Fused method lookup + call: takes [const_name: u8, argc: u8]
            auto name_idx = chunk.code()[offset + 1];
            auto argc = chunk.code()[offset + 2];
            auto value = chunk.get_constant(name_idx);

            std::string value_str = value
                ? value->to_string()
                : "<unknown>";

            std::cout << std::format("{:<16} ({} args) {:4} '{}'\n",
                "invk", name_idx, argc, value_str);
            return offset +3;
        }
        case Opcode::SuperInvoke: {
            auto name_idx = chunk.code()[offset + 1];
            auto argc = chunk.code()[offset + 2];
            auto value = chunk.get_constant(name_idx);

            std::string value_str = value
                ? value->to_string()
                : "<unknown>";

            std::cout << std::format("{:<16} ({} args) {:4} '{}'\n",
                "sinvk", name_idx, argc, value_str);

            return offset + 3;
        }
        case Opcode::Closure: {
            auto curr = offset + 1;
            auto const_idx = chunk.code()[curr];
            auto value = chunk.get_constant(const_idx);

            std::string value_str = value
                ? value->to_string()
                : "<unknown>";

            std::cout << std::format("{:<16} {:4} '{}'\n", "closure", const_idx, value_str);
            curr++;

            return curr;
        }
        case Opcode::CloseUpvalue: return simple_instruction("closeupv", offset);
        case Opcode::Return: return simple_instruction("ret", offset);
        case Opcode::Class: return constant_instruction("class", chunk, offset);
        case Opcode::Inherit: return simple_instruction("inherit", offset);
        case Opcode::Method: return constant_instruction("method", chunk, offset);
        case Opcode::BwAnd: return simple_instruction("and", offset);
        case Opcode::BwOr: return simple_instruction("or", offset);
        case Opcode::BwXor: return simple_instruction("xor", offset);
        case Opcode::BwNot: return simple_instruction("bwnot", offset);
        case Opcode::ShiftLeft: return simple_instruction("shl", offset);
        case Opcode::ShiftRight: return simple_instruction("shr", offset);
        case Opcode::ArrayBuild: return byte_instruction("arrbld", chunk, offset);
        case Opcode::ArrayStore: return simple_instruction("arrstr", offset);
        case Opcode::ArrayIndex: return simple_instruction("arridx", offset);
        case Opcode::Import: return byte_instruction("import", chunk, offset);
    }
}

void disassemble_chunk(const Chunk& chunk, std::string_view name) {
    std::cout << std::format("== {} ==\n", name);
    size_t offset = 0;

    while (offset < chunk.size()) offset = disassemble_instruction(chunk, offset);
}

}
