#include <ely/jit/runtime.hpp>
#include <format>
#include <iostream>
#include <unordered_map>
#include <vector>

namespace {

struct JitRuntimeState {
    std::unordered_map<uint32_t, ely::core::Value> globals{};
    std::vector<std::string> strings{};
    std::unordered_map<size_t, const void*> functions{};
};

thread_local JitRuntimeState g_jit_state{};

} // namespace

namespace ely::jit {

void register_jit_function(size_t func_idx, const void* ptr) {
    g_jit_state.functions[func_idx] = ptr;
}

const void* get_jit_function(size_t func_idx) {
    const auto it = g_jit_state.functions.find(func_idx);
    return (it != g_jit_state.functions.end()) ? it->second : nullptr;
}

void register_jit_strings(std::span<const std::string> strings) {
    g_jit_state.strings.assign(strings.begin(), strings.end());
}

size_t append_jit_strings(std::span<const std::string> strings) {
    const size_t base = g_jit_state.strings.size();
    g_jit_state.strings.insert(g_jit_state.strings.end(), strings.begin(), strings.end());
    return base;
}

void reset_jit_runtime() {
    g_jit_state.globals.clear();
    g_jit_state.strings.clear();
    g_jit_state.functions.clear();
}

} // namespace ely::jit

extern "C" {

void ely_jit_set_global(uint32_t id, uint64_t val_bits) {
    g_jit_state.globals[id] = ely::core::Value::from_raw(val_bits);
}

uint64_t ely_jit_get_global(uint32_t id) {
    const auto it = g_jit_state.globals.find(id);
    if (it != g_jit_state.globals.end()) {
        return it->second.raw();
    }
    return ely::core::Value::null().raw();
}

void ely_jit_print_value(uint64_t val_bits) {
    const auto value = ely::core::Value::from_raw(val_bits);
    if (value.is_number()) {
        std::cout << std::format("{}", value.as_number()) << '\n';
    } else if (value.is_null()) {
        std::cout << "null\n";
    } else if (value.is_boolean()) {
        std::cout << (value.as_boolean() ? "true\n" : "false\n");
    } else if (value.is_obj()) {
        const auto str_idx = static_cast<size_t>(value.as_obj());
        if (str_idx < g_jit_state.strings.size()) {
            std::cout << g_jit_state.strings[str_idx] << '\n';
        } else {
            std::cout << "<object " << str_idx << ">\n";
        }
    } else {
        std::cout << "<unknown " << val_bits << ">\n";
    }
}

double ely_jit_call_0(uint64_t callee_bits) {
    const auto callee = ely::core::Value::from_raw(callee_bits);
    const auto func_idx = static_cast<size_t>(callee.as_obj());
    const void* ptr = ely::jit::get_jit_function(func_idx);
    if (ptr) {
        using Fn = double (*)();
        auto fn = reinterpret_cast<Fn>(const_cast<void*>(ptr));
        return fn();
    }
    return 0.0;
}

double ely_jit_call_1(uint64_t callee_bits, double arg0) {
    const auto callee = ely::core::Value::from_raw(callee_bits);
    const auto func_idx = static_cast<size_t>(callee.as_obj());
    const void* ptr = ely::jit::get_jit_function(func_idx);
    if (ptr) {
        using Fn = double (*)(double);
        auto fn = reinterpret_cast<Fn>(const_cast<void*>(ptr));
        return fn(arg0);
    }
    return 0.0;
}

double ely_jit_call_2(uint64_t callee_bits, double arg0, double arg1) {
    const auto callee = ely::core::Value::from_raw(callee_bits);
    const auto func_idx = static_cast<size_t>(callee.as_obj());
    const void* ptr = ely::jit::get_jit_function(func_idx);
    if (ptr) {
        using Fn = double (*)(double, double);
        auto fn = reinterpret_cast<Fn>(const_cast<void*>(ptr));
        return fn(arg0, arg1);
    }
    return 0.0;
}

}
