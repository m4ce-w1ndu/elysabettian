#include <ely/compiler/compiler.hpp>
#include <ely/core/opcode.hpp>
#include <ely/core/value.hpp>
#include <ely/vm/heap_object.hpp>
#include <ely/vm/vm.hpp>
#include <algorithm>
#include <cassert>
#include <format>
#include <iostream>
#include <span>
#include <variant>

namespace ely::vm {

void VM::push(core::Value value) {
    m_stack.push_back(value);
}

core::Value VM::pop() {
    assert(!m_stack.empty() && "Stack is empty. Aborting...");
    auto back = m_stack.back();
    m_stack.pop_back();
    return back;
}

core::Value VM::peek(size_t distance) const {
    assert(distance < m_stack.size() && "Stack size exceeded! Cannot peek.");
    return m_stack[m_stack.size() - 1 - distance];
}

void VM::reset_stack() noexcept {
    m_stack.clear();
    m_frames.clear();
    m_open_upvalues.clear();
}

core::Value VM::allocate_object(HeapObject obj) {
    const auto id = m_heap.allocate(std::move(obj));
    return core::Value::obj(id);
}

const HeapObject& VM::get_object(core::Value value) const {
    const auto obj = value.as_obj();
    return m_heap.get(static_cast<size_t>(obj));
}

HeapObject& VM::get_object(core::Value value) {
    const auto obj = value.as_obj();
    return m_heap.get(static_cast<size_t>(obj));
}

size_t VM::capture_upvalue(size_t location) {
    auto it = std::ranges::find_if(m_open_upvalues, [this, location](size_t id) {
        const auto* u = m_heap.get_if<ObjUpvalue>(id);
        return u && u->is_open() && u->location == location;
    });

    // Already existing
    if (it != m_open_upvalues.end()) return *it;

    // Let's create a new one on the heap
    const auto id = m_heap.emplace<ObjUpvalue>(location);
    m_open_upvalues.push_back(id);
    return id;
}

void VM::close_upvalue(size_t slot) {
    std::erase_if(m_open_upvalues, [this, slot](size_t id) {
        auto* u = m_heap.get_if<ObjUpvalue>(id);
        if (u && u->location >= slot) {
            u->closed = m_stack[u->location];
            return true;
        }

        return false;
    });
}

std::optional<core::Value> VM::get_upvalue_value(size_t closure_id, size_t slot) const {
    const auto* closure = m_heap.get_if<ObjClosure>(closure_id);
    if (!closure || (closure && slot >= closure->upvalues.size())) return std::nullopt;

    const auto* u = m_heap.get_if<ObjUpvalue>(closure->upvalues[slot]);

    return u && u->is_open()
        ? m_stack[u->location]
        : u && !u->is_open()
        ? u->closed
        : std::nullopt;
}

bool VM::set_upvalue_value(size_t closure_id, size_t slot, core::Value value) {
    const auto* closure = m_heap.get_if<ObjClosure>(closure_id);
    if (!closure || (closure && slot >= closure->upvalues.size())) return false;

    auto* u = m_heap.get_if<ObjUpvalue>(closure->upvalues[slot]);
    if (!u) return false;

    if (u->is_open()) {
        m_stack[u->location] = value;
    } else {
        u->closed = value;
    }

    return true;
}

void VM::define_native(uint32_t name_id, stdlib::CallableNative native) {
    m_natives.insert_or_assign(name_id, std::move(native));
}

template <class... Ts>
struct overloaded : Ts... { using Ts::operator()...; };

std::string VM::format_value(core::Value value) const {
    if (value.is_number()) return std::format("{}", value.as_number());
    if (value.is_null()) return "null";
    if (value.is_boolean()) return value.as_boolean() ? "true" : "false";
    if (value.is_obj()) {
        const auto& obj = get_object(value);
        return std::visit(overloaded {
            [](const std::string& str) { return str; },
            [this](const std::vector<core::Value>& arr) {
                std::string res = "[";
                for (size_t i = 0; i < arr.size(); ++i) {
                    if (i > 0) res += ", ";
                    res += format_value(arr[i]);
                }
                res += "]";
                return res;
            },
            [](const ObjFunction& fn) {
                return "<fn " + fn.name.value_or("script") + ">";
            },
            [](const ObjClosure&) {
                return std::string("<closure>");
            },
            [](const ObjClass& c) {
                return "<class " + c.name + ">";
            },
            [](const ObjInstance& inst) {
                return "<instance of class " + std::to_string(inst.class_id) + ">";
            },
            [](const ObjBoundMethod&) {
                return std::string("<bound method>");
            },
            [](const ObjUpvalue&) {
                return std::string("<upvalue>");
            }
        }, obj);
    }

    return "<unknown>";
}

void VM::load_chunk_objects(core::Chunk &chunk) {
    std::vector<core::Value> string_vals;
    string_vals.reserve(chunk.string_constants().size());
    for (const auto& str : chunk.string_constants()) {
        string_vals.push_back(allocate_object(HeapObject(str)));
    }

    std::vector<core::Value> func_vals;
    func_vals.reserve(chunk.function_constants().size());
    for (auto& f : chunk.function_constants()) {
        if (f.chunk) {
            load_chunk_objects(*f.chunk);
        }

        ObjFunction obj_func(f.arity, f.name);
        obj_func.chunk = std::move(f.chunk);
        func_vals.push_back(allocate_object(HeapObject(std::move(obj_func))));
    }

    size_t ip = 0;
    const auto code = chunk.code();

    while (ip < code.size()) {
        const auto opcode = static_cast<core::Opcode>(code[ip++]);

        switch (opcode) {
            case core::Opcode::Constant: {
                const auto const_idx = static_cast<size_t>(code[ip++]);
                if (auto c = chunk.get_constant(const_idx); c && c->is_obj()) {
                    const auto orig_idx = static_cast<size_t>(c->as_obj());
                    if (orig_idx < string_vals.size()) {
                        chunk.patch_constant(const_idx, string_vals[orig_idx]);
                    }
                }
                break;
            }

            case core::Opcode::Closure: {
                const auto const_idx = static_cast<size_t>(code[ip++]);
                if (auto c = chunk.get_constant(const_idx); c && c->is_obj()) {
                    const auto orig_idx = static_cast<size_t>(c->as_obj());
                    if (orig_idx < func_vals.size()) {
                        chunk.patch_constant(const_idx, func_vals[orig_idx]);
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
            case core::Opcode::ArrayBuild:
            case core::Opcode::Import:
                ip++;
                break;

            case core::Opcode::Invoke:
            case core::Opcode::SuperInvoke:
                ip += 2;
                break;

            case core::Opcode::Jump:
            case core::Opcode::JumpZero:
            case core::Opcode::Loop:
                ip += 2;
                break;

            default: break;
        }
    }
}

InterpretResult VM::run() {
    auto hash_string = [](std::string_view str) noexcept -> uint32_t {
        uint32_t hash = 2166136261u;
        for (char c : str) {
            hash ^= static_cast<uint8_t>(c);
            hash *= 16777619u;
        }
        return hash;
    };

    auto read_name_id = [&](CallFrame& frame) -> uint32_t {
        const uint8_t byte_val = frame.read_byte();
        if (byte_val < frame.chunk->string_constants().size()) {
            return hash_string(frame.chunk->string_constants()[byte_val]);
        }
        if (byte_val < frame.chunk->constants().size() && frame.chunk->constants()[byte_val].is_obj()) {
            return static_cast<uint32_t>(frame.chunk->constants()[byte_val].as_obj());
        }
        return static_cast<uint32_t>(byte_val);
    };

    auto binary_num_op = [&](auto op) -> bool {
        if (!peek(0).is_number() || !peek(1).is_number()) return false;
        const double b = pop().as_number();
        const double a = pop().as_number();
        push(core::Value::number(op(a, b)));
        return true;
    };

    auto bitwise_op = [&](auto op) -> bool {
        if (!peek(0).is_number() || !peek(1).is_number()) return false;
        const auto b = static_cast<int64_t>(pop().as_number());
        const auto a = static_cast<int64_t>(pop().as_number());
        push(core::Value::number(static_cast<double>(op(a, b))));
        return true;
    };

    while (true) {
        if (m_frames.empty()) return InterpretResult::Ok;

        auto& frame = m_frames.back();
        const auto instruction = frame.read_byte();
        const auto opcode = static_cast<core::Opcode>(instruction);

        switch (opcode) {
            case core::Opcode::Constant: {
                push(frame.read_constant());
                break;
            }

            case core::Opcode::Nop: {
                push(core::Value::null());
                break;
            }

            case core::Opcode::True: {
                push(core::Value::boolean(true));
                break;
            }

            case core::Opcode::False: {
                push(core::Value::boolean(false));
                break;
            }

            case core::Opcode::Pop: {
                (void)pop();
                break;
            }

            case core::Opcode::Return: {
                const auto result = pop();
                const auto closing_frame = m_frames.back();
                m_frames.pop_back();
                close_upvalue(closing_frame.slots);

                m_stack.resize(closing_frame.slots);
                push(result);

                if (m_frames.empty()) {
                    return InterpretResult::Ok;
                }
                break;
            }

            case core::Opcode::Add: {
                if (peek(0).is_number() && peek(1).is_number()) {
                    const double b = pop().as_number();
                    const double a = pop().as_number();
                    push(core::Value::number(a + b));
                } else {
                    const auto b = pop();
                    const auto a = pop();
                    auto str = format_value(a) + format_value(b);
                    push(allocate_object(std::move(str)));
                }
                break;
            }

            case core::Opcode::Subtract: {
                if (!binary_num_op([](double a, double b) { return a - b; })) {
                    return InterpretResult::RuntimeError;
                }
                break;
            }

            case core::Opcode::Multiply: {
                if (!binary_num_op([](double a, double b) { return a * b; })) {
                    return InterpretResult::RuntimeError;
                }
                break;
            }

            case core::Opcode::Divide: {
                if (!binary_num_op([](double a, double b) { return a / b; })) {
                    return InterpretResult::RuntimeError;
                }
                break;
            }

            case core::Opcode::Negate: {
                if (!peek(0).is_number()) return InterpretResult::RuntimeError;
                const double n = pop().as_number();
                push(core::Value::number(-n));
                break;
            }

            case core::Opcode::BwAnd: {
                if (!bitwise_op([](int64_t a, int64_t b) { return a & b; })) {
                    return InterpretResult::RuntimeError;
                }
                break;
            }

            case core::Opcode::BwOr: {
                if (!bitwise_op([](int64_t a, int64_t b) { return a | b; })) {
                    return InterpretResult::RuntimeError;
                }
                break;
            }

            case core::Opcode::BwXor: {
                if (!bitwise_op([](int64_t a, int64_t b) { return a ^ b; })) {
                    return InterpretResult::RuntimeError;
                }
                break;
            }

            case core::Opcode::ShiftLeft: {
                if (!bitwise_op([](int64_t a, int64_t b) { return a << b; })) {
                    return InterpretResult::RuntimeError;
                }
                break;
            }

            case core::Opcode::ShiftRight: {
                if (!bitwise_op([](int64_t a, int64_t b) { return a >> b; })) {
                    return InterpretResult::RuntimeError;
                }
                break;
            }

            case core::Opcode::BwNot: {
                if (!peek(0).is_number()) return InterpretResult::RuntimeError;
                const auto a = static_cast<int64_t>(pop().as_number());
                push(core::Value::number(static_cast<double>(~a)));
                break;
            }

            case core::Opcode::Not: {
                const auto val = pop();
                push(core::Value::boolean(!val.is_truthy()));
                break;
            }

            case core::Opcode::Equal: {
                const auto b = pop();
                const auto a = pop();
                push(core::Value::boolean(a == b));
                break;
            }

            case core::Opcode::Greater: {
                if (!peek(0).is_number() || !peek(1).is_number()) return InterpretResult::RuntimeError;
                const double b = pop().as_number();
                const double a = pop().as_number();
                push(core::Value::boolean(a > b));
                break;
            }

            case core::Opcode::Less: {
                if (!peek(0).is_number() || !peek(1).is_number()) return InterpretResult::RuntimeError;
                const double b = pop().as_number();
                const double a = pop().as_number();
                push(core::Value::boolean(a < b));
                break;
            }

            case core::Opcode::DefineGlobal: {
                const auto name_id = read_name_id(frame);
                const auto val = pop();
                m_globals.insert_or_assign(name_id, val);
                break;
            }

            case core::Opcode::GetGlobal: {
                const auto name_id = read_name_id(frame);
                if (auto it = m_globals.find(name_id); it != m_globals.end()) {
                    push(it->second);
                } else if (m_natives.contains(name_id)) {
                    push(core::Value::obj(name_id));
                } else {
                    return InterpretResult::RuntimeError;
                }
                break;
            }

            case core::Opcode::SetGlobal: {
                const auto name_id = read_name_id(frame);
                auto it = m_globals.find(name_id);
                if (it == m_globals.end()) {
                    return InterpretResult::RuntimeError;
                }
                it->second = peek(0);
                break;
            }

            case core::Opcode::GetLocal: {
                const auto slot = static_cast<size_t>(frame.read_byte());
                push(m_stack[frame.slots + slot]);
                break;
            }

            case core::Opcode::SetLocal: {
                const auto slot = static_cast<size_t>(frame.read_byte());
                m_stack[frame.slots + slot] = peek(0);
                break;
            }

            case core::Opcode::GetUpvalue: {
                const auto slot = static_cast<size_t>(frame.read_byte());
                if (!frame.closure_id) return InterpretResult::RuntimeError;
                const auto val = get_upvalue_value(*frame.closure_id, slot);
                if (!val) return InterpretResult::RuntimeError;
                push(*val);
                break;
            }

            case core::Opcode::SetUpvalue: {
                const auto slot = static_cast<size_t>(frame.read_byte());
                if (!frame.closure_id) return InterpretResult::RuntimeError;
                if (!set_upvalue_value(*frame.closure_id, slot, peek(0))) {
                    return InterpretResult::RuntimeError;
                }
                break;
            }

            case core::Opcode::Jump: {
                const auto offset = frame.read_short();
                frame.ip += offset;
                break;
            }

            case core::Opcode::JumpZero: {
                const auto val = peek(0);
                const auto offset = frame.read_short();
                if (!val.is_truthy()) {
                    frame.ip += offset;
                }
                break;
            }

            case core::Opcode::Loop: {
                const auto offset = frame.read_short();
                frame.ip -= offset;
                break;
            }

            case core::Opcode::Call: {
                const auto arg_count = static_cast<size_t>(frame.read_byte());
                const auto callee = peek(arg_count);

                if (callee.is_obj()) {
                    const auto name_id = static_cast<uint32_t>(callee.as_obj());
                    if (auto it = m_natives.find(name_id); it != m_natives.end()) {
                        const size_t args_start = m_stack.size() - arg_count;
                        std::span<const core::Value> args_span(m_stack.data() + args_start, arg_count);
                        const auto result = it->second.invoke(args_span);
                        m_stack.resize(args_start - 1);
                        push(result);
                        break;
                    }

                    const auto obj_id = static_cast<size_t>(callee.as_obj());
                    if (auto* fn = m_heap.get_if<ObjFunction>(obj_id)) {
                        if (arg_count != fn->arity || m_frames.size() == kMaxFrames) {
                            return InterpretResult::RuntimeError;
                        }
                        fn->invocation_count++;
                        const size_t slots = m_stack.size() - arg_count - 1;
                        m_frames.emplace_back(fn->chunk.get(), slots);
                    } else if (auto* closure = m_heap.get_if<ObjClosure>(obj_id)) {
                        auto* cfn = m_heap.get_if<ObjFunction>(closure->function_id);
                        if (!cfn || arg_count != cfn->arity || m_frames.size() == kMaxFrames) {
                            return InterpretResult::RuntimeError;
                        }
                        cfn->invocation_count++;
                        const size_t slots = m_stack.size() - arg_count - 1;
                        m_frames.emplace_back(cfn->chunk.get(), slots, obj_id);
                    } else if (m_heap.get_if<ObjClass>(obj_id)) {
                        const auto inst_val = allocate_object(ObjInstance(obj_id));
                        const size_t callee_slot = m_stack.size() - arg_count - 1;
                        m_stack[callee_slot] = inst_val;
                    } else if (auto* bound = m_heap.get_if<ObjBoundMethod>(obj_id)) {
                        const auto receiver = bound->receiver;
                        const auto method_id = bound->method_id;
                        const size_t callee_slot = m_stack.size() - arg_count - 1;
                        m_stack[callee_slot] = receiver;

                        const ObjFunction* mfn = m_heap.get_if<ObjFunction>(method_id);
                        std::optional<size_t> closure_id = std::nullopt;
                        if (!mfn) {
                            if (auto* c = m_heap.get_if<ObjClosure>(method_id)) {
                                mfn = m_heap.get_if<ObjFunction>(c->function_id);
                                closure_id = method_id;
                            }
                        }
                        if (!mfn || arg_count != mfn->arity || m_frames.size() == kMaxFrames) {
                            return InterpretResult::RuntimeError;
                        }
                        const size_t slots = m_stack.size() - arg_count - 1;
                        if (closure_id) {
                            m_frames.emplace_back(mfn->chunk.get(), slots, *closure_id);
                        } else {
                            m_frames.emplace_back(mfn->chunk.get(), slots);
                        }
                    } else {
                        return InterpretResult::RuntimeError;
                    }
                } else {
                    return InterpretResult::RuntimeError;
                }
                break;
            }

            case core::Opcode::Print: {
                const auto value = pop();
                std::cout << format_value(value) << '\n';
                break;
            }

            case core::Opcode::CloseUpvalue: {
                close_upvalue(m_stack.size() - 1);
                (void)pop();
                break;
            }

            case core::Opcode::Closure: {
                const auto const_idx = static_cast<size_t>(frame.read_byte());
                const auto func_val = frame.chunk->constants()[const_idx];
                if (!func_val.is_obj()) return InterpretResult::RuntimeError;
                const auto func_id = static_cast<size_t>(func_val.as_obj());

                const auto* func = m_heap.get_if<ObjFunction>(func_id);
                if (!func) return InterpretResult::RuntimeError;

                const auto upvalue_count = func->upvalue_count;
                const auto current_slots = frame.slots;
                const auto current_closure_id = frame.closure_id;

                ObjClosure closure(func_id);
                closure.upvalues.reserve(upvalue_count);

                for (size_t i = 0; i < upvalue_count; ++i) {
                    const auto is_local = frame.read_byte();
                    const auto index = static_cast<size_t>(frame.read_byte());

                    if (is_local == 1) {
                        closure.upvalues.push_back(capture_upvalue(current_slots + index));
                    } else {
                        if (!current_closure_id) return InterpretResult::RuntimeError;
                        const auto* enc = m_heap.get_if<ObjClosure>(*current_closure_id);
                        if (!enc || index >= enc->upvalues.size()) return InterpretResult::RuntimeError;
                        closure.upvalues.push_back(enc->upvalues[index]);
                    }
                }

                push(allocate_object(std::move(closure)));
                break;
            }

            case core::Opcode::ArrayBuild: {
                const auto count = static_cast<size_t>(frame.read_byte());
                const size_t start = m_stack.size() - count;
                std::vector<core::Value> elements;
                elements.reserve(count);
                for (size_t i = start; i < m_stack.size(); ++i) {
                    elements.push_back(m_stack[i]);
                }
                m_stack.resize(start);
                push(allocate_object(std::move(elements)));
                break;
            }

            case core::Opcode::ArrayIndex: {
                const auto index_value = pop();
                const auto array_value = pop();

                if (!index_value.is_number() || !array_value.is_obj()) {
                    return InterpretResult::RuntimeError;
                }

                const auto idx = static_cast<size_t>(index_value.as_number());
                const auto* arr = m_heap.get_if<std::vector<core::Value>>(static_cast<size_t>(array_value.as_obj()));
                if (!arr || idx >= arr->size()) {
                    return InterpretResult::RuntimeError;
                }

                push((*arr)[idx]);
                break;
            }

            case core::Opcode::ArrayStore: {
                const auto val = pop();
                const auto index_value = pop();
                const auto array_value = pop();

                if (!index_value.is_number() || !array_value.is_obj()) {
                    return InterpretResult::RuntimeError;
                }

                const auto idx = static_cast<size_t>(index_value.as_number());
                auto* arr = m_heap.get_if<std::vector<core::Value>>(static_cast<size_t>(array_value.as_obj()));
                if (!arr || idx >= arr->size()) {
                    return InterpretResult::RuntimeError;
                }

                (*arr)[idx] = val;
                push(val);
                break;
            }

            case core::Opcode::Class: {
                const auto name_byte = frame.read_byte();
                std::string name;
                if (name_byte < frame.chunk->constants().size() && frame.chunk->constants()[name_byte].is_obj()) {
                    name = std::format("Class_{}", frame.chunk->constants()[name_byte].as_obj());
                } else if (name_byte < frame.chunk->string_constants().size()) {
                    name = frame.chunk->string_constants()[name_byte];
                } else {
                    name = std::format("Class_{}", name_byte);
                }
                push(allocate_object(ObjClass(std::move(name))));
                break;
            }

            case core::Opcode::Method: {
                const auto name_id = read_name_id(frame);
                const auto method_val = pop();
                const auto class_val = peek(0);

                if (!method_val.is_obj() || !class_val.is_obj()) {
                    return InterpretResult::RuntimeError;
                }

                auto* class_obj = m_heap.get_if<ObjClass>(static_cast<size_t>(class_val.as_obj()));
                if (!class_obj) return InterpretResult::RuntimeError;

                class_obj->methods[name_id] = static_cast<size_t>(method_val.as_obj());
                break;
            }

            case core::Opcode::GetProperty: {
                const auto name_id = read_name_id(frame);
                const auto instance_val = pop();
                if (!instance_val.is_obj()) return InterpretResult::RuntimeError;

                const auto* inst = m_heap.get_if<ObjInstance>(static_cast<size_t>(instance_val.as_obj()));
                if (!inst) return InterpretResult::RuntimeError;

                if (auto it = inst->fields.find(name_id); it != inst->fields.end()) {
                    push(it->second);
                } else {
                    // Method binding fallback
                    const auto* cls = m_heap.get_if<ObjClass>(inst->class_id);
                    if (!cls) return InterpretResult::RuntimeError;
                    auto mit = cls->methods.find(name_id);
                    if (mit == cls->methods.end()) return InterpretResult::RuntimeError;
                    push(allocate_object(ObjBoundMethod(instance_val, mit->second)));
                }
                break;
            }

            case core::Opcode::SetProperty: {
                const auto name_id = read_name_id(frame);
                const auto value = pop();
                const auto instance_val = pop();
                if (!instance_val.is_obj()) return InterpretResult::RuntimeError;

                auto* inst = m_heap.get_if<ObjInstance>(static_cast<size_t>(instance_val.as_obj()));
                if (!inst) return InterpretResult::RuntimeError;

                inst->fields[name_id] = value;
                push(value);
                break;
            }

            case core::Opcode::Inherit: {
                const auto subclass_val = peek(0);
                const auto superclass_val = peek(1);

                if (!subclass_val.is_obj() || !superclass_val.is_obj()) {
                    return InterpretResult::RuntimeError;
                }

                const auto* super_cls = m_heap.get_if<ObjClass>(static_cast<size_t>(superclass_val.as_obj()));
                if (!super_cls) return InterpretResult::RuntimeError;
                const auto super_methods = super_cls->methods;

                auto* sub_cls = m_heap.get_if<ObjClass>(static_cast<size_t>(subclass_val.as_obj()));
                if (!sub_cls) return InterpretResult::RuntimeError;

                for (const auto& [name, method] : super_methods) {
                    sub_cls->methods.try_emplace(name, method);
                }

                (void)pop(); // Pop subclass
                break;
            }

            case core::Opcode::GetSuper: {
                const auto name_id = read_name_id(frame);
                const auto superclass_val = pop();
                const auto instance_val = peek(0);

                if (!superclass_val.is_obj()) return InterpretResult::RuntimeError;
                const auto* super_cls = m_heap.get_if<ObjClass>(static_cast<size_t>(superclass_val.as_obj()));
                if (!super_cls) return InterpretResult::RuntimeError;

                auto mit = super_cls->methods.find(name_id);
                if (mit == super_cls->methods.end()) return InterpretResult::RuntimeError;

                const auto bound_val = allocate_object(ObjBoundMethod(instance_val, mit->second));
                (void)pop(); // Pop instance
                push(bound_val);
                break;
            }

            case core::Opcode::Invoke: {
                const auto name_id = read_name_id(frame);
                const auto arg_count = static_cast<size_t>(frame.read_byte());
                const auto receiver_val = peek(arg_count);
                if (!receiver_val.is_obj()) return InterpretResult::RuntimeError;

                const auto* inst = m_heap.get_if<ObjInstance>(static_cast<size_t>(receiver_val.as_obj()));
                if (!inst) return InterpretResult::RuntimeError;

                const auto class_id = inst->class_id;
                auto fit = inst->fields.find(name_id);
                if (fit != inst->fields.end()) {
                    const auto field_val = fit->second;
                    const size_t receiver_slot = m_stack.size() - arg_count - 1;
                    m_stack[receiver_slot] = field_val;

                    if (field_val.is_obj()) {
                        const auto nid = static_cast<uint32_t>(field_val.as_obj());
                        if (auto nit = m_natives.find(nid); nit != m_natives.end()) {
                            const size_t args_start = m_stack.size() - arg_count;
                            std::span<const core::Value> args_span(m_stack.data() + args_start, arg_count);
                            const auto result = nit->second.invoke(args_span);
                            m_stack.resize(args_start - 1);
                            push(result);
                            break;
                        }

                        const auto fid = static_cast<size_t>(field_val.as_obj());
                        if (auto* fn = m_heap.get_if<ObjFunction>(fid)) {
                            if (arg_count != fn->arity || m_frames.size() == kMaxFrames) {
                                return InterpretResult::RuntimeError;
                            }
                            fn->invocation_count++;
                            const size_t slots = m_stack.size() - arg_count - 1;
                            m_frames.emplace_back(fn->chunk.get(), slots);
                        } else if (auto* closure = m_heap.get_if<ObjClosure>(fid)) {
                            auto* cfn = m_heap.get_if<ObjFunction>(closure->function_id);
                            if (!cfn || arg_count != cfn->arity || m_frames.size() == kMaxFrames) {
                                return InterpretResult::RuntimeError;
                            }
                            cfn->invocation_count++;
                            const size_t slots = m_stack.size() - arg_count - 1;
                            m_frames.emplace_back(cfn->chunk.get(), slots, fid);
                        } else {
                            return InterpretResult::RuntimeError;
                        }
                    } else {
                        return InterpretResult::RuntimeError;
                    }
                } else {
                    const auto* cls = m_heap.get_if<ObjClass>(class_id);
                    if (!cls) return InterpretResult::RuntimeError;

                    auto mit = cls->methods.find(name_id);
                    if (mit == cls->methods.end()) return InterpretResult::RuntimeError;
                    const auto method_id = mit->second;

                    const ObjFunction* fn = m_heap.get_if<ObjFunction>(method_id);
                    std::optional<size_t> closure_id = std::nullopt;
                    if (!fn) {
                        if (auto* c = m_heap.get_if<ObjClosure>(method_id)) {
                            fn = m_heap.get_if<ObjFunction>(c->function_id);
                            closure_id = method_id;
                        }
                    }

                    if (!fn || arg_count != fn->arity || m_frames.size() == kMaxFrames) {
                        return InterpretResult::RuntimeError;
                    }

                    const size_t slots = m_stack.size() - arg_count - 1;
                    if (closure_id) {
                        m_frames.emplace_back(fn->chunk.get(), slots, *closure_id);
                    } else {
                        m_frames.emplace_back(fn->chunk.get(), slots);
                    }
                }
                break;
            }

            case core::Opcode::SuperInvoke: {
                const auto name_id = read_name_id(frame);
                const auto arg_count = static_cast<size_t>(frame.read_byte());
                const auto superclass_val = pop();
                if (!superclass_val.is_obj()) return InterpretResult::RuntimeError;

                const auto* super_cls = m_heap.get_if<ObjClass>(static_cast<size_t>(superclass_val.as_obj()));
                if (!super_cls) return InterpretResult::RuntimeError;

                auto mit = super_cls->methods.find(name_id);
                if (mit == super_cls->methods.end()) return InterpretResult::RuntimeError;
                const auto method_id = mit->second;

                const ObjFunction* fn = m_heap.get_if<ObjFunction>(method_id);
                std::optional<size_t> closure_id = std::nullopt;
                if (!fn) {
                    if (auto* c = m_heap.get_if<ObjClosure>(method_id)) {
                        fn = m_heap.get_if<ObjFunction>(c->function_id);
                        closure_id = method_id;
                    }
                }

                if (!fn || arg_count != fn->arity || m_frames.size() == kMaxFrames) {
                    return InterpretResult::RuntimeError;
                }

                const size_t slots = m_stack.size() - arg_count - 1;
                if (closure_id) {
                    m_frames.emplace_back(fn->chunk.get(), slots, *closure_id);
                } else {
                    m_frames.emplace_back(fn->chunk.get(), slots);
                }
                break;
            }

            case core::Opcode::Import: {
                const auto const_idx = static_cast<size_t>(frame.read_byte());
                if (const_idx >= frame.chunk->string_constants().size()) {
                    return InterpretResult::RuntimeError;
                }
                break;
            }

            default:
                std::cerr << "RuntimeError on opcode " << core::opcode_name(opcode) << "\n";
                return InterpretResult::RuntimeError;
        }
    }
}

InterpretResult VM::interpret(const core::Chunk& chunk) {
    reset_stack();
    push(core::Value::null());

    auto local_chunk = chunk;
    load_chunk_objects(local_chunk);

    m_frames.emplace_back(&local_chunk, 0);
    return run();
}

InterpretResult VM::interpret(std::string_view source) {
    compiler::Compiler comp(source);
    auto chunk = comp.compile();
    if (!chunk) return InterpretResult::CompileError;
    return interpret(*chunk);
}

} // namespace ely::vm
