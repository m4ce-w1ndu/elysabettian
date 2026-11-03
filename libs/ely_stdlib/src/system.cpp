#include <ely/stdlib/system.hpp>
#include <ely/core/value.hpp>
#include <ely/stdlib/module.hpp>

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <span>
#include <string>
#include <string_view>
#include <thread>

namespace ely::stdlib {

namespace {

/**
 * @brief Returns the current timestamp in seconds since the UNIX epoch: `clock()` / `time()`
 */
core::Value system_clock([[maybe_unused]] std::span<const core::Value> args) {
    const auto now = std::chrono::system_clock::now().time_since_epoch();
    const double secs = std::chrono::duration<double>(now).count();
    return core::Value::number(secs);
}

/**
 * @brief Reads a line from standard input and parses it as a number if possible: `read()`
 */
core::Value system_read([[maybe_unused]] std::span<const core::Value> args) {
    std::cout.flush();
    std::string line;
    if (std::getline(std::cin, line)) {
        const auto first = line.find_first_not_of(" \t\r\n");
        if (first != std::string::npos) {
            const auto last = line.find_last_not_of(" \t\r\n");
            const std::string trimmed = line.substr(first, last - first + 1);
            char* end = nullptr;
            const double val = std::strtod(trimmed.c_str(), &end);
            if (end != trimmed.c_str() && *end == '\0') {
                return core::Value::number(val);
            }
        }
    }
    return core::Value::null();
}

/**
 * @brief Suspends current thread execution for the given duration in seconds: `sleep(seconds)`
 */
core::Value system_sleep(std::span<const core::Value> args) {
    if (!args.empty() && args[0].is_number()) {
        const double secs = args[0].as_number();
        if (secs > 0.0) {
            std::this_thread::sleep_for(std::chrono::duration<double>(secs));
        }
    }
    return core::Value::null();
}

/**
 * @brief Flushes standard output buffer: `flush()`
 */
core::Value system_flush([[maybe_unused]] std::span<const core::Value> args) {
    std::cout.flush();
    return core::Value::null();
}

void populate_system_functions(NativeModule& m) {
    m.add_fn("clock", system_clock)
     .add_fn("time", system_clock)
     .add_fn("read", system_read)
     .add_fn("sleep", system_sleep)
     .add_fn("flush", system_flush);
}

} // namespace

NativeModule make_system_module() noexcept {
    auto m = NativeModule("system");
    populate_system_functions(m);
    return m;
}

NativeModule make_io_module() noexcept {
    auto m = NativeModule("io");
    populate_system_functions(m);
    return m;
}

} // namespace ely::stdlib
