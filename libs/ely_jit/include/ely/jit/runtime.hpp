#pragma once

#include <ely/core/value.hpp>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>

namespace ely::jit {

/**
 * Registers a compiled JIT function entry point for dynamic calls.
 */
void register_jit_function(size_t func_idx, const void* ptr);

/**
 * Retrieves a registered JIT function entry point.
 */
[[nodiscard]] const void* get_jit_function(size_t func_idx);

/**
 * Registers string constants for the active JIT execution context.
 */
void register_jit_strings(std::span<const std::string> strings);

/**
 * Appends string constants and returns the base offset.
 */
size_t append_jit_strings(std::span<const std::string> strings);

/**
 * Clears the JIT runtime state (globals, strings, function handles, natives).
 */
void reset_jit_runtime();

} // namespace ely::jit

extern "C" {

/**
 * Sets a global variable in the JIT runtime environment.
 */
void ely_jit_set_global(uint32_t id, uint64_t val_bits);

/**
 * Retrieves a global variable value from the JIT runtime environment.
 */
uint64_t ely_jit_get_global(uint32_t id);

/**
 * Formats and prints a NaN-boxed Value to standard output.
 */
void ely_jit_print_value(uint64_t val_bits);

/**
 * Dynamic call trampoline for 0-argument JIT functions.
 */
double ely_jit_call_0(uint64_t callee_bits);

/**
 * Dynamic call trampoline for 1-argument JIT functions.
 */
double ely_jit_call_1(uint64_t callee_bits, double arg0);

/**
 * Dynamic call trampoline for 2-argument JIT functions.
 */
double ely_jit_call_2(uint64_t callee_bits, double arg0, double arg1);

/**
 * Dynamically loads and binds a module into JIT runtime globals and native tables.
 */
void ely_jit_import_module(uint32_t string_idx);

}
