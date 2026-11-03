#include <ely/cli/repl.hpp>
#include <ely/cli/runner.hpp>
#include <iostream>
#include <string>
#include <string_view>

namespace {

[[nodiscard]] constexpr std::string_view trim(std::string_view sv) noexcept {
    const auto start = sv.find_first_not_of(" \t\n\r");
    if (start == std::string_view::npos) {
        return {};
    }
    const auto end = sv.find_last_not_of(" \t\n\r");
    return sv.substr(start, end - start + 1);
}

} // namespace

namespace ely::cli {

void run_repl(vm::VM& vm, bool jit_only, bool dump_ir, bool dump_asm) {
    const std::string_view mode = jit_only ? "JIT Native Mode" : "VM Tier-up Mode";
    std::cout << "Elysabettian REPL (v0.1.0) [" << mode << "]\n";
    std::cout << "Type 'exit' or press Ctrl+D to exit.\n\n";

    std::string line;
    while (true) {
        std::cout << "ely> " << std::flush;
        if (!std::getline(std::cin, line)) {
            std::cout << '\n';
            break;
        }

        const auto trimmed = trim(line);
        if (trimmed == "exit" || trimmed == "quit") {
            break;
        }

        if (trimmed.empty()) {
            continue;
        }

        static_cast<void>(run_source(line, vm, jit_only, dump_ir, dump_asm));
    }
}

} // namespace ely::cli
