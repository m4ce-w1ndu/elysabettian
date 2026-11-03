#include <ely/cli/repl.hpp>
#include <ely/cli/runner.hpp>
#include <ely/vm/vm.hpp>
#include <filesystem>
#include <iostream>
#include <optional>
#include <string_view>
#include <vector>

namespace {

void print_help(std::string_view program_name) {
    std::cout << "Elysabettian programming language runtime, JIT compiler, and REPL\n\n"
              << "Usage: " << program_name << " [OPTIONS] [SCRIPT]\n\n"
              << "Arguments:\n"
              << "  [SCRIPT]  Path to the .ely script to run\n\n"
              << "Options:\n"
              << "  -c, --command <CODE>  One-liner command script to execute\n"
              << "      --jit             Execute script directly via native JIT (bypassing VM interpreter)\n"
              << "      --no-jit          Disable dynamic JIT tier-up in VM interpreter\n"
              << "      --dump-ir         Print generated JIT intermediate representation (IR)\n"
              << "      --dump-asm        Print generated machine code disassembly (ASM)\n"
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

} // namespace

int main(int argc, char* argv[]) {
    std::vector<std::string_view> args;
    args.reserve(static_cast<size_t>(argc));
    for (int i = 0; i < argc; ++i) {
        args.emplace_back(argv[i]);
    }

    std::optional<std::string_view> command;
    std::optional<std::filesystem::path> script_path;
    bool jit_only = false;
    bool no_jit = false;
    bool dump_ir = false;
    bool dump_asm = false;

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
        if (arg == "--jit") {
            jit_only = true;
        } else if (arg == "--no-jit") {
            no_jit = true;
        } else if (arg == "--dump-ir") {
            dump_ir = true;
        } else if (arg == "--dump-asm") {
            dump_asm = true;
        } else if (arg == "-c" || arg == "--command") {
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
    if (no_jit) {
        vm.enable_jit(false);
    }
    if (dump_ir) {
        vm.set_dump_ir(true);
    }
    if (dump_asm) {
        vm.set_dump_asm(true);
    }

    if (command.has_value()) {
        const auto result = ely::cli::run_source(*command, vm, jit_only, dump_ir, dump_asm);
        return handle_result(result);
    }

    if (script_path.has_value()) {
        return ely::cli::run_file(*script_path, vm, jit_only, dump_ir, dump_asm);
    }

    ely::cli::run_repl(vm, jit_only, dump_ir, dump_asm);
    return 0;
}
