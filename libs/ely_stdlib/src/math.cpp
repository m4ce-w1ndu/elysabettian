#include <ely/stdlib/math.hpp>
#include <ely/core/value.hpp>
#include <ely/stdlib/module.hpp>

#include <algorithm>
#include <cmath>
#include <limits>
#include <numbers>
#include <span>

namespace ely::stdlib {

namespace {

/**
 * @brief Validates that the argument span contains at least @p required_count numeric arguments.
 *
 * @param args Span of incoming Value arguments.
 * @param required_count Minimum number of arguments required.
 * @return True if args.size() >= required_count and all required arguments are numbers.
 */
[[nodiscard]] bool has_numeric_args(std::span<const core::Value> args, size_t required_count) noexcept {
    if (args.size() < required_count) return false;
    for (size_t i = 0; i < required_count; ++i) {
        if (!args[i].is_number()) return false;
    }
    return true;
}

/**
 * @brief Computes the square root of a number: `sqrt(x)`
 */
core::Value math_sqrt(std::span<const core::Value> args) {
    if (!has_numeric_args(args, 1)) return core::Value::null();
    return core::Value::number(std::sqrt(args[0].as_number()));
}

/**
 * @brief Computes base raised to the power exponent: `pow(base, exp)`
 */
core::Value math_pow(std::span<const core::Value> args) {
    if (!has_numeric_args(args, 2)) return core::Value::null();
    return core::Value::number(std::pow(args[0].as_number(), args[1].as_number()));
}

/**
 * @brief Computes the absolute value of a number: `abs(x)`
 */
core::Value math_abs(std::span<const core::Value> args) {
    if (!has_numeric_args(args, 1)) return core::Value::null();
    return core::Value::number(std::abs(args[0].as_number()));
}

/**
 * @brief Rounds a number downward to the nearest integer: `floor(x)`
 */
core::Value math_floor(std::span<const core::Value> args) {
    if (!has_numeric_args(args, 1)) return core::Value::null();
    return core::Value::number(std::floor(args[0].as_number()));
}

/**
 * @brief Rounds a number upward to the nearest integer: `ceil(x)`
 */
core::Value math_ceil(std::span<const core::Value> args) {
    if (!has_numeric_args(args, 1)) return core::Value::null();
    return core::Value::number(std::ceil(args[0].as_number()));
}

/**
 * @brief Rounds a number to the nearest integer: `round(x)`
 */
core::Value math_round(std::span<const core::Value> args) {
    if (!has_numeric_args(args, 1)) return core::Value::null();
    return core::Value::number(std::round(args[0].as_number()));
}

/**
 * @brief Returns the integer part of a number by dropping fractional digits: `trunc(x)`
 */
core::Value math_trunc(std::span<const core::Value> args) {
    if (!has_numeric_args(args, 1)) return core::Value::null();
    return core::Value::number(std::trunc(args[0].as_number()));
}

/**
 * @brief Returns the sine of a radian angle: `sin(x)`
 */
core::Value math_sin(std::span<const core::Value> args) {
    if (!has_numeric_args(args, 1)) return core::Value::null();
    return core::Value::number(std::sin(args[0].as_number()));
}

/**
 * @brief Returns the cosine of a radian angle: `cos(x)`
 */
core::Value math_cos(std::span<const core::Value> args) {
    if (!has_numeric_args(args, 1)) return core::Value::null();
    return core::Value::number(std::cos(args[0].as_number()));
}

/**
 * @brief Returns the tangent of a radian angle: `tan(x)`
 */
core::Value math_tan(std::span<const core::Value> args) {
    if (!has_numeric_args(args, 1)) return core::Value::null();
    return core::Value::number(std::tan(args[0].as_number()));
}

/**
 * @brief Returns the arcsine (in radians) of a number: `asin(x)`
 */
core::Value math_asin(std::span<const core::Value> args) {
    if (!has_numeric_args(args, 1)) return core::Value::null();
    return core::Value::number(std::asin(args[0].as_number()));
}

/**
 * @brief Returns the arccosine (in radians) of a number: `acos(x)`
 */
core::Value math_acos(std::span<const core::Value> args) {
    if (!has_numeric_args(args, 1)) return core::Value::null();
    return core::Value::number(std::acos(args[0].as_number()));
}

/**
 * @brief Returns the arctangent (in radians) of a number: `atan(x)`
 */
core::Value math_atan(std::span<const core::Value> args) {
    if (!has_numeric_args(args, 1)) return core::Value::null();
    return core::Value::number(std::atan(args[0].as_number()));
}

/**
 * @brief Returns the four-quadrant arctangent: `atan2(y, x)`
 */
core::Value math_atan2(std::span<const core::Value> args) {
    if (!has_numeric_args(args, 2)) return core::Value::null();
    return core::Value::number(std::atan2(args[0].as_number(), args[1].as_number()));
}

/**
 * @brief Computes e raised to the given power: `exp(x)`
 */
core::Value math_exp(std::span<const core::Value> args) {
    if (!has_numeric_args(args, 1)) return core::Value::null();
    return core::Value::number(std::exp(args[0].as_number()));
}

/**
 * @brief Computes the natural logarithm: `log(x)`
 */
core::Value math_log(std::span<const core::Value> args) {
    if (!has_numeric_args(args, 1)) return core::Value::null();
    return core::Value::number(std::log(args[0].as_number()));
}

/**
 * @brief Computes the base-10 logarithm: `log10(x)`
 */
core::Value math_log10(std::span<const core::Value> args) {
    if (!has_numeric_args(args, 1)) return core::Value::null();
    return core::Value::number(std::log10(args[0].as_number()));
}

/**
 * @brief Computes the base-2 logarithm: `log2(x)`
 */
core::Value math_log2(std::span<const core::Value> args) {
    if (!has_numeric_args(args, 1)) return core::Value::null();
    return core::Value::number(std::log2(args[0].as_number()));
}

/**
 * @brief Computes the square root of the sum of squares: `hypot(x, y)`
 */
core::Value math_hypot(std::span<const core::Value> args) {
    if (!has_numeric_args(args, 2)) return core::Value::null();
    return core::Value::number(std::hypot(args[0].as_number(), args[1].as_number()));
}

/**
 * @brief Returns the minimum of two numbers: `min(a, b)`
 */
core::Value math_min(std::span<const core::Value> args) {
    if (!has_numeric_args(args, 2)) return core::Value::null();
    return core::Value::number(std::min(args[0].as_number(), args[1].as_number()));
}

/**
 * @brief Returns the maximum of two numbers: `max(a, b)`
 */
core::Value math_max(std::span<const core::Value> args) {
    if (!has_numeric_args(args, 2)) return core::Value::null();
    return core::Value::number(std::max(args[0].as_number(), args[1].as_number()));
}

} // namespace

NativeModule make_math_module() noexcept {
    auto m = NativeModule("math");

    // Mathematical constants
    m.add_const("PI", core::Value::number(std::numbers::pi));
    m.add_const("TAU", core::Value::number(2.0 * std::numbers::pi));
    m.add_const("E", core::Value::number(std::numbers::e));
    m.add_const("INFINITY", core::Value::number(std::numeric_limits<double>::infinity()));

    // Mathematical functions
    m.add_fn("sqrt", math_sqrt)
     .add_fn("pow", math_pow)
     .add_fn("abs", math_abs)
     .add_fn("floor", math_floor)
     .add_fn("ceil", math_ceil)
     .add_fn("round", math_round)
     .add_fn("trunc", math_trunc)
     .add_fn("sin", math_sin)
     .add_fn("cos", math_cos)
     .add_fn("tan", math_tan)
     .add_fn("asin", math_asin)
     .add_fn("acos", math_acos)
     .add_fn("atan", math_atan)
     .add_fn("atan2", math_atan2)
     .add_fn("exp", math_exp)
     .add_fn("log", math_log)
     .add_fn("log10", math_log10)
     .add_fn("log2", math_log2)
     .add_fn("hypot", math_hypot)
     .add_fn("min", math_min)
     .add_fn("max", math_max);

    return m;
}

} // namespace ely::stdlib
