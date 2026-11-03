#include <ely/jit/engine.hpp>

namespace ely::jit {

JitEngine::JitEngine() : m_runtime(std::make_unique<asmjit::JitRuntime>()) {}

JitEngine::~JitEngine() {
    if (m_runtime) {
        for (void* ptr : m_allocations) {
            if (ptr) {
                m_runtime->release(ptr);
            }
        }
    }
}

JitEngine::JitEngine(JitEngine&&) noexcept = default;
JitEngine& JitEngine::operator=(JitEngine&&) noexcept = default;

void JitEngine::track_allocation(void* code_ptr) {
    if (code_ptr) {
        m_allocations.push_back(code_ptr);
    }
}

} // namespace ely::jit
