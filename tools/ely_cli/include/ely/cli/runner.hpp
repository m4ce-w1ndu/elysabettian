#pragma once

#include <ely/vm/vm.hpp>
#include <filesystem>
#include <string_view>

namespace ely::cli {

/**
 * Executes Elysabettian source code, either directly via native JIT or via the VM.
 *
 * @param source Raw source code to execute.
 * @param vm Virtual machine maintaining state.
 * @param jit_only Execute directly via native JIT (bypassing VM interpreter).
 * @param dump_ir Print generated intermediate representation.
 * @param dump_asm Print generated machine code disassembly.
 * @return Execution outcome.
 */
vm::InterpretResult run_source(
    std::string_view source,
    vm::VM& vm,
    bool jit_only = false,
    bool dump_ir = false,
    bool dump_asm = false);

/**
 * Reads a script file from disk and executes it.
 *
 * @param path Path to the .ely script.
 * @param vm Virtual machine maintaining state.
 * @param jit_only Execute directly via native JIT.
 * @param dump_ir Print generated intermediate representation.
 * @param dump_asm Print generated machine code disassembly.
 * @return Process exit code (0 on success, 65 compile error, 70 runtime error, 74 I/O error).
 */
int run_file(
    const std::filesystem::path& path,
    vm::VM& vm,
    bool jit_only = false,
    bool dump_ir = false,
    bool dump_asm = false);

} // namespace ely::cli
