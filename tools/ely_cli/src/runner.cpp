#include <ely/cli/runner.hpp>
#include <ely/compiler/compiler.hpp>
#include <ely/jit/compiler.hpp>
#include <ely/jit/engine.hpp>
#include <ely/jit/runtime.hpp>
#include <atomic>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

namespace ely::cli {

namespace {

static std::atomic<size_t> s_jit_eval_counter{0};
static std::atomic<size_t> s_jit_func_counter{0};

bool prepare_jit_chunk(core::Chunk& chunk, jit::JitCompiler& compiler) {
    const size_t base_string_idx = jit::append_jit_strings(chunk.string_constants());

    std::vector<const uint8_t*> func_ptrs;
    func_ptrs.reserve(chunk.function_constants().size());

    for (size_t i = 0; i < chunk.function_constants().size(); ++i) {
        auto& f = chunk.function_constants()[i];
        if (f.chunk) {
            if (!prepare_jit_chunk(*f.chunk, compiler)) {
                return false;
            }
            const std::string fn_name = f.name.value_or("jit_fn_" + std::to_string(i));
            const auto res = compiler.compile_function(*f.chunk, f.arity, fn_name);
            if (!res) {
                std::cerr << "JIT Compilation Error in function '" << fn_name << "': " << res.error << "\n";
                return false;
            }
            func_ptrs.push_back(res.code_ptr);
        }
    }

    std::vector<size_t> func_ids;
    func_ids.reserve(func_ptrs.size());
    for (const auto* ptr : func_ptrs) {
        const size_t func_id = s_jit_func_counter.fetch_add(1, std::memory_order_seq_cst);
        jit::register_jit_function(func_id, ptr);
        func_ids.push_back(func_id);
    }

    // Patch chunk constants so that string constants and closures point to global JIT runtime IDs
    const auto& code = chunk.code();
    size_t ip = 0;
    while (ip < code.size()) {
        const auto op = static_cast<core::Opcode>(code[ip++]);
        switch (op) {
            case core::Opcode::Constant: {
                const auto const_idx = static_cast<size_t>(code[ip++]);
                if (const_idx < chunk.constants().size()) {
                    auto c = chunk.constants()[const_idx];
                    if (c.is_obj()) {
                        const size_t orig_idx = static_cast<size_t>(c.as_obj());
                        chunk.patch_constant(const_idx, core::Value::obj(base_string_idx + orig_idx));
                    }
                }
                break;
            }
            case core::Opcode::Closure: {
                const auto const_idx = static_cast<size_t>(code[ip++]);
                if (const_idx < chunk.constants().size()) {
                    auto c = chunk.constants()[const_idx];
                    if (c.is_obj()) {
                        const size_t orig_idx = static_cast<size_t>(c.as_obj());
                        if (orig_idx < func_ids.size()) {
                            chunk.patch_constant(const_idx, core::Value::obj(func_ids[orig_idx]));
                        }
                    }
                }
                break;
            }
            case core::Opcode::GetLocal:
            case core::Opcode::SetLocal:
            case core::Opcode::GetGlobal:
            case core::Opcode::DefineGlobal:
            case core::Opcode::SetGlobal:
            case core::Opcode::Call:
            case core::Opcode::Class:
            case core::Opcode::GetProperty:
            case core::Opcode::SetProperty:
            case core::Opcode::Method:
            case core::Opcode::Invoke:
            case core::Opcode::SuperInvoke:
            case core::Opcode::Import:
                ip += 1;
                break;
            case core::Opcode::Jump:
            case core::Opcode::JumpZero:
            case core::Opcode::Loop:
                ip += 2;
                break;
            default:
                break;
        }
    }
    return true;
}

vm::InterpretResult run_jit(
    const core::Chunk& chunk,
    jit::JitEngine& engine,
    bool dump_ir,
    bool dump_asm) {

    jit::JitCompiler compiler(engine);
    compiler.dump_ir = dump_ir;
    compiler.dump_asm = dump_asm;

    core::Chunk exec_chunk = chunk;
    if (!prepare_jit_chunk(exec_chunk, compiler)) {
        return vm::InterpretResult::CompileError;
    }

    const size_t eval_idx = s_jit_eval_counter.fetch_add(1, std::memory_order_seq_cst);
    const std::string eval_name = "jit_eval_" + std::to_string(eval_idx);
    const auto res = compiler.compile_arithmetic(exec_chunk, eval_name);
    if (!res) {
        std::cerr << "JIT Execution Error: " << res.error << "\n";
        return vm::InterpretResult::RuntimeError;
    }

    using Fn = double (*)();
    auto native_fn = reinterpret_cast<Fn>(const_cast<uint8_t*>(res.code_ptr));
    native_fn();
    return vm::InterpretResult::Ok;
}

} // namespace

vm::InterpretResult run_source(
    std::string_view source,
    vm::VM& vm,
    bool jit_only,
    bool dump_ir,
    bool dump_asm) {

    if (jit_only) {
        compiler::Compiler frontend(source);
        auto chunk_opt = frontend.compile();
        if (!chunk_opt.has_value()) {
            return vm::InterpretResult::CompileError;
        }
        auto* engine = vm.jit_engine();
        if (!engine) {
            std::cerr << "Error: JIT engine is not available.\n";
            return vm::InterpretResult::RuntimeError;
        }
        return run_jit(*chunk_opt, *engine, dump_ir, dump_asm);
    }

    return vm.interpret(source);
}

int run_file(
    const std::filesystem::path& path,
    vm::VM& vm,
    bool jit_only,
    bool dump_ir,
    bool dump_asm) {

    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "Error reading file '" << path.string() << "': failed to open file.\n";
        return 74;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    const auto result = run_source(buffer.str(), vm, jit_only, dump_ir, dump_asm);
    switch (result) {
        case vm::InterpretResult::Ok:
            return 0;
        case vm::InterpretResult::CompileError:
            return 65;
        case vm::InterpretResult::RuntimeError:
            return 70;
    }
    return 0;
}

} // namespace ely::cli
