#pragma once

#include "native.hpp"
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace ely::stdlib {

/**
 * Represents a named standard library module containing native functions and named constants.
 *
 * Provides a fluent builder interface (`add_fn`, `add_const`) to register native callables
 * and constant values before binding them into the VM runtime environment.
 */
class NativeModule {
public:
    /**
     * Constructs a new named native module.
     *
     * @param name Name identifier of the module (e.g. "math", "io").
     */
    explicit NativeModule(std::string name)
        : m_name(std::move(name)) {}

    /**
     * Registers a native function into the module.
     *
     * @param name Exported function identifier.
     * @param func Modern C++ callable receiving a span of Values.
     * @return Reference to this module for fluent chaining.
     */
    NativeModule& add_fn(std::string name, NativeFn func) {
        m_functions.emplace_back(std::move(name), std::move(func));
        return *this;
    }

    /**
     * Registers an exported constant Value into the module.
     *
     * @param name Exported constant identifier.
     * @param value NaN-boxed Value constant.
     * @return Reference to this module for fluent chaining.
     */
    NativeModule& add_const(std::string name, core::Value value) {
        m_constants.emplace_back(std::move(name), value);
        return *this;
    }

    /**
     * Returns the name of the module.
     */
    [[nodiscard]] std::string_view name() const noexcept { return m_name; }

    /**
     * Returns the list of registered native functions.
     */
    [[nodiscard]] const std::vector<std::pair<std::string, NativeFn>>& functions() const noexcept { return m_functions; }

    /**
     * Returns the list of registered constants.
     */
    [[nodiscard]] const std::vector<std::pair<std::string, core::Value>>& constants() const noexcept { return m_constants; }

private:
    std::string m_name;
    std::vector<std::pair<std::string, NativeFn>> m_functions{};
    std::vector<std::pair<std::string, core::Value>> m_constants{};
};

} // namespace ely::stdlib
