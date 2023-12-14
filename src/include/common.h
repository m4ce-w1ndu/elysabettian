#ifndef ELY_RT_COMMON_H
#define ELY_RT_COMMON_H

#include <iostream>
#include <vector>
#include <utility>
#include <fmt/core.h>
#include <fmt/printf.h>
#include <fmt/format.h>

#define DEBUG_PRINT_CODE
#define DEBUG_TRACE_EXECUTION

#undef DEBUG_PRINT_CODE
#undef DEBUG_TRACE_EXECUTION

constexpr auto UINT8_COUNT = UINT8_MAX + 1;

constexpr auto VERSION = "1.1.2";
constexpr auto VERSION_FULLNAME = "Elysabettian 1.1.2 Jupiter (JIT)";

#endif
