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

    /**
     * Checks if the current value is a number.
     */
    [[nodiscard]] constexpr bool is_number() const noexcept {
        return (m_bits & QNAN) != QNAN;
    }

    /**
     * Checks if the current value is null.
     */
    [[nodiscard]] constexpr bool is_null() const noexcept {
        return m_bits == NULL_VAL;
    }

    /**
     * Checks if the current value is a boolean value.
     */
    [[nodiscard]] constexpr bool is_boolean() const noexcept {
        return (m_bits | 1) == TRUE_VAL;
    }

    /**
     * Checks if the current value is a heap object.
     */
    [[nodiscard]] constexpr bool is_obj() const noexcept {
        return (m_bits & (SIGN_BIT | QNAN)) == OBJ_TAG;
    }

    /**
     * Checks if the current value is "truthy".
     * All values that are NOT "false" and "null" are considered "truthy".
     */
    [[nodiscard]] constexpr bool is_truthy() const noexcept {
        return !(is_null() || (is_boolean() && !as_boolean()));
    }

    /**
     * Converts the current NaN-boxed value to a number.
     */
    [[nodiscard]] constexpr double as_number() const noexcept {
        assert(is_number() && "Value is not a number.");
        return std::bit_cast<double>(m_bits);
    }

    /**
     * Converts the current NaN-boxed value to a boolean.
     */
    [[nodiscard]] constexpr bool as_boolean() const noexcept {
        assert(is_boolean() && "Value is not a boolean.");
        return m_bits == TRUE_VAL;
    }

    /**
     * Retrieves the payload of a NaN-boxed memory heap object.
     */
    [[nodiscard]] constexpr uint64_t as_obj() const noexcept {
        assert(is_obj() && "Value is not a heap object.");
        return m_bits & PAYLOAD_MASK;
    }

    /**
     * Compares two values to check for equality.
     */
    [[nodiscard]] constexpr bool operator==(const Value& other) const noexcept {
        return (is_number() && other.is_number())
            ? as_number() == other.as_number()
            : m_bits == other.m_bits;
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
