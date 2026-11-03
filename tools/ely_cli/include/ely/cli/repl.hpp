#pragma once

#include <ely/vm/vm.hpp>

namespace ely::cli {

/**
 * @brief Runs the interactive Read-Eval-Print Loop (REPL).
 *
 * @param vm Virtual machine instance maintaining runtime state across evaluations.
 * @param jit_only Whether to compile and execute directly via native JIT.
 * @param dump_ir Whether to print intermediate representation during compilation.
 * @param dump_asm Whether to print machine code disassembly during compilation.
 */
void run_repl(vm::VM& vm, bool jit_only = false, bool dump_ir = false, bool dump_asm = false);

} // namespace ely::cli
