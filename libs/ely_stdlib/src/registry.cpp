#include "ely/core/value.hpp"
#include "ely/stdlib/math.hpp"
#include "ely/stdlib/native.hpp"
#include "ely/stdlib/system.hpp"
#include <cstdint>
#include <ely/stdlib/registry.hpp>

#include <dlfcn.h>
#include <unordered_map>

namespace ely::stdlib {

/**
 * @brief Hases a string using the FNV-1a algorithm.
 *
 * @param str String view to the string to hash.
 * @return Hash value of the given string.
 */
static constexpr uint32_t hash_string(std::string_view str) noexcept {
    uint32_t hash = 2166136261u;
    for (char c : str) {
        hash ^= static_cast<uint8_t>(c);
        hash *= 16777619u;
    }
    return hash;
}

ModuleRegistry::ModuleRegistry()
    : m_builtins({
        { "math", make_math_module() },
        { "sys", make_system_module() },
        { "system", make_system_module() },
        { "io", make_io_module() },
        { "stdio", make_io_module() }
    }),
    m_search_paths({ "./modules", "/usr/local/lib/ely" }) {}

ModuleRegistry::~ModuleRegistry() {
    for (const auto handle : m_loaded_handles) {
        (void)dlclose(handle);
    }
}

bool ModuleRegistry::load(std::string_view name,
                          std::unordered_map<uint32_t, CallableNative> &natives,
                          std::unordered_map<uint32_t, core::Value> &globals) {

    const std::string as_string(name);

    // Check if the module already exists as built-in
    if (auto it = m_builtins.find(as_string); it != m_builtins.end()) {
        for (const auto& fn : it->second.functions()) {
            const auto id = hash_string(fn.first);
            natives.insert_or_assign(id, CallableNative(fn.second));
            globals.insert_or_assign(id, core::Value::obj(id));
        }

        for (const auto& c : it->second.constants()) {
            const auto id = hash_string(c.first);
            globals.insert_or_assign(id, c.second);
        }

        return true;
    }

    // Get candidate names
    const auto candidate_names = {
        std::format("lib{}.so", name),
        std::format("{}.so", name),
        std::format("lib{}.dylib", name),
        std::format("{}.dylib", name)
    };

    // Build full candidate paths
    for (const auto& c_path : m_search_paths) {
        for (const auto& c_name : candidate_names) {
            const auto full_path = c_path / c_name;
            if (std::filesystem::exists(full_path)) {
                if (load_external_so(full_path, natives, globals)) return true;
            }
        }
    }

    return false;
}

bool ModuleRegistry::load_external_so(const std::filesystem::path &path, std::unordered_map<uint32_t, CallableNative> &natives, std::unordered_map<uint32_t, core::Value> &globals) {
    // Open library and get its handle
    void* handle = dlopen(path.c_str(), RTLD_NOW | RTLD_LOCAL);
    if (!handle) return false;

    // Recover its initialisation function
    using InitFn = int (*)(ElyApi*);
    auto init_fn = reinterpret_cast<InitFn>(dlsym(handle, "ely_init_module"));

    // Check init fn validity
    if (!init_fn) {
        (void)dlclose(handle);
        return false;
    }

    // Create a context
    struct LoadContext {
        std::unordered_map<uint32_t, CallableNative>& natives;
        std::unordered_map<uint32_t, core::Value>& globals;
    } ctx {natives, globals };

    ElyApi api{
        .ctx = &ctx,
        .register_fn = [](void* context, const char* name, ExternNativeFn func) {
            if (!context || !name || !func) return;
            auto* load_ctx = static_cast<LoadContext*>(context);
            const auto id = hash_string(name);
            load_ctx->natives.insert_or_assign(id, CallableNative(func));
            load_ctx->globals.insert_or_assign(id, core::Value::obj(id));
        },
        .register_number = [](void* context, const char* name, double value) {
            if (!context || !name) return;
            auto* load_ctx = static_cast<LoadContext*>(context);
            const auto id = hash_string(name);
            load_ctx->globals.insert_or_assign(id, core::Value::number(value));
        }
    };

    // Initialise the module
    if (init_fn(&api) == 0) {
        m_loaded_handles.push_back(handle);
        return true;
    }

    (void)dlclose(handle);
    return false;
}

void ModuleRegistry::register_builtin(NativeModule module) {
    m_builtins.insert_or_assign(std::string(module.name()), std::move(module));
}

void ModuleRegistry::add_search_path(std::filesystem::path path) {
    m_search_paths.push_back(std::move(path));
}

}
