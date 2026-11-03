#pragma once

#include <ely/core/chunk.hpp>
#include <ely/jit/engine.hpp>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

namespace ely::jit {

/**
 * Result of a JIT function compilation.
 */
struct JitCompileResult {
    const uint8_t* code_ptr{nullptr};
    std::string error{};

    [[nodiscard]] explicit operator bool() const noexcept {
        return code_ptr != nullptr;
    }
};

/**
 * Compiles Elysabettian bytecode chunks into native machine code using AsmJit.
 */
class JitCompiler {
public:
    explicit JitCompiler(JitEngine& engine) : m_engine(engine) {}

    /**
     * Compiles an arithmetic/expression chunk into a native `extern "C" double fn()` pointer.
     */
    [[nodiscard]] JitCompileResult compile_arithmetic(const core::Chunk& chunk, std::string_view name = "anon_arithmetic");

    /**
     * Compiles a function chunk with `arity` parameters and local variables into a native function.
     */
    [[nodiscard]] JitCompileResult compile_function(const core::Chunk& chunk, size_t arity, std::string_view name = "anon_fn");

    bool dump_asm{false};

private:
    JitEngine& m_engine;
};

} // namespace ely::jit
