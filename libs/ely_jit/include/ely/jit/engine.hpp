#pragma once

#include <asmjit/core.h>
#include <memory>
#include <vector>

namespace ely::jit {

/**
 * JIT execution engine managing executable memory allocations via AsmJit.
 */
class JitEngine {
public:
    JitEngine();
    ~JitEngine();

    JitEngine(const JitEngine&) = delete;
    JitEngine& operator=(const JitEngine&) = delete;

    JitEngine(JitEngine&&) noexcept;
    JitEngine& operator=(JitEngine&&) noexcept;

    [[nodiscard]] asmjit::JitRuntime& runtime() noexcept { return *m_runtime; }
    [[nodiscard]] const asmjit::JitRuntime& runtime() const noexcept { return *m_runtime; }

    /**
     * Registers an allocated executable memory pointer to be tracked and released upon engine destruction.
     */
    void track_allocation(void* code_ptr);

private:
    std::unique_ptr<asmjit::JitRuntime> m_runtime;
    std::vector<void*> m_allocations{};
};

} // namespace ely::jit
