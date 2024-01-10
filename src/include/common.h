#ifndef ELY_RT_COMMON_H
#define ELY_RT_COMMON_H

// Standard C++ headers.
#include <iostream>
#include <vector>
#include <utility>

// fmt headers.
#define FMT_HEADER_ONLY
#include <fmt/core.h>
#include <fmt/printf.h>
#include <fmt/format.h>

#define DEBUG_PRINT_CODE
#define DEBUG_TRACE_EXECUTION

#undef DEBUG_PRINT_CODE
#undef DEBUG_TRACE_EXECUTION

/**
 * @brief Max number of UINT8 variables.
*/
constexpr size_t UINT8_COUNT = UINT8_MAX + 1;

/**
 * @brief Current version of the language.
*/
constexpr char* VERSION = "1.1.3";

/**
 * @brief Full name of the language's version.
*/
constexpr char* VERSION_FULLNAME = "Elysabettian 1.1.3 Jupiter (JIT)";

#endif
