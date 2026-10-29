#include "ely/core/opcode.hpp"
#include <algorithm>
#include <cstdint>
#include <ely/compiler/compiler.hpp>
#include <format>
#include <optional>

namespace ely::compiler {

void Compiler::begin_scope() noexcept {
    m_context.begin_scope();
}

void Compiler::end_scope() noexcept {
    const auto to_pop = m_context.pop_scope();
    for (size_t i = 0; i < to_pop; i++) {
        emit_opcode(core::Opcode::Pop);
    }
}

void Compiler::add_local(Token name) {
    if (m_context.locals().size() >= MAX_LOCALS) {
        m_parser.error(std::format(
            "Number of local variables ({}) exceeds max: {}",
            m_context.locals().size(),
            MAX_LOCALS)
        );
    }

    m_context.locals().push_back(Local {
        std::move(name),
        -1,
        false
    });
}

void Compiler::declare_variable() {
  if (m_context.scope_depth() == 0) return;

  const auto name = m_parser.previous();

  for (auto it = m_context.locals().rbegin(); it != m_context.locals().rend(); it++) {
      if (it->depth != -1 && it->depth < static_cast<int32_t>(m_context.scope_depth())) break;

      if (name.lexeme == it->name.lexeme) {
          m_parser.error("There is already a variable with this name declared in this scope.");
          return;
      }
  }

  add_local(std::move(name));
}

void Compiler::mark_initialized() noexcept {
    if (m_context.scope_depth() == 0) return;
    auto& var = m_context.locals().back();
    var.depth = static_cast<int32_t>(m_context.scope_depth());
}

[[nodiscard]] std::optional<uint8_t> Compiler::resolve_local(Token name) {
    for (auto i = m_context.locals().size(); i-- > 0;) {
        auto& var = m_context.locals()[i];

        if (name.lexeme == var.name.lexeme) {
            if (var.depth == -1) {
                m_parser.error("Cannot read local variable in its own initializer.");
                return std::nullopt;
            }

            return static_cast<uint8_t>(i);
        }
    }

    return std::nullopt;
}

}
