#pragma once

#include <ely/vm/vm.hpp>

namespace ely::cli {

    /**
     * @brief Runs the interactive Read-Eval-Print Loop (REPL).
     *
     * @param vm Virtual machine instance maintaining runtime state across evaluations.
     */
void run_repl(vm::VM& vm);

}
