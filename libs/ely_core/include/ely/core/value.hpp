#pragma once

#include <cassert>
#include <cstdint>
#include <bit>

namespace ely::core {

class Value {
public:
    /**
     * Default constructor has no purpose except constructing containers
     * and collections.
     */
    constexpr Value() noexcept : m_bits(NULL_VAL) {}

    /**
     * Creates a new, "null" value using NaN-boxing.
     */
    [[nodiscard]] static constexpr Value null() noexcept {
        return Value { NULL_VAL };
    }

    /**
     * Creates a new boolean value using NaN-boxing.
     */
    [[nodiscard]] static constexpr Value boolean(bool value) noexcept {
        return Value { value ? TRUE_VAL : FALSE_VAL };
    }

    /**
     * Creates a new numeric value using NaN-boxing and number
     * canonicalisation to avoid NaN collisions.
     */
    [[nodiscard]] static constexpr Value number(double value) noexcept {
        return Value { std::bit_cast<uint64_t>(value) };
    }

    /**
     * Creates a heap object reference from an identifier or a 48-bit pointer.
     */
    [[nodiscard]] static constexpr Value obj(uint64_t payload) noexcept {
        assert((payload & ~PAYLOAD_MASK) == 0 && "Payload exceeds 48 bits.");
        return Value { OBJ_TAG | (payload & PAYLOAD_MASK) };
    }
private:
    uint64_t m_bits{0};

    /**
     * Private constructor to handle value conversions.
     */
    constexpr Value(uint64_t value) noexcept : m_bits(value) {}

    /// Sign bit (bit 63)
    static constexpr uint64_t SIGN_BIT = 0x8000000000000000;
    /// Exponent with all bits set to 1 + Quiet NaN bit (51)
    static constexpr uint64_t QNAN = 0x7FFC000000000000;
    /// NULL type tag.
    static constexpr uint64_t TAG_NULL = 0x1;
    /// Boolean "false" tag.
    static constexpr uint64_t TAG_FALSE = 0x2;
    /// Boolean "true" tag.
    static constexpr uint64_t TAG_TRUE = 0x3;

    /// Full null value representation with NaN-boxing.
    static constexpr uint64_t NULL_VAL = QNAN | TAG_NULL;
    /// Full "true" value representation with NaN-boxing.
    static constexpr uint64_t TRUE_VAL = QNAN | TAG_TRUE;
    /// Full "false" value representation with NaN-boxing.
    static constexpr uint64_t FALSE_VAL = QNAN | TAG_FALSE;

    /// Heap objects, pointers and handles tag.
    static constexpr uint64_t OBJ_TAG = SIGN_BIT | QNAN;
    /// Bitmask to extract payload form NaN-boxed values.
    static constexpr uint64_t PAYLOAD_MASK = 0x0000FFFFFFFFFFFF;
};

}
