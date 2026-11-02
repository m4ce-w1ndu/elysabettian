#pragma once

#include <cstdint>
#include <ely/core/value.hpp>
#include <functional>
#include <span>
#include <utility>

namespace ely::stdlib {

/**
 * Modern C++20 callable signature for native standard library functions.
 *
 * Accepts a bounds-safe, zero-copy `std::span` over the VM stack arguments
 * and returns an IEEE-754 NaN-boxed `Value`.
 */
using NativeFn = std::function<core::Value(std::span<const core::Value> args)>;

extern "C" {
    /**
     * C-ABI compatible function pointer signature for foreign dynamically loaded functions (.so / .dll).
     *
     * Enables interoperability with external modules compiled with standard C linkage.
     */
    using ExternNativeFn = core::Value (*)(uint8_t argc, const core::Value* args);
}

/**
 * Unified callable representation for native functions in Elysabettian.
 *
 * Encapsulates both modern C++ callables (lambdas, functors, function pointers)
 * and foreign C-ABI functions into a single type-safe interface.
 */
class CallableNative {
public:
    /**
     * Constructs a CallableNative wrapping a modern C++ native function.
     *
     * @param native_fn Modern C++ callable taking a span of Value arguments.
     */
    explicit CallableNative(NativeFn native_fn) : m_callable(std::move(native_fn)) {}

    /**
     * Constructs a CallableNative adapting an external C-ABI function pointer.
     *
     * Automatically adapts the modern `std::span` arguments to the C-ABI `(argc, args)` convention.
     *
     * @param ext_native_fn Raw C-ABI function pointer from a shared library.
     */
    explicit CallableNative(ExternNativeFn ext_native_fn)
        : m_callable([ext_native_fn](std::span<const core::Value> args) -> core::Value {
            if (!ext_native_fn) return core::Value::null();
            return ext_native_fn(static_cast<uint8_t>(args.size()), args.data());
        }) {}

    /**
     * Default constructor creating an uninitialized, non-callable instance.
     */
    CallableNative() = default;

    /**
     * Invokes the wrapped native function with a bounds-safe span of arguments.
     *
     * @param args Span of evaluated argument Values from the VM stack.
     * @return Resulting Value from the native execution, or null if uninitialized.
     */
    [[nodiscard]] core::Value invoke(std::span<const core::Value> args) const {
        if (!m_callable) return core::Value::null();
        return m_callable(args);
    }

    /**
     * Checks if this CallableNative holds an active callable target.
     *
     * @return True if initialized with a valid function, false otherwise.
     */
    [[nodiscard]] explicit operator bool() const noexcept {
        return static_cast<bool>(m_callable);
    }

private:
    NativeFn m_callable;
};

} // namespace ely::stdlib
