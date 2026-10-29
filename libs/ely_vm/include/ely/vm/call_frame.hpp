#pragma once

#include <ely/core/chunk.hpp>
#include <ely/core/value.hpp>
#include <cstddef>
#include <cstdint>
#include <optional>

namespace ely::vm {

/**
 * Represents an active call frame in the virtual machine call stack.
 *
 * Each frame tracks the currently executing bytecode chunk, the instruction
 * pointer (IP), the base index into the VM value stack for local variables,
 * and an optional identifier of the associated closure environment.
 */
struct CallFrame {
    /// Non-owning pointer to the immutable bytecode chunk being executed.
    const core::Chunk* chunk{nullptr};
    /// Bytecode instruction pointer offset within the chunk.
    size_t ip{0};
    /// Base index in the VM stack where this frame's local variables begin.
    size_t slots{0};
    /// Optional index referencing the closure heap object when calling a function.
    std::optional<size_t> closure_id{std::nullopt};

    /**
     * Constructs a call frame for top-level script execution or non-closure functions.
     *
     * @param chunk Non-owning pointer to the bytecode chunk.
     * @param slots Base index offset into the VM stack for locals.
     */
    explicit CallFrame(const core::Chunk* chunk, size_t slots) noexcept
        : chunk(chunk), slots(slots) {}

    /**
     * Constructs a call frame associated with a specific closure instance.
     *
     * @param chunk Non-owning pointer to the bytecode chunk.
     * @param slots Base index offset into the VM stack for locals.
     * @param closure_id Identifier of the closure in the VM heap.
     */
    explicit CallFrame(const core::Chunk* chunk, size_t slots, size_t closure_id) noexcept
        : chunk(chunk), slots(slots), closure_id(closure_id) {}

    /**
     * Reads the next byte from the instruction stream and advances the instruction pointer.
     *
     * @return Next instruction byte or operand.
     */
    [[nodiscard]] uint8_t read_byte() noexcept {
        return chunk->code()[ip++];
    }

    /**
     * Reads an unsigned 16-bit big-endian integer from the instruction stream.
     *
     * Advances the instruction pointer by 2 bytes. Used for jump offsets.
     *
     * @return 16-bit unsigned integer operand.
     */
    [[nodiscard]] uint16_t read_short() noexcept {
        ip += 2;
        const auto left = static_cast<uint16_t>(static_cast<uint16_t>(chunk->code()[ip - 2]) << 8);
        const auto right = static_cast<uint16_t>(chunk->code()[ip - 1]);
        return static_cast<uint16_t>(left | right);
    }

    /**
     * Reads a constant index operand byte and retrieves the corresponding Value from the pool.
     *
     * @return Value constant from the chunk's constant pool.
     */
    [[nodiscard]] core::Value read_constant() noexcept {
        const auto index = static_cast<size_t>(read_byte());
        return chunk->constants()[index];
    }
};

} // namespace ely::vm
