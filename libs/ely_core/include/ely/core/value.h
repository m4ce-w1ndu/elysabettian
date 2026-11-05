#ifndef ELY_CORE_VALUE_H
#define ELY_CORE_VALUE_H

#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>

/**
 * Defines a nodiscard attribute for the function result
 */
#if defined(__GNUC__) || defined(__clang__)
    #define ELY_NODISCARD __attribute__((warn_unused_result))
#elif defined(_MSC_VER)
    #define ELY_NODISCARD _Check_return_
#else
    #define ELY_NODISCARD
#endif

/**
 * Defines the generic language value type used by the VM
 * infrastrucure. It leverages NaN-boxing to store native,
 * standard types.
 */
typedef uint64_t ElyValue;

/**
 * Sign bit (bit 63)
 */
#define ELY_SIGN_BIT 0x8000000000000000ULL

/**
 * Exponent with all bits set to 1 + Quiet NaN bit (51)
 */
#define ELY_QNAN 0x7FFC000000000000ULL

/**
 * NULL type tag
 */
#define ELY_TAG_NULL 0x01

/**
 * Boolean "false" tag
 */
#define ELY_TAG_FALSE 0x02

/**
 * Boolean "true" tag
 */
#define ELY_TAG_TRUE 0x03

/**
 * Full null value representation with NaN-boxing
 */
#define ELY_NULL_VAL (ELY_QNAN | ELY_TAG_NULL)

/**
 * Full "true" value representation with NaN-boxing
 */
#define ELY_TRUE_VAL (ELY_QNAN | ELY_TAG_TRUE)

/**
 * Full "false" value representation with NaN-boxing
 */
#define ELY_FALSE_VAL (ELY_QNAN | ELY_TAG_FALSE)

/**
 * Heap objects, pointers and handles tag
 */
#define ELY_OBJ_TAG (ELY_SIGN_BIT | ELY_QNAN)

/**
 * Bitmask to extract payload from NaN-boxed values
 */
#define ELY_PAYLOAD_MASK 0x0000FFFFFFFFFFFF

/**
 * Creates a new null value using NaN-boxing.
 *
 * @return Value representing the null literal.
 */
ELY_NODISCARD static inline ElyValue ely_value_null(void) { return ELY_NULL_VAL; }

/**
 * Creates a new boolean value using NaN-boxing.
 *
 * @param value Boolean true or false value to wrap.
 * @return Value representing the given boolean.
 */
ELY_NODISCARD static inline ElyValue ely_value_bool(bool value) {
    return value ? ELY_TRUE_VAL : ELY_FALSE_VAL;
}

/**
 * Creates a new numeric value using NaN-boxing.
 *
 * @param value Double-precision floating point number to wrap.
 * @return Value representing the given number.
 */
ELY_NODISCARD static inline ElyValue ely_value_number(double value) {
    ElyValue val;
    memcpy(&val, &value, sizeof(double));
    return val;
}

/**
 * Creates a heap object reference from a 48-bit pointer or identifier.
 *
 * @param payload 48-bit address or object handle.
 * @return Value representing the boxed heap object.
 */
ELY_NODISCARD static inline ElyValue ely_value_obj(uint64_t payload) {
    assert((payload & ~ELY_PAYLOAD_MASK) == 0 && "Payload exceeds 48 bits.");
    return ELY_OBJ_TAG | (payload & ELY_PAYLOAD_MASK);
}

/**
 * Checks if the current value is a standard floating point number.
 *
 * @return True if the value is a number, false otherwise.
 */
ELY_NODISCARD static inline bool ely_value_is_number(ElyValue value) {
    return (value & ELY_QNAN) != ELY_QNAN;
}

/**
 * Checks if the current value is null.
 *
 * @return True if null, false otherwise.
 */
ELY_NODISCARD static inline bool ely_value_is_null(ElyValue value) {
    return value == ELY_NULL_VAL;
}

/**
 * Checks if the current value is a boolean value (true or false).
 *
 * @return True if boolean, false otherwise.
 */
ELY_NODISCARD static inline bool ely_value_is_bool(ElyValue value) {
    return (value | 1) == ELY_TRUE_VAL;
}

/**
 * Checks if the current value is a boxed heap object pointer.
 *
 * @return True if a heap object, false otherwise.
 */
ELY_NODISCARD static inline bool ely_value_is_obj(ElyValue value) {
    return (value & (ELY_SIGN_BIT | ELY_QNAN)) == ELY_OBJ_TAG;
}

/**
 * Checks if the current value is considered truthy.
 *
 * All values except null and false are truthy.
 *
 * @return True if truthy, false if falsy.
 */
ELY_NODISCARD static inline bool ely_value_is_truthy(ElyValue value) {
    return !ely_value_is_null(value) && value != ELY_FALSE_VAL;
}

/**
 * Converts the current NaN-boxed value to a native double.
 *
 * @return Extracted double precision value.
 */
ELY_NODISCARD static inline double ely_value_as_number(ElyValue value) {
    assert(ely_value_is_number(value) && "Value is not a number.");

    double result;
    memcpy(&result, &value, sizeof(double));
    return result;
}

/**
 * Converts the current NaN-boxed value to a native boolean.
 *
 * @return Extracted boolean value.
 */
ELY_NODISCARD static inline bool ely_value_as_bool(ElyValue value) {
    assert(ely_value_is_bool(value) && "Value is not a boolean.");
    return value == ELY_TRUE_VAL;
}

/**
 * Retrieves the 48-bit address payload of a boxed heap object.
 *
 * @return 48-bit address or handle.
 */
ELY_NODISCARD static inline uint64_t ely_value_as_obj(ElyValue value) {
    assert(ely_value_is_obj(value) && "Value is not a heap object.");
    return value & ELY_PAYLOAD_MASK;
}

/**
 * Compares two values for semantic equality.
 *
 * @param a Left ElyValue to compare against.
 * @param b Right ElyValue to compare against.
 * @return True if equal, false otherwise.
 */
ELY_NODISCARD static inline bool ely_value_equal(ElyValue a, ElyValue b) {
    if (ely_value_is_number(a) && ely_value_is_number(b)) {
        return ely_value_as_number(a) == ely_value_as_number(b);
    }
    return a == b;
}

#endif
