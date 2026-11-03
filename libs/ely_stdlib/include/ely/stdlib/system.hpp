#pragma once

#include <ely/stdlib/module.hpp>

namespace ely::stdlib {

/**
 * @brief Constructs and registers the standard 'system' native module.
 *
 * Provides system-level utilities including high-resolution time measurement,
 * thread sleeping, console flushing, and terminal input reading.
 *
 * @return NativeModule populated with system functions.
 */
[[nodiscard]] NativeModule make_system_module() noexcept;

/**
 * @brief Constructs and registers the 'io' native module (standard I/O alias).
 *
 * @return NativeModule populated with standard I/O functions.
 */
[[nodiscard]] NativeModule make_io_module() noexcept;

} // namespace ely::stdlib
