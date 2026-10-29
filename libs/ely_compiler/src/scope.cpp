#include <ely/compiler/scope.hpp>

namespace ely::compiler {

size_t CompilerContext::pop_scope() noexcept {
    m_scope_depth--;
    size_t popped = 0;

    while (!m_locals.empty() && m_locals.back().depth > static_cast<int32_t>(m_scope_depth)) {
        m_locals.pop_back();
        popped++;
    }

    return popped;
}

}
