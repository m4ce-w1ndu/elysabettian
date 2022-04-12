//
//  VirtualMachine.cpp
//  CElysabettian
//
//  Created by Simone Rolando on 11/07/21.
//

#include "VirtualMachine.hpp"

#include <cstdarg>
#include <exception>

template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

const std::string operator+(const std::string& str, double num)
{
    auto str_num = std::to_string(num);
    return str + str_num;
}

const std::string operator+(double num, const std::string& str)
{
    auto str_num = std::to_string(num);
    return str_num + str;
}

struct CallVisitor {
    const int arg_count;
    VM& vm;
    
    CallVisitor(int arg_count, VM& vm)
        : arg_count(arg_count), vm(vm) {}
    
    bool operator()(const NativeFunction& native) const
    {
        auto result = native->function(arg_count, vm.stack.end() - arg_count);
        try {
            vm.stack.resize(vm.stack.size() - arg_count - 1);
            vm.stack.reserve(STACK_MAX);
            vm.push(std::move(result));
        } catch (std::length_error) {
            return false;
        }
        return true;
    }
    
    bool operator()(const Closure& closure) const
    {
        return vm.call(closure, arg_count);
    }
    
    bool operator()(const ClassValue& class_value) const
    {
        // increment reference count to avoid premature deletion
        auto class_val = class_value;
        vm.stack[vm.stack.size() - arg_count - 1] = std::make_shared<InstanceObject>(class_value);
        try {
            auto& funcs = class_val->methods;
            auto found = funcs.at(vm.init_string);
            return vm.call(found, arg_count);
        }
        catch (std::out_of_range&) {
            vm.runtime_error("Expected 0 arguments but got %d.", arg_count);
            return false;
        }
        return true;
    }
    
    bool operator()(const BoundMethodValue& bound) const
    {
        vm.stack[vm.stack.size() - arg_count - 1] = bound->receiver;
        return vm.call(bound->method, arg_count);
    }

    template <typename T>
    bool operator()(const T& value) const
    {
        vm.runtime_error("Can only call functions and classes.");
        return false;
    }
};

bool VM::call_value(const Value& callee, int arg_count)
{
    return std::visit(CallVisitor(arg_count, *this), callee);
}

bool VM::invoke(const std::string& name, int arg_count)
{
    try {
        auto instance = std::get<InstanceValue>(peek(arg_count));
        
        auto found = instance->fields.find(name);
        if (found != instance->fields.end()) {
            auto value = found->second;
            stack[stack.size() - arg_count - 1] = value;
            return call_value(value, arg_count);
        }
        
        return invoke_from_class(instance->class_value, name, arg_count);
    } catch (std::bad_variant_access&) {
        runtime_error("Only instances have methods.");
        return false;
    }
}

bool VM::invoke_from_class(ClassValue class_value, const std::string& name, int arg_count)
{
    auto found = class_value->methods.find(name);
    if (found == class_value->methods.end()) {
        runtime_error("Undefined property '%s'.", name.c_str());
        return false;
    }
    auto method = found->second;
    return call(method, arg_count);
}

bool VM::bind_method(ClassValue class_value, const std::string& name)
{
    auto found = class_value->methods.find(name);
    if (found == class_value->methods.end()) {
        runtime_error("Undefined property '%s'.", name.c_str());
        return false;
    }
    auto method = found->second;
    auto instance = std::get<InstanceValue>(peek(0));
    auto bound = std::make_shared<MemberFuncObject>(instance, method);
    
    pop();
    push(bound);
    
    return true;
}

UpvalueValue VM::capture_upvalue(Value* local)
{
    UpvalueValue prev_upvalue = nullptr;
    auto upvalue = open_upvalues;
    
    while (upvalue != nullptr && upvalue->location > local) {
        prev_upvalue = upvalue;
        upvalue = upvalue->next;
    }
    
    if (upvalue != nullptr && upvalue->location == local) {
        return upvalue;
    }
    
    auto new_upvalue = std::make_shared<UpvalueObject>(local);
    new_upvalue->next = upvalue;
    
    if (prev_upvalue == nullptr) {
        open_upvalues = new_upvalue;
    } else {
        prev_upvalue->next = new_upvalue;
    }
    
    return new_upvalue;
}

void VM::close_upvalues(Value* last)
{
    while (open_upvalues != nullptr && open_upvalues->location >= last){
        auto& upvalue = open_upvalues;
        upvalue->closed = *upvalue->location;
        upvalue->location = &upvalue->closed;
        open_upvalues = upvalue->next;
    }
}

void VM::define_method(const std::string& name)
{
    auto method = std::get<Closure>(peek(0));
    auto class_value = std::get<ClassValue>(peek(1));
    class_value->methods[name] = method;
    pop();
}

bool VM::call(const Closure& closure, int arg_count)
{
    if (arg_count != closure->function->arity) {
        runtime_error("Expected %d arguments but got %d.",
                     closure->function->arity, arg_count);
        return false;
    }
    
    if (frames.size() + 1 == FRAMES_MAX) {
        runtime_error("Stack overflow.");
        return false;
    }

    frames.emplace_back(CallFrame());
    auto& frame = frames.back();
    frame.ip = 0;
    frame.closure = closure;
    frame.stack_offset = static_cast<unsigned long>(stack.size() - arg_count - 1);
    
    return true;
}

IResult VM::Interpret(const std::string& source)
{
    auto parser = Parser(source);
    auto opt = parser.Compile();
    if (!opt) { return IResult::COMPILE_ERROR; }

    auto& function = *opt;
    auto closure = std::make_shared<ClosureObject>(function);
    push(closure);
    call(closure, 0);

    return Run();
}

void VM::runtime_error(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    std::cerr << std::endl;
    
    for (auto i = frames.size(); i-- > 0; ) {
        auto& frame = frames[i];
        auto function = frame.closure->function;
        auto line = function->get_chunk().get_line(frame.ip - 1);
        std::cerr << "[line " << line << "] in ";
        if (function->name.empty()) {
            std::cerr << "script" << std::endl;
        } else {
            std::cerr << function->name << "()" << std::endl;
        }
    }

    ResetStack();
}

void VM::define_native(const std::string& name, NativeFn function)
{
    auto obj = std::make_shared<NativeFunctionObject>();
    obj->function = function;
    globals[name] = obj;
}

void VM::define_native_const(const std::string& name, Value value)
{
    globals[name] = value;
}

template <typename F>
bool VM::binary_op(F op)
{
    try {
        auto b = std::get<double>(peek(0));
        auto a = std::get<double>(peek(1));
        
        double_pop_and_push(op(a, b));
        return true;
    } catch (std::bad_variant_access&) {
        runtime_error("Operands must be numbers.");
        return false;
    }
}

void VM::double_pop_and_push(const Value& v)
{
    pop();
    pop();
    push(v);
}

IResult VM::Run()
{
    auto read_byte = [this]() -> uint8_t {
        return this->frames.back().closure->function->get_code(this->frames.back().ip++);
    };
    
    auto read_constant = [this, read_byte]() -> const Value& {
        return this->frames.back().closure->function->get_const(read_byte());
    };
    
    auto read_short = [this]() -> uint16_t {
        this->frames.back().ip += 2;
        return ((this->frames.back().closure->function->get_code(this->frames.back().ip - 2) << 8) | (this->frames.back().closure->function->get_code(this->frames.back().ip - 1)));
    };
    
    auto read_string = [read_constant]() -> const std::string& {
        return std::get<std::string>(read_constant());
    };
    
    while (true) {
#ifdef DEBUG_TRACE_EXECUTION
        std::cout << "          ";
        for (auto value: stack) {
            std::cout << "[ " << value << " ]";
        }
        std::cout << std::endl;
        
        frames.back().closure->function->get_chunk().disas_instruction(frames.back().ip);
#endif

#define BINARY_OP(op) \
    do { \
        if (!binary_op([](double a, double b) -> Value { return a op b; })) { \
            return IResult::RUNTIME_ERROR; \
        } \
    } while (false)
        
#define INTEGER_BINARY_OP(op) \
    do { \
        if (!binary_op([](int a, int b) -> Value { return static_cast<double>(a op b); })) { \
            return IResult::RUNTIME_ERROR; \
        } \
    } while (false)
        
        auto instruction = OpCode(read_byte());
        switch (instruction) {
            case OpCode::CONSTANT: {
                auto constant = read_constant();
                push(constant);
                break;
            }
            case OpCode::NULLOP: push(std::monostate()); break;
            case OpCode::TRUE: push(true); break;
            case OpCode::FALSE: push(false); break;
            case OpCode::POP: pop(); break;
                
            case OpCode::GET_LOCAL: {
                uint8_t slot = read_byte();
                push(stack[frames.back().stack_offset + slot]);
                break;
            }
                
            case OpCode::GET_GLOBAL: {
                auto name = read_string();
                auto found = globals.find(name);
                if (found == globals.end()) {
                    runtime_error("Undefined variable '%s'.", name.c_str());
                    return IResult::RUNTIME_ERROR;
                }
                auto value = found->second;
                push(value);
                break;
            }
                
            case OpCode::DEFINE_GLOBAL: {
                auto name = read_string();
                globals[name] = peek(0);
                pop();
                break;
            }
                
            case OpCode::SET_LOCAL: {
                uint8_t slot = read_byte();
                stack[frames.back().stack_offset + slot] = peek(0);
                break;
            }
                
            case OpCode::SET_GLOBAL: {
                auto name = read_string();
                auto found = globals.find(name);
                if (found == globals.end()) {
                    runtime_error("Undefined variable '%s'.", name.c_str());
                    return IResult::RUNTIME_ERROR;
                }
                found->second = peek(0);
                break;
            }
            case OpCode::GET_UPVALUE: {
                auto slot = read_byte();
                push(*frames.back().closure->upvalues[slot]->location);
                break;
            }
            case OpCode::SET_UPVALUE: {
                auto slot = read_byte();
                *frames.back().closure->upvalues[slot]->location = peek(0);
                break;
            }
            case OpCode::GET_PROPERTY: {
                InstanceValue instance;

                try {
                    instance = std::get<InstanceValue>(peek(0));
                } catch (std::bad_variant_access&) {
                    runtime_error("Only instances have properties.");
                    return IResult::RUNTIME_ERROR;
                }

                auto name = read_string();
                auto found = instance->fields.find(name);
                if (found != instance->fields.end()) {
                    auto value = found->second;
                    pop(); // Instance.
                    push(value);
                    break;
                }

                if (!bind_method(instance->class_value, name)) {
                    return IResult::RUNTIME_ERROR;
                }
                break;
            }
            case OpCode::SET_PROPERTY: {
                try {
                    auto instance = std::get<InstanceValue>(peek(1));
                    auto name = read_string();
                    instance->fields[name] = peek(0);
                    
                    auto value = pop();
                    pop();
                    push(value);
                } catch (std::bad_variant_access&) {
                    runtime_error("Only instances have fields.");
                    return IResult::RUNTIME_ERROR;
                }
                break;
            }
            case OpCode::GET_SUPER: {
                auto name = read_string();
                auto superclass = std::get<ClassValue>(pop());
                
                if (!bind_method(superclass, name)) {
                    return IResult::RUNTIME_ERROR;
                }
                break;
            }
            case OpCode::EQUAL: {
                double_pop_and_push(peek(0) == peek(1));
                break;
            }
                
            case OpCode::GREATER:   BINARY_OP(>); break;
            case OpCode::LESS:      BINARY_OP(<); break;
                
            case OpCode::ADD: {
                auto success = std::visit(overloaded {
                    [this](double b, double a) -> bool {
                        this->double_pop_and_push(a + b);
                        return true;
                    },
                    [this](std::string b, std::string a) -> bool {
                        this->double_pop_and_push(a + b);
                        return true;
                    },
					[this](std::string a, double b) -> bool {
						std::string bStr = std::to_string(b);
                        bStr.erase(bStr.find_last_not_of('0') + 1, std::string::npos);
                        bStr.erase(bStr.find_last_not_of('.') + 1, std::string::npos);
						this->double_pop_and_push(bStr + a);
						return true;
					},
					[this](double a, std::string b) -> bool {
						std::string aStr = std::to_string(a);
                        aStr.erase(aStr.find_last_not_of('0') + 1, std::string::npos);
                        aStr.erase(aStr.find_last_not_of('.') + 1, std::string::npos);
						this->double_pop_and_push(b + aStr);
						return true;
					},
                    [this](auto a, auto b) -> bool {
                        this->runtime_error("Operands must be two numbers or two strings.");
                        return false;
                    }
                }, peek(0), peek(1));
                
                if (!success)
                    return IResult::RUNTIME_ERROR;
                
                break;
            }
                
            case OpCode::SUBTRACT:  BINARY_OP(-); break;
            case OpCode::MULTIPLY:  BINARY_OP(*); break;
            case OpCode::DIVIDE:    BINARY_OP(/); break;
            case OpCode::BW_AND:    INTEGER_BINARY_OP(&); break;
            case OpCode::BW_OR:     INTEGER_BINARY_OP(|); break;
            case OpCode::BW_XOR:    INTEGER_BINARY_OP(^); break;
            case OpCode::NOT: push(is_false(pop())); break;
            
            case OpCode::BW_NOT: {
                try {
                    auto bwNotted = ~static_cast<int>(std::get<double>(peek(0)));
                    pop();
                    push(static_cast<double>(bwNotted));
                } catch (std::bad_variant_access) {
                    runtime_error("Operand must be a number.");
                    return IResult::RUNTIME_ERROR;
                }
            }
                
            case OpCode::NEGATE:
                try {
                    auto negated = -std::get<double>(peek(0));
                    pop(); // if we get here it means it was good
                    push(negated);
                } catch (std::bad_variant_access&) {
                    runtime_error("Operand must be a number.");
                    return IResult::RUNTIME_ERROR;
                }
                break;
                
            case OpCode::PRINT: {
                std::cout << pop() << std::endl;
                break;
            }
            
            case OpCode::JUMP: {
                auto offset = read_short();
                frames.back().ip += offset;
                break;
            }
                
            case OpCode::LOOP: {
                auto offset = read_short();
                frames.back().ip -= offset;
                break;
            }
                
            case OpCode::JUMP_IF_FALSE: {
                auto offset = read_short();
                if (is_false(peek(0))) {
                    frames.back().ip += offset;
                }
                break;
            }
                
            case OpCode::CALL: {
                int argCount = read_byte();
                if (!call_value(peek(argCount), argCount)) {
                    return IResult::RUNTIME_ERROR;
                }
                break;
            }
                
            case OpCode::INVOKE: {
                auto method = read_string();
                int argCount = read_byte();
                if (!invoke(method, argCount)) {
                    return IResult::RUNTIME_ERROR;
                }
                break;
            }
                
            case OpCode::SUPER_INVOKE: {
                auto method = read_string();
                int argCount = read_byte();
                auto superclass = std::get<ClassValue>(pop());
                if (!invoke_from_class(superclass, method, argCount)) {
                    return IResult::RUNTIME_ERROR;
                }
                break;
            }
                
            case OpCode::CLOSURE: {
                auto function = std::get<Func>(read_constant());
                auto closure = std::make_shared<ClosureObject>(function);
                push(closure);
                for (int i = 0; i < static_cast<int>(closure->upvalues.size()); i++) {
                    auto isLocal = read_byte();
                    auto index = read_byte();
                    if (isLocal) {
                        closure->upvalues[i] = capture_upvalue(&stack[frames.back().stack_offset + index]);
                    } else {
                        closure->upvalues[i] = frames.back().closure->upvalues[index];
                    }
                }
                break;
            }
            
            case OpCode::CLOSE_UPVALUE:
                close_upvalues(&stack.back());
                pop();
                break;
                
            case OpCode::RETURN: {
                auto result = pop();
                close_upvalues(&stack[frames.back().stack_offset]);
                
                auto lastOffset = frames.back().stack_offset;
                frames.pop_back();
                if (frames.empty()) {
                    pop();
                    return IResult::OK;
                }

                stack.resize(lastOffset);
                stack.reserve(STACK_MAX);
                push(result);
                break;
            }
                
            case OpCode::CLASS:
                push(std::make_shared<ClassObject>(read_string()));
                break;
                
            case OpCode::INHERIT: {
                try {
                    auto superclass = std::get<ClassValue>(peek(1));
                    auto subclass = std::get<ClassValue>(peek(0));
                    subclass->methods = superclass->methods;
                    pop(); // Subclass.
                    break;
                } catch (std::bad_variant_access&) {
                    runtime_error("Superclass must be a class.");
                    return IResult::RUNTIME_ERROR;
                }
            }
                
            case OpCode::METHOD:
                define_method(read_string());
                break;
        }
    }
    
    return IResult::OK;

#undef BINARY_OP
}
