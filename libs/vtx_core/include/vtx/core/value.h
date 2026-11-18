#ifndef VTX_CORE_VALUE_H
#define VTX_CORE_VALUE_H

#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>

/**
 * Defines a nodiscard attribute for the function result
 */
#if defined(__GNUC__) || defined(__clang__)
    #define VTX_NODISCARD __attribute__((warn_unused_result))
#elif defined(_MSC_VER)
    #define VTX_NODISCARD _Check_return_
#else
    #define VTX_NODISCARD
#endif

/**
 * Defines the generic language value type used by the VM
 * infrastrucure. It leverages NaN-boxing to store native,
 * standard types.
 */
typedef uint64_t VtxValue;

/**
 * Sign bit (bit 63)
 */
#define VTX_SIGN_BIT 0x8000000000000000ULL

/**
 * Exponent with all bits set to 1 + Quiet NaN bit (51)
 */
#define VTX_QNAN 0x7FFC000000000000ULL

/**
 * NULL type tag
 */
#define VTX_TAG_NULL 0x01

/**
 * Boolean "false" tag
 */
#define VTX_TAG_FALSE 0x02

/**
 * Boolean "true" tag
 */
#define VTX_TAG_TRUE 0x03

/**
 * Full null value representation with NaN-boxing
 */
#define VTX_NULL_VAL (VTX_QNAN | VTX_TAG_NULL)

/**
 * Full "true" value representation with NaN-boxing
 */
#define VTX_TRUE_VAL (VTX_QNAN | VTX_TAG_TRUE)

/**
 * Full "false" value representation with NaN-boxing
 */
#define VTX_FALSE_VAL (VTX_QNAN | VTX_TAG_FALSE)

/**
 * Heap objects, pointers and handles tag
 */
#define VTX_OBJ_TAG (VTX_SIGN_BIT | VTX_QNAN)

/**
 * Bitmask to extract payload from NaN-boxed values
 */
#define VTX_PAYLOAD_MASK 0x0000FFFFFFFFFFFFULL

/**
 * Creates a new null value using NaN-boxing.
 *
 * @return Value representing the null literal.
 */
VTX_NODISCARD static inline VtxValue vtx_value_null(void) { return VTX_NULL_VAL; }

/**
 * Creates a new boolean value using NaN-boxing.
 *
 * @param value Boolean true or false value to wrap.
 * @return Value representing the given boolean.
 */
VTX_NODISCARD static inline VtxValue vtx_value_bool(bool value) {
    return value ? VTX_TRUE_VAL : VTX_FALSE_VAL;
}

/**
 * Creates a new numeric value using NaN-boxing.
 *
 * @param value Double-precision floating point number to wrap.
 * @return Value representing the given number.
 */
VTX_NODISCARD static inline VtxValue vtx_value_number(double value) {
    VtxValue val;
    memcpy(&val, &value, sizeof(double));
    return val;
}

/**
 * Creates a heap object reference from a 48-bit pointer or identifier.
 *
 * @param payload 48-bit address or object handle.
 * @return Value representing the boxed heap object.
 */
VTX_NODISCARD static inline VtxValue vtx_value_obj(uint64_t payload) {
    assert((payload & ~VTX_PAYLOAD_MASK) == 0 && "Payload exceeds 48 bits.");
    return VTX_OBJ_TAG | (payload & VTX_PAYLOAD_MASK);
}

/**
 * Checks if the current value is a standard floating point number.
 *
 * @return True if the value is a number, false otherwise.
 */
VTX_NODISCARD static inline bool vtx_value_is_number(VtxValue value) {
    return (value & VTX_QNAN) != VTX_QNAN;
}

/**
 * Checks if the current value is null.
 *
 * @return True if null, false otherwise.
 */
VTX_NODISCARD static inline bool vtx_value_is_null(VtxValue value) {
    return value == VTX_NULL_VAL;
}

/**
 * Checks if the current value is a boolean value (true or false).
 *
 * @return True if boolean, false otherwise.
 */
VTX_NODISCARD static inline bool vtx_value_is_bool(VtxValue value) {
    return (value | 1) == VTX_TRUE_VAL;
}

/**
 * Checks if the current value is a boxed heap object pointer.
 *
 * @return True if a heap object, false otherwise.
 */
VTX_NODISCARD static inline bool vtx_value_is_obj(VtxValue value) {
    return (value & (VTX_SIGN_BIT | VTX_QNAN)) == VTX_OBJ_TAG;
}

/**
 * Checks if the current value is considered truthy.
 *
 * All values except null and false are truthy.
 *
 * @return True if truthy, false if falsy.
 */
VTX_NODISCARD static inline bool vtx_value_is_truthy(VtxValue value) {
    return !vtx_value_is_null(value) && value != VTX_FALSE_VAL;
}

/**
 * Converts the current NaN-boxed value to a native double.
 *
 * @return Extracted double precision value.
 */
VTX_NODISCARD static inline double vtx_value_as_number(VtxValue value) {
    assert(vtx_value_is_number(value) && "Value is not a number.");

    double result;
    memcpy(&result, &value, sizeof(double));
    return result;
}

/**
 * Converts the current NaN-boxed value to a native boolean.
 *
 * @return Extracted boolean value.
 */
VTX_NODISCARD static inline bool vtx_value_as_bool(VtxValue value) {
    assert(vtx_value_is_bool(value) && "Value is not a boolean.");
    return value == VTX_TRUE_VAL;
}

/**
 * Retrieves the 48-bit address payload of a boxed heap object.
 *
 * @return 48-bit address or handle.
 */
VTX_NODISCARD static inline uint64_t vtx_value_as_obj(VtxValue value) {
    assert(vtx_value_is_obj(value) && "Value is not a heap object.");
    return value & VTX_PAYLOAD_MASK;
}

/**
 * Compares two values for semantic equality.
 *
 * @param a Left VtxValue to compare against.
 * @param b Right VtxValue to compare against.
 * @return True if equal, false otherwise.
 */
VTX_NODISCARD static inline bool vtx_value_equal(VtxValue a, VtxValue b) {
    if (vtx_value_is_number(a) && vtx_value_is_number(b)) {
        return vtx_value_as_number(a) == vtx_value_as_number(b);
    }
    return a == b;
}

#endif
