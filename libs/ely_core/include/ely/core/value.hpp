#pragma once

#include <cassert>
#include <cstdint>
#include <bit>
#include <string>
#include <format>

namespace ely::core {

/**
 * 64-bit IEEE-754 NaN-boxed value representation.
 *
 * Encodes numbers (f64), booleans, null, and 48-bit heap pointers directly
 * within a single 8-byte value without heap allocation overhead.
 */
class Value {
public:
    /**
     * Default constructor initializing to the null value representation.
     */
    constexpr Value() noexcept : m_bits(NULL_VAL) {}

    /**
     * Creates a new null value using NaN-boxing.
     *
     * @return Value representing the null literal.
     */
    [[nodiscard]] static constexpr Value null() noexcept {
        return Value { NULL_VAL };
    }

    /**
     * Creates a new boolean value using NaN-boxing.
     *
     * @param value Boolean true or false value to wrap.
     * @return Value representing the given boolean.
     */
    [[nodiscard]] static constexpr Value boolean(bool value) noexcept {
        return Value { value ? TRUE_VAL : FALSE_VAL };
    }

    /**
     * Creates a new numeric value using NaN-boxing.
     *
     * @param value Double-precision floating point number to wrap.
     * @return Value representing the given number.
     */
    [[nodiscard]] static constexpr Value number(double value) noexcept {
        return Value { std::bit_cast<uint64_t>(value) };
    }

    /**
     * Creates a heap object reference from a 48-bit pointer or identifier.
     *
     * @param payload 48-bit address or object handle.
     * @return Value representing the boxed heap object.
     */
    [[nodiscard]] static constexpr Value obj(uint64_t payload) noexcept {
        assert((payload & ~PAYLOAD_MASK) == 0 && "Payload exceeds 48 bits.");
        return Value { OBJ_TAG | (payload & PAYLOAD_MASK) };
    }

    /**
     * Checks if the current value is a standard floating point number.
     *
     * @return True if the value is a number, false otherwise.
     */
    [[nodiscard]] constexpr bool is_number() const noexcept {
        return (m_bits & QNAN) != QNAN;
    }

    /**
     * Checks if the current value is null.
     *
     * @return True if null, false otherwise.
     */
    [[nodiscard]] constexpr bool is_null() const noexcept {
        return m_bits == NULL_VAL;
    }

    /**
     * Checks if the current value is a boolean value (true or false).
     *
     * @return True if boolean, false otherwise.
     */
    [[nodiscard]] constexpr bool is_boolean() const noexcept {
        return (m_bits | 1) == TRUE_VAL;
    }

    /**
     * Checks if the current value is a boxed heap object pointer.
     *
     * @return True if a heap object, false otherwise.
     */
    [[nodiscard]] constexpr bool is_obj() const noexcept {
        return (m_bits & (SIGN_BIT | QNAN)) == OBJ_TAG;
    }

    /**
     * Checks if the current value is considered truthy.
     *
     * All values except null and false are truthy.
     *
     * @return True if truthy, false if falsy.
     */
    [[nodiscard]] constexpr bool is_truthy() const noexcept {
        return !(is_null() || (is_boolean() && !as_boolean()));
    }

    /**
     * Converts the current NaN-boxed value to a native double.
     *
     * @return Extracted double precision value.
     */
    [[nodiscard]] constexpr double as_number() const noexcept {
        assert(is_number() && "Value is not a number.");
        return std::bit_cast<double>(m_bits);
    }

    /**
     * Converts the current NaN-boxed value to a native boolean.
     *
     * @return Extracted boolean value.
     */
    [[nodiscard]] constexpr bool as_boolean() const noexcept {
        assert(is_boolean() && "Value is not a boolean.");
        return m_bits == TRUE_VAL;
    }

    /**
     * Retrieves the 48-bit address payload of a boxed heap object.
     *
     * @return 48-bit address or handle.
     */
    [[nodiscard]] constexpr uint64_t as_obj() const noexcept {
        assert(is_obj() && "Value is not a heap object.");
        return m_bits & PAYLOAD_MASK;
    }

    /**
     * Converts the value to its string representation.
     *
     * @return Formatted string describing the value.
     */
    [[nodiscard]] std::string to_string() const {
        if (is_number()) return std::format("{}", as_number());
        if (is_null()) return "null";
        if (is_boolean()) return as_boolean() ? "true" : "false";
        if (is_obj()) return std::format("<object {:#014x}>", as_obj());

        return "<unknown>";
    }

    /**
     * Compares two values for semantic equality.
     *
     * @param other Value to compare against.
     * @return True if equal, false otherwise.
     */
    [[nodiscard]] constexpr bool operator==(const Value& other) const noexcept {
        return (is_number() && other.is_number())
            ? as_number() == other.as_number()
            : m_bits == other.m_bits;
    }
private:
    uint64_t m_bits{0};

    /**
     * Private raw bits constructor.
     *
     * @param value Raw 64-bit unsigned representation.
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
    /// Bitmask to extract payload from NaN-boxed values.
    static constexpr uint64_t PAYLOAD_MASK = 0x0000FFFFFFFFFFFF;
};

} // namespace ely::core
