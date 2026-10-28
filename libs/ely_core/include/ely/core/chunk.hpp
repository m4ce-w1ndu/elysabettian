#pragma once

#include "opcode.hpp"
#include "value.hpp"
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <vector>
#include <optional>

namespace ely::core {

/**
 * Represents a source line mapping checkpoint in compiled bytecode.
 *
 * Stores the starting bytecode offsfet where a particular source line begins,
 * enabling compact line metadata storage and fast logarithmic lookup via binary search.
 */
struct LineInfo {
    /// Bytecode offset where instructions for this line start.
    uint32_t offset;
    /// Source code line number corresponding to this code offset.
    uint32_t line;
};

class Chunk;

/**
 * Describes the concept of a function, with its associated arity, bytecode stream
 * and signature name.
 */
struct FunctionDescriptor {
    /// The arity of the function (number of parameters).
    size_t arity;
    /// Code chunk associated with this function.
    std::unique_ptr<Chunk> chunk;
    /// Name of this function, if present.
    std::optional<std::string> name;

    FunctionDescriptor() = default;
    ~FunctionDescriptor() = default;

    /**
     * Only move semantics are allowed.
     */
    FunctionDescriptor(FunctionDescriptor&&) noexcept = default;
    FunctionDescriptor& operator=(FunctionDescriptor&&) noexcept = default;
    FunctionDescriptor(const FunctionDescriptor&) = delete;
    FunctionDescriptor& operator=(const FunctionDescriptor&) = delete;
};

/**
 * A contiguous chunk of compiled bytecode, constant pool values, and debug line mappings.
 *
 * `Chunk` acts as the primary compilation unit emitted by the frontend compiler
 * and directly consumed by the virtual machine interpreter or lowered to Cranelift IR.
 */
class Chunk {
public:
    /**
     * Appends a raw byte to the instruction stream and records it source line mapping.
     *
     * To optimise memory usage, a new `LineInfo` checkpoint is only recorded
     * when the source line differs from the preceding instruction.
     */
    void write_byte(uint8_t byte, uint32_t line);

    /**
     * Appends a typed opcode instruction to the bytecode stream.
     */
    void write_opcode(Opcode opcode, uint32_t line);

    /**
     * Appends a value to the chunk's constant pool and returns its index.
     */
    [[nodiscard]] size_t add_constant(Value value);

    /**
     * Resolves the source line number for a given bytecode instruction offset
     * using binary search over the compact `LineInfo` table.
     */
    [[nodiscard]] int get_line(size_t offset) const noexcept;

    /**
     * Accesses the underlying bytecode stream buffer with read-only access.
     */
    [[nodiscard]] std::span<const uint8_t> code() const noexcept { return m_code; }

    /**
     * Accesses the underlying constants stream buffer with read-only access.
     */
    [[nodiscard]] std::span<const Value> constants() const noexcept { return m_constants; }

    /**
     * Returns the size of the bytecode stream (number of instructions and operands).
     */
    [[nodiscard]] size_t size() const noexcept { return m_code.size(); }

    /**
     * Checks if the machine has an empty execution buffer.
     */
    [[nodiscard]] bool empty() const noexcept { return m_code.empty(); }

    /**
     * Retrieves a value from the constant pool by its index.
     */
    [[nodiscard]] std::optional<Value> get_constant(size_t index) const noexcept {
        if (index >= m_constants.size()) return std::nullopt;
        return std::optional(m_constants[index]);
    }

    /**
     * Appends a string literal constant to the chunk and returns its 0-based index.
     */
    [[nodiscard]] size_t add_string_constant(std::string str) {
        m_string_constants.push_back(std::move(str));
        return m_string_constants.size() - 1;
    }

    /**
     * Appends a compiled function descriptor to the chunk and returns its 0-based index.
     */
    [[nodiscard]] size_t add_function_constant(FunctionDescriptor fd) {
        m_function_constants.push_back(std::move(fd));
        return m_function_constants.size() - 1;
    }
private:
    /// Raw bytecode instruction stream and inline operands.
    std::vector<uint8_t> m_code;
    /// Constant values referenced by bytecode instructions via indexing.
    std::vector<Value> m_constants;
    /// Compact line metadata table sorted by bytecode offset.
    std::vector<LineInfo> m_lines;
    /// String literal constants loaded into the VM heap during chunk initialisation.
    std::vector<std::string> m_string_constants;
    /// Compiled function prototypes declared within this chunk.
    std::vector<FunctionDescriptor> m_function_constants;
};

}
