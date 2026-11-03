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
 * Compiles Elysabettian bytecode chunks into native machine code.
 *
 * Provides cross-architecture JIT compilation supporting both x86-64 and
 * ARM64 (AArch64 for Linux and macOS Apple Silicon).
 */
class JitCompiler {
public:
    explicit JitCompiler(JitEngine& engine) : m_engine(engine) {}

    /**
     * Compiles an arithmetic/expression chunk into native code for the host CPU.
     */
    [[nodiscard]] JitCompileResult compile_arithmetic(const core::Chunk& chunk, std::string_view name = "anon_arithmetic");

    /**
     * Compiles a function chunk with `arity` parameters for the host CPU.
     */
    [[nodiscard]] JitCompileResult compile_function(const core::Chunk& chunk, size_t arity, std::string_view name = "anon_fn");

    /**
     * Explicit x86-64 code generation backend.
     */
    [[nodiscard]] JitCompileResult compile_function_x86(const core::Chunk& chunk, size_t arity, std::string_view name = "x86_fn");

    /**
     * Explicit ARM64 (AArch64) code generation backend (Linux and macOS Apple Silicon).
     */
    [[nodiscard]] JitCompileResult compile_function_a64(const core::Chunk& chunk, size_t arity, std::string_view name = "a64_fn");

    bool dump_ir{false};
    bool dump_asm{false};

private:
    JitEngine& m_engine;
};

} // namespace ely::jit
