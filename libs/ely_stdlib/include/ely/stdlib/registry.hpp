#pragma once

#include <ely/core/value.hpp>
#include <ely/stdlib/module.hpp>
#include <ely/stdlib/native.hpp>

#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace ely::stdlib {

/**
 * @brief C-compatible API passed to foreign shared libraries upon module initialisation.
 */
extern "C" struct ElyApi {
    void* ctx{nullptr};
    void (*register_fn)(void* ctx, const char* name, ExternNativeFn func){nullptr};
    void (*register_number)(void* ctx, const char* name, double val){nullptr};
};

/**
 * @brief Manages built-in standard modules and dynamically loaded shared libraries.
 */
class ModuleRegistry {
public:
    ModuleRegistry();
    ~ModuleRegistry();

    // Disable all copy constructors
    ModuleRegistry(const ModuleRegistry&) = delete;
    ModuleRegistry& operator=(const ModuleRegistry&) = delete;

    // Enable move
    ModuleRegistry(ModuleRegistry&&) noexcept = default;
    ModuleRegistry& operator=(ModuleRegistry&&) noexcept = default;

    /**
     * @brief Loads a module by name (either built-in or from an external shared library).
     *
     * @param name Module name (e.g. "math", "system", "io").
     * @param natives Reference to the VM's native function registry.
     * @param globals Reference to the VM's global variable registry.
     * @return True if the module was found and loaded successfully, false otherwise.
     */
    [[nodiscard]] bool load(
        std::string_view name,
        std::unordered_map<uint32_t, CallableNative>& natives,
        std::unordered_map<uint32_t, core::Value>& globals
    );

    /**
     * @brief Registers a built-in module into the VM call context.
     *
     * @param module NativeModule containing a reference to the module to register.
     */
    void register_builtin(NativeModule module);

    /**
     * @brief Adds a search path for shared libraries used to extend functionalities.
     *
     * @param path std::filesystem::path containing the path to the candidate directory.
     */
    void add_search_path(std::filesystem::path path);

private:
    /**
     * @brief Loads an external shared library.
     *
     * @param path Path to the external shared library file.
     * @param natives Native functions to register and load.
     * @param globals Global values to register and load.
     */
    [[nodiscard]] bool load_external_so(
        const std::filesystem::path& path,
        std::unordered_map<uint32_t, CallableNative>& natives,
        std::unordered_map<uint32_t, core::Value>& globals
    );

    std::unordered_map<std::string, NativeModule> m_builtins{};
    std::vector<std::filesystem::path> m_search_paths{};
    std::vector<void*> m_loaded_handles{};
};

}
