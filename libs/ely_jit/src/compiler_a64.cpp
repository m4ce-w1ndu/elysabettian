#include <ely/jit/compiler.hpp>
#include <ely/jit/runtime.hpp>
#include <ely/core/opcode.hpp>
#include <ely/core/value.hpp>
#include <asmjit/a64.h>
#include <iostream>
#include <set>
#include <unordered_map>
#include <vector>

namespace ely::jit {

namespace {

uint32_t hash_string(std::string_view str) noexcept {
    uint32_t hash = 2166136261u;
    for (char c : str) {
        hash ^= static_cast<uint8_t>(c);
        hash *= 16777619u;
    }
    return hash;
}

uint32_t get_name_id(const core::Chunk& chunk, uint8_t byte_val) noexcept {
    if (byte_val < chunk.string_constants().size()) {
        return hash_string(chunk.string_constants()[byte_val]);
    }
    if (byte_val < chunk.constants().size() && chunk.constants()[byte_val].is_obj()) {
        return static_cast<uint32_t>(chunk.constants()[byte_val].as_obj());
    }
    return static_cast<uint32_t>(byte_val);
}

} // namespace

JitCompileResult JitCompiler::compile_function_a64(const core::Chunk& chunk, size_t arity, std::string_view name) {
    asmjit::Environment env = m_engine.runtime().environment();
    asmjit::CpuFeatures cpu_features = m_engine.runtime().cpu_features();
    if (env.arch() != asmjit::Arch::kAArch64) {
        env.init(asmjit::Arch::kAArch64);
        cpu_features.reset();
        cpu_features.add(asmjit::CpuFeatures::ARM::kASIMD);
    }

    asmjit::CodeHolder code_holder;
    code_holder.init(env, cpu_features);

    asmjit::a64::Compiler cc(&code_holder);

    // Build signature: arity double arguments, returning double
    asmjit::FuncSignature sig(asmjit::CallConvId::kCDecl);
    sig.set_ret_t<double>();
    for (size_t i = 0; i < arity; ++i) {
        sig.add_arg_t<double>();
    }

    asmjit::FuncNode* func_node = cc.add_func(sig);

    // Pre-pass: Scan chunk for branch targets and maximum local slot
    std::set<size_t> branch_targets;
    branch_targets.insert(0);

    size_t max_local = arity;
    size_t scan_ip = 0;
    const auto& code = chunk.code();

    while (scan_ip < code.size()) {
        const auto op = static_cast<core::Opcode>(code[scan_ip++]);
        switch (op) {
            case core::Opcode::GetLocal:
            case core::Opcode::SetLocal: {
                if (scan_ip < code.size()) {
                    const auto slot = static_cast<size_t>(code[scan_ip++]);
                    if (slot > max_local) {
                        max_local = slot;
                    }
                }
                break;
            }
            case core::Opcode::Constant:
            case core::Opcode::DefineGlobal:
            case core::Opcode::GetGlobal:
            case core::Opcode::SetGlobal:
            case core::Opcode::Closure:
            case core::Opcode::Import:
            case core::Opcode::Call: {
                scan_ip += 1;
                break;
            }
            case core::Opcode::Jump:
            case core::Opcode::JumpZero: {
                if (scan_ip + 1 < code.size()) {
                    const auto high = static_cast<uint16_t>(code[scan_ip]);
                    const auto low = static_cast<uint16_t>(code[scan_ip + 1]);
                    const uint16_t offset = static_cast<uint16_t>((high << 8) | low);
                    scan_ip += 2;
                    const size_t target = scan_ip + offset;
                    branch_targets.insert(target);
                }
                break;
            }
            case core::Opcode::Loop: {
                if (scan_ip + 1 < code.size()) {
                    const auto high = static_cast<uint16_t>(code[scan_ip]);
                    const auto low = static_cast<uint16_t>(code[scan_ip + 1]);
                    const uint16_t offset = static_cast<uint16_t>((high << 8) | low);
                    scan_ip += 2;
                    const size_t target = scan_ip - offset;
                    branch_targets.insert(target);
                }
                break;
            }
            default:
                break;
        }
    }

    // Allocate stack slots for all local variables
    std::vector<asmjit::a64::Mem> vars;
    vars.reserve(max_local + 1);
    for (size_t i = 0; i <= max_local; ++i) {
        vars.push_back(cc.new_stack(8, 8));
    }

    // Bind function argument registers and store into stack slots
    for (size_t i = 0; i < arity; ++i) {
        asmjit::a64::Vec arg_reg = cc.new_vec_d();
        func_node->set_arg(i, arg_reg);
        cc.str(arg_reg, vars[i]);
    }

    // Pre-create AsmJit labels for all branch targets
    std::unordered_map<size_t, asmjit::Label> labels;
    for (size_t target : branch_targets) {
        labels[target] = cc.new_label();
    }

    // Simulated evaluation stack of virtual double registers
    std::vector<asmjit::a64::Vec> eval_stack;
    bool current_terminated = false;

    size_t ip = 0;
    while (ip < code.size()) {
        auto label_it = labels.find(ip);
        if (label_it != labels.end()) {
            cc.bind(label_it->second);
            current_terminated = false;
        }

        const auto op = static_cast<core::Opcode>(code[ip++]);

        if (current_terminated) {
            switch (op) {
                case core::Opcode::Constant:
                case core::Opcode::GetLocal:
                case core::Opcode::SetLocal:
                case core::Opcode::DefineGlobal:
                case core::Opcode::GetGlobal:
                case core::Opcode::SetGlobal:
                case core::Opcode::Closure:
                case core::Opcode::Import:
                case core::Opcode::Call:
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
            continue;
        }

        switch (op) {
            case core::Opcode::Constant: {
                const auto const_idx = static_cast<size_t>(code[ip++]);
                const auto const_val = chunk.constants()[const_idx];
                asmjit::a64::Vec reg = cc.new_vec_d();
                if (const_val.is_number()) {
                    asmjit::a64::Mem c_mem = cc.new_double_const(asmjit::ConstPoolScope::kLocal, const_val.as_number());
                    cc.ldr(reg, c_mem);
                } else {
                    asmjit::a64::Gp gpq = cc.new_gp64();
                    cc.mov(gpq, asmjit::Imm(static_cast<int64_t>(const_val.raw())));
                    cc.fmov(reg, gpq);
                }
                eval_stack.push_back(reg);
                break;
            }

            case core::Opcode::True: {
                asmjit::a64::Gp gpq = cc.new_gp64();
                cc.mov(gpq, asmjit::Imm(static_cast<int64_t>(core::Value::boolean(true).raw())));
                asmjit::a64::Vec reg = cc.new_vec_d();
                cc.fmov(reg, gpq);
                eval_stack.push_back(reg);
                break;
            }

            case core::Opcode::False: {
                asmjit::a64::Gp gpq = cc.new_gp64();
                cc.mov(gpq, asmjit::Imm(static_cast<int64_t>(core::Value::boolean(false).raw())));
                asmjit::a64::Vec reg = cc.new_vec_d();
                cc.fmov(reg, gpq);
                eval_stack.push_back(reg);
                break;
            }

            case core::Opcode::Closure: {
                const auto const_idx = static_cast<size_t>(code[ip++]);
                const auto const_val = chunk.constants()[const_idx];
                asmjit::a64::Gp gpq = cc.new_gp64();
                cc.mov(gpq, asmjit::Imm(static_cast<int64_t>(const_val.raw())));
                asmjit::a64::Vec reg = cc.new_vec_d();
                cc.fmov(reg, gpq);
                eval_stack.push_back(reg);
                break;
            }

            case core::Opcode::GetLocal: {
                const auto slot = static_cast<size_t>(code[ip++]);
                asmjit::a64::Vec reg = cc.new_vec_d();
                cc.ldr(reg, vars[slot]);
                eval_stack.push_back(reg);
                break;
            }

            case core::Opcode::SetLocal: {
                const auto slot = static_cast<size_t>(code[ip++]);
                if (eval_stack.empty()) return {nullptr, "Stack underflow in SetLocal"};
                asmjit::a64::Vec val = eval_stack.back();
                cc.str(val, vars[slot]);
                break;
            }

            case core::Opcode::DefineGlobal: {
                const auto const_idx = code[ip++];
                const uint32_t name_id = get_name_id(chunk, const_idx);
                if (eval_stack.empty()) return {nullptr, "Stack underflow in DefineGlobal"};
                asmjit::a64::Vec val = eval_stack.back();
                eval_stack.pop_back();
                asmjit::a64::Gp bits = cc.new_gp64();
                cc.fmov(bits, val);
                asmjit::InvokeNode* invoke = nullptr;
                cc.invoke(asmjit::Out(invoke), reinterpret_cast<uint64_t>(&ely_jit_set_global), asmjit::FuncSignature::build<void, uint32_t, uint64_t>());
                invoke->set_arg(0, asmjit::Imm(name_id));
                invoke->set_arg(1, bits);
                break;
            }

            case core::Opcode::GetGlobal: {
                const auto const_idx = code[ip++];
                const uint32_t name_id = get_name_id(chunk, const_idx);
                asmjit::a64::Gp bits = cc.new_gp64();
                asmjit::InvokeNode* invoke = nullptr;
                cc.invoke(asmjit::Out(invoke), reinterpret_cast<uint64_t>(&ely_jit_get_global), asmjit::FuncSignature::build<uint64_t, uint32_t>());
                invoke->set_arg(0, asmjit::Imm(name_id));
                invoke->set_ret(0, bits);
                asmjit::a64::Vec res = cc.new_vec_d();
                cc.fmov(res, bits);
                eval_stack.push_back(res);
                break;
            }

            case core::Opcode::SetGlobal: {
                const auto const_idx = code[ip++];
                const uint32_t name_id = get_name_id(chunk, const_idx);
                if (eval_stack.empty()) return {nullptr, "Stack underflow in SetGlobal"};
                asmjit::a64::Vec val = eval_stack.back();
                asmjit::a64::Gp bits = cc.new_gp64();
                cc.fmov(bits, val);
                asmjit::InvokeNode* invoke = nullptr;
                cc.invoke(asmjit::Out(invoke), reinterpret_cast<uint64_t>(&ely_jit_set_global), asmjit::FuncSignature::build<void, uint32_t, uint64_t>());
                invoke->set_arg(0, asmjit::Imm(name_id));
                invoke->set_arg(1, bits);
                break;
            }

            case core::Opcode::Add: {
                if (eval_stack.size() < 2) return {nullptr, "Stack underflow in Add"};
                auto b = eval_stack.back(); eval_stack.pop_back();
                auto a = eval_stack.back(); eval_stack.pop_back();
                auto res = cc.new_vec_d();
                cc.fadd(res, a, b);
                eval_stack.push_back(res);
                break;
            }

            case core::Opcode::Subtract: {
                if (eval_stack.size() < 2) return {nullptr, "Stack underflow in Subtract"};
                auto b = eval_stack.back(); eval_stack.pop_back();
                auto a = eval_stack.back(); eval_stack.pop_back();
                auto res = cc.new_vec_d();
                cc.fsub(res, a, b);
                eval_stack.push_back(res);
                break;
            }

            case core::Opcode::Multiply: {
                if (eval_stack.size() < 2) return {nullptr, "Stack underflow in Multiply"};
                auto b = eval_stack.back(); eval_stack.pop_back();
                auto a = eval_stack.back(); eval_stack.pop_back();
                auto res = cc.new_vec_d();
                cc.fmul(res, a, b);
                eval_stack.push_back(res);
                break;
            }

            case core::Opcode::Divide: {
                if (eval_stack.size() < 2) return {nullptr, "Stack underflow in Divide"};
                auto b = eval_stack.back(); eval_stack.pop_back();
                auto a = eval_stack.back(); eval_stack.pop_back();
                auto res = cc.new_vec_d();
                cc.fdiv(res, a, b);
                eval_stack.push_back(res);
                break;
            }

            case core::Opcode::Negate: {
                if (eval_stack.empty()) return {nullptr, "Stack underflow in Negate"};
                auto a = eval_stack.back(); eval_stack.pop_back();
                auto res = cc.new_vec_d();
                cc.fneg(res, a);
                eval_stack.push_back(res);
                break;
            }

            case core::Opcode::Pop: {
                if (!eval_stack.empty()) {
                    eval_stack.pop_back();
                }
                break;
            }

            case core::Opcode::Equal: {
                if (eval_stack.size() < 2) return {nullptr, "Stack underflow in Equal"};
                auto b = eval_stack.back(); eval_stack.pop_back();
                auto a = eval_stack.back(); eval_stack.pop_back();
                cc.fcmp(a, b);
                auto flag = cc.new_gp64();
                cc.cset(flag, asmjit::arm::CondCode::kEQ);
                auto res = cc.new_vec_d();
                cc.scvtf(res, flag);
                eval_stack.push_back(res);
                break;
            }

            case core::Opcode::Greater: {
                if (eval_stack.size() < 2) return {nullptr, "Stack underflow in Greater"};
                auto b = eval_stack.back(); eval_stack.pop_back();
                auto a = eval_stack.back(); eval_stack.pop_back();
                cc.fcmp(a, b);
                auto flag = cc.new_gp64();
                cc.cset(flag, asmjit::arm::CondCode::kGT);
                auto res = cc.new_vec_d();
                cc.scvtf(res, flag);
                eval_stack.push_back(res);
                break;
            }

            case core::Opcode::Less: {
                if (eval_stack.size() < 2) return {nullptr, "Stack underflow in Less"};
                auto b = eval_stack.back(); eval_stack.pop_back();
                auto a = eval_stack.back(); eval_stack.pop_back();
                cc.fcmp(a, b);
                auto flag = cc.new_gp64();
                cc.cset(flag, asmjit::arm::CondCode::kMI);
                auto res = cc.new_vec_d();
                cc.scvtf(res, flag);
                eval_stack.push_back(res);
                break;
            }

            case core::Opcode::Not: {
                if (eval_stack.empty()) return {nullptr, "Stack underflow in Not"};
                auto a = eval_stack.back(); eval_stack.pop_back();
                auto zero = cc.new_vec_d();
                asmjit::a64::Mem c_zero = cc.new_double_const(asmjit::ConstPoolScope::kLocal, 0.0);
                cc.ldr(zero, c_zero);
                cc.fcmp(a, zero);
                auto flag = cc.new_gp64();
                cc.cset(flag, asmjit::arm::CondCode::kEQ);
                auto res = cc.new_vec_d();
                cc.scvtf(res, flag);
                eval_stack.push_back(res);
                break;
            }

            case core::Opcode::BwAnd: {
                if (eval_stack.size() < 2) return {nullptr, "Stack underflow in BwAnd"};
                auto b = eval_stack.back(); eval_stack.pop_back();
                auto a = eval_stack.back(); eval_stack.pop_back();
                auto a_int = cc.new_gp64();
                auto b_int = cc.new_gp64();
                cc.fcvtzs(a_int, a);
                cc.fcvtzs(b_int, b);
                auto res_int = cc.new_gp64();
                cc.and_(res_int, a_int, b_int);
                auto res = cc.new_vec_d();
                cc.scvtf(res, res_int);
                eval_stack.push_back(res);
                break;
            }

            case core::Opcode::BwOr: {
                if (eval_stack.size() < 2) return {nullptr, "Stack underflow in BwOr"};
                auto b = eval_stack.back(); eval_stack.pop_back();
                auto a = eval_stack.back(); eval_stack.pop_back();
                auto a_int = cc.new_gp64();
                auto b_int = cc.new_gp64();
                cc.fcvtzs(a_int, a);
                cc.fcvtzs(b_int, b);
                auto res_int = cc.new_gp64();
                cc.orr(res_int, a_int, b_int);
                auto res = cc.new_vec_d();
                cc.scvtf(res, res_int);
                eval_stack.push_back(res);
                break;
            }

            case core::Opcode::BwXor: {
                if (eval_stack.size() < 2) return {nullptr, "Stack underflow in BwXor"};
                auto b = eval_stack.back(); eval_stack.pop_back();
                auto a = eval_stack.back(); eval_stack.pop_back();
                auto a_int = cc.new_gp64();
                auto b_int = cc.new_gp64();
                cc.fcvtzs(a_int, a);
                cc.fcvtzs(b_int, b);
                auto res_int = cc.new_gp64();
                cc.eor(res_int, a_int, b_int);
                auto res = cc.new_vec_d();
                cc.scvtf(res, res_int);
                eval_stack.push_back(res);
                break;
            }

            case core::Opcode::BwNot: {
                if (eval_stack.empty()) return {nullptr, "Stack underflow in BwNot"};
                auto a = eval_stack.back(); eval_stack.pop_back();
                auto a_int = cc.new_gp64();
                cc.fcvtzs(a_int, a);
                auto res_int = cc.new_gp64();
                cc.mvn(res_int, a_int);
                auto res = cc.new_vec_d();
                cc.scvtf(res, res_int);
                eval_stack.push_back(res);
                break;
            }

            case core::Opcode::ShiftLeft: {
                if (eval_stack.size() < 2) return {nullptr, "Stack underflow in ShiftLeft"};
                auto b = eval_stack.back(); eval_stack.pop_back();
                auto a = eval_stack.back(); eval_stack.pop_back();
                auto a_int = cc.new_gp64();
                auto b_int = cc.new_gp64();
                cc.fcvtzs(a_int, a);
                cc.fcvtzs(b_int, b);
                auto res_int = cc.new_gp64();
                cc.lsl(res_int, a_int, b_int);
                auto res = cc.new_vec_d();
                cc.scvtf(res, res_int);
                eval_stack.push_back(res);
                break;
            }

            case core::Opcode::ShiftRight: {
                if (eval_stack.size() < 2) return {nullptr, "Stack underflow in ShiftRight"};
                auto b = eval_stack.back(); eval_stack.pop_back();
                auto a = eval_stack.back(); eval_stack.pop_back();
                auto a_int = cc.new_gp64();
                auto b_int = cc.new_gp64();
                cc.fcvtzs(a_int, a);
                cc.fcvtzs(b_int, b);
                auto res_int = cc.new_gp64();
                cc.asr(res_int, a_int, b_int);
                auto res = cc.new_vec_d();
                cc.scvtf(res, res_int);
                eval_stack.push_back(res);
                break;
            }

            case core::Opcode::Print: {
                if (eval_stack.empty()) return {nullptr, "Stack underflow in Print"};
                auto val = eval_stack.back(); eval_stack.pop_back();
                auto bits = cc.new_gp64();
                cc.fmov(bits, val);
                asmjit::InvokeNode* invoke = nullptr;
                cc.invoke(asmjit::Out(invoke), reinterpret_cast<uint64_t>(&ely_jit_print_value), asmjit::FuncSignature::build<void, uint64_t>());
                invoke->set_arg(0, bits);
                break;
            }

            case core::Opcode::Call: {
                const auto arg_count = static_cast<size_t>(code[ip++]);
                std::vector<asmjit::a64::Vec> call_args(arg_count);
                for (size_t i = 0; i < arg_count; ++i) {
                    if (eval_stack.empty()) return {nullptr, "Stack underflow in Call arguments"};
                    call_args[arg_count - 1 - i] = eval_stack.back();
                    eval_stack.pop_back();
                }
                if (eval_stack.empty()) return {nullptr, "Stack underflow in Call callee"};
                auto callee = eval_stack.back();
                eval_stack.pop_back();
                auto callee_bits = cc.new_gp64();
                cc.fmov(callee_bits, callee);
                auto res = cc.new_vec_d();
                asmjit::InvokeNode* invoke = nullptr;
                if (arg_count == 0) {
                    cc.invoke(asmjit::Out(invoke), reinterpret_cast<uint64_t>(&ely_jit_call_0), asmjit::FuncSignature::build<double, uint64_t>());
                    invoke->set_arg(0, callee_bits);
                    invoke->set_ret(0, res);
                } else if (arg_count == 1) {
                    cc.invoke(asmjit::Out(invoke), reinterpret_cast<uint64_t>(&ely_jit_call_1), asmjit::FuncSignature::build<double, uint64_t, double>());
                    invoke->set_arg(0, callee_bits);
                    invoke->set_arg(1, call_args[0]);
                    invoke->set_ret(0, res);
                } else if (arg_count == 2) {
                    cc.invoke(asmjit::Out(invoke), reinterpret_cast<uint64_t>(&ely_jit_call_2), asmjit::FuncSignature::build<double, uint64_t, double, double>());
                    invoke->set_arg(0, callee_bits);
                    invoke->set_arg(1, call_args[0]);
                    invoke->set_arg(2, call_args[1]);
                    invoke->set_ret(0, res);
                } else {
                    return {nullptr, "JIT does not yet support calls with > 2 arguments"};
                }
                eval_stack.push_back(res);
                break;
            }

            case core::Opcode::JumpZero: {
                const auto high = static_cast<uint16_t>(code[ip]);
                const auto low = static_cast<uint16_t>(code[ip + 1]);
                const uint16_t offset = static_cast<uint16_t>((high << 8) | low);
                ip += 2;
                const size_t target = ip + offset;
                if (eval_stack.empty()) return {nullptr, "Stack underflow in JumpZero"};
                auto cond = eval_stack.back();
                auto zero = cc.new_vec_d();
                asmjit::a64::Mem c_zero = cc.new_double_const(asmjit::ConstPoolScope::kLocal, 0.0);
                cc.ldr(zero, c_zero);
                cc.fcmp(cond, zero);
                cc.b_eq(labels.at(target));
                break;
            }

            case core::Opcode::Jump: {
                const auto high = static_cast<uint16_t>(code[ip]);
                const auto low = static_cast<uint16_t>(code[ip + 1]);
                const uint16_t offset = static_cast<uint16_t>((high << 8) | low);
                ip += 2;
                const size_t target = ip + offset;
                cc.b(labels.at(target));
                current_terminated = true;
                break;
            }

            case core::Opcode::Loop: {
                const auto high = static_cast<uint16_t>(code[ip]);
                const auto low = static_cast<uint16_t>(code[ip + 1]);
                const uint16_t offset = static_cast<uint16_t>((high << 8) | low);
                ip += 2;
                const size_t target = ip - offset;
                cc.b(labels.at(target));
                current_terminated = true;
                break;
            }

            case core::Opcode::Return: {
                asmjit::a64::Vec res;
                if (!eval_stack.empty()) {
                    res = eval_stack.back();
                    eval_stack.pop_back();
                } else {
                    res = cc.new_vec_d();
                    asmjit::a64::Mem c_zero = cc.new_double_const(asmjit::ConstPoolScope::kLocal, 0.0);
                    cc.ldr(res, c_zero);
                }
                cc.ret(res);
                current_terminated = true;
                break;
            }

            case core::Opcode::Nop: {
                break;
            }

            case core::Opcode::Import: {
                const auto const_idx = static_cast<uint32_t>(code[ip++]);
                asmjit::InvokeNode* invoke = nullptr;
                cc.invoke(asmjit::Out(invoke), reinterpret_cast<uint64_t>(&ely_jit_import_module), asmjit::FuncSignature::build<void, uint32_t>());
                invoke->set_arg(0, asmjit::Imm(const_idx));
                break;
            }

            default:
                return {nullptr, std::string("Unsupported opcode for JIT: ") + std::string(core::opcode_name(op))};
        }
    }

    if (!current_terminated) {
        asmjit::a64::Vec res;
        if (!eval_stack.empty()) {
            res = eval_stack.back();
        } else {
            res = cc.new_vec_d();
            asmjit::a64::Mem c_zero = cc.new_double_const(asmjit::ConstPoolScope::kLocal, 0.0);
            cc.ldr(res, c_zero);
        }
        cc.ret(res);
    }

    cc.end_func();

    if (dump_ir) {
        asmjit::String sb;
        asmjit::FormatOptions format_options;
        asmjit::Formatter::format_node_list(sb, format_options, &cc);
        std::cout << "=== AsmJit IR [" << name << "] ===\n";
        std::cout << sb.data() << "\n";
    }

    asmjit::StringLogger asm_logger;
    if (dump_asm) {
        asm_logger.add_flags(asmjit::FormatFlags::kMachineCode);
        code_holder.set_logger(&asm_logger);
    }

    const asmjit::Error finalize_err = cc.finalize();
    if (finalize_err != asmjit::Error::kOk) {
        return {nullptr, std::string("Finalize error: ") + asmjit::stringify_error(finalize_err)};
    }

    if (dump_asm) {
        code_holder.reset_logger();
        std::cout << "=== Machine Code Disassembly [" << name << "] ===\n";
        std::cout << asm_logger.data() << "\n";
    }

#if defined(__aarch64__) || defined(_M_ARM64)
    void* fn_ptr = nullptr;
    const asmjit::Error add_err = m_engine.runtime().add(&fn_ptr, &code_holder);
    if (add_err != asmjit::Error::kOk) {
        return {nullptr, std::string("JitRuntime add error: ") + asmjit::stringify_error(add_err)};
    }

    m_engine.track_allocation(fn_ptr);
    return {reinterpret_cast<const uint8_t*>(fn_ptr), {}};
#else
    return {nullptr, {}};
#endif
}

} // namespace ely::jit
