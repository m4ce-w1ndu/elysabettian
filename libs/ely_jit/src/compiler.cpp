#include <ely/jit/compiler.hpp>

namespace ely::jit {

JitCompileResult JitCompiler::compile_arithmetic(const core::Chunk& chunk, std::string_view name) {
    return compile_function(chunk, 0, name);
}

JitCompileResult JitCompiler::compile_function(const core::Chunk& chunk, size_t arity, std::string_view name) {
#if defined(__aarch64__) || defined(_M_ARM64)
    return compile_function_a64(chunk, arity, name);
#elif defined(__x86_64__) || defined(_M_X64)
    return compile_function_x86(chunk, arity, name);
#else
    return {nullptr, "JIT compilation is not supported on this CPU architecture."};
#endif
}

} // namespace ely::jit
