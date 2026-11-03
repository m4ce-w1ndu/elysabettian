#include <ely/cli/repl.hpp>
#include <ely/vm/vm.hpp>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace {

void print_help(std::string_view program_name) {
    std::cout << "Elysabettian programming language runtime and REPL\n\n"
              << "Usage: " << program_name << " [OPTIONS] [SCRIPT]\n\n"
              << "Arguments:\n"
              << "  [SCRIPT]  Path to the .ely script to run\n\n"
              << "Options:\n"
              << "  -c, --command <CODE>  One-liner command script to execute\n"
              << "  -h, --help            Print help\n"
              << "  -v, --version         Print version\n";
}

void print_version() {
    std::cout << "ely 0.1.0\n";
}

int handle_result(ely::vm::InterpretResult result) {
    switch (result) {
        case ely::vm::InterpretResult::Ok:
            return 0;
        case ely::vm::InterpretResult::CompileError:
            return 65;
        case ely::vm::InterpretResult::RuntimeError:
            return 70;
    }
    return 0;
}

int run_file(ely::vm::VM& vm, const std::filesystem::path& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "Error reading file '" << path.string() << "': failed to open file.\n";
        return 74;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    const auto result = vm.interpret(buffer.str());
    return handle_result(result);
}

} // namespace

int main(int argc, char* argv[]) {
    std::vector<std::string_view> args;
    args.reserve(static_cast<size_t>(argc));
    for (int i = 0; i < argc; ++i) {
        args.emplace_back(argv[i]);
    }

    std::optional<std::string_view> command;
    std::optional<std::filesystem::path> script_path;

    for (size_t i = 1; i < args.size(); ++i) {
        const auto arg = args[i];
        if (arg == "-h" || arg == "--help") {
            print_help(args[0]);
            return 0;
        }
        if (arg == "-v" || arg == "--version") {
            print_version();
            return 0;
        }
        if (arg == "-c" || arg == "--command") {
            if (i + 1 >= args.size()) {
                std::cerr << "Error: flag '" << arg << "' requires an argument.\n";
                return 64;
            }
            command = args[++i];
        } else if (arg.starts_with("-")) {
            std::cerr << "Error: unexpected option '" << arg << "'.\n";
            print_help(args[0]);
            return 64;
        } else {
            if (script_path.has_value()) {
                std::cerr << "Error: multiple script files provided.\n";
                return 64;
            }
            script_path = std::filesystem::path(arg);
        }
    }

    ely::vm::VM vm;

    if (command.has_value()) {
        const auto result = vm.interpret(*command);
        return handle_result(result);
    }

    if (script_path.has_value()) {
        return run_file(vm, *script_path);
    }

    ely::cli::run_repl(vm);
    return 0;
}
