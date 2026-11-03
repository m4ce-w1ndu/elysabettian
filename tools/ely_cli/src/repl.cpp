#include <ely/cli/repl.hpp>
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

void run_repl(vm::VM& vm) {
    std::cout << "Elysabettian REPL (v0.1.0) [VM Interpreter Mode]\n";
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

        static_cast<void>(vm.interpret(line));
    }
}

} // namespace ely::cli
