#pragma once

#include "module.hpp"

namespace ely::stdlib {

/**
 * @brief Constructs and registers the standard 'math' native module.
 *
 * @return NativeModule populated with mathematical functions and constants.
 */
[[nodiscard]] NativeModule make_math_module() noexcept;

}
