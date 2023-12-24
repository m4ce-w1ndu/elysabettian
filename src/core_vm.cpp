#include "runtime/core_vm.h"

#include <cstdarg>
#include <exception>

/**
 * @brief Recursively creates a list of overloads for the operator() using
 * variadic templates. This is useful to seamlessly implement the visitor
 * pattern.
 * @tparam ...Ts List of variadic types (auto-deducted).
*/
template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };

/**
 * @brief Applies the overloaded struct (callable object) to a given
 * type.
*/
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

[[maybe_unused]] const std::string operator+(const std::string& str, double num)
{
    std::string str_num = std::to_string(num);
    return str + str_num;
}

[[maybe_unused]] static const std::string operator+(double num, const std::string& str)
{
    std::string str_num = std::to_string(num);
    return str_num + str;
}

struct CallVisitor {
    const int arg_count;
    VirtualMachine& vm;
    
    CallVisitor(int arg_count, VirtualMachine& vm)
        : arg_count(arg_count), vm(vm) {}
    
    bool operator()(const NativeFunc& native) const
    {
        Value result = native->function(arg_count, vm.stack.end() - arg_count);
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
    
    bool operator()(const Class& class_value) const
    {
        // increment reference count to avoid premature deletion
        Class class_val = class_value;

        vm.stack[vm.stack.size() - arg_count - 1] = std::make_shared<InstanceObj>(class_value);
        try {
            std::unordered_map<std::string, Closure>& funcs = class_val->methods;
            Closure found = funcs.at(vm.init_string);
            return vm.call(found, arg_count);
        }
        catch (std::out_of_range&) {
            vm.runtime_error("Expected 0 arguments but got %d.", arg_count);
            return false;
        }
        return true;
    }
    
    bool operator()(const MemberFunc& bound) const
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

bool VirtualMachine::call_value(const Value& callee, int arg_count)
{
    return std::visit(CallVisitor(arg_count, *this), callee);
}

bool VirtualMachine::invoke(const std::string& name, int arg_count)
{
    try {
        Instance instance = std::get<Instance>(peek(arg_count));
        
        auto found = instance->fields.find(name);
        if (found != instance->fields.end()) {
            Value value = found->second;
            stack[stack.size() - arg_count - 1] = value;
            return call_value(value, arg_count);
        }
        
        return invoke_from_class(instance->class_value, name, arg_count);
    } catch (std::bad_variant_access&) {
        runtime_error("Only instances have methods.");
        return false;
    }
}

bool VirtualMachine::invoke_from_class(Class class_value, const std::string& name, int arg_count)
{
    auto found = class_value->methods.find(name);
    if (found == class_value->methods.end()) {
        runtime_error("Undefined property '%s'.", name.c_str());
        return false;
    }
    Closure method = found->second;
    return call(method, arg_count);
}

bool VirtualMachine::bind_method(Class class_value, const std::string& name)
{
    auto found = class_value->methods.find(name);
    if (found == class_value->methods.end()) {
        runtime_error("Undefined property '%s'.", name.c_str());
        return false;
    }
    Closure method = found->second;
    Instance instance = std::get<Instance>(peek(0));
    MemberFunc bound = std::make_shared<MemberFuncObj>(instance, method);
    
    pop();
    push(bound);
    
    return true;
}

Upvalue VirtualMachine::capture_upvalue(Value* local)
{
    Upvalue prev_upvalue = nullptr;
    Upvalue upvalue = open_upvalues;
    
    while (upvalue != nullptr && upvalue->location > local) {
        prev_upvalue = upvalue;
        upvalue = upvalue->next;
    }
    
    if (upvalue != nullptr && upvalue->location == local) {
        return upvalue;
    }
    
    Upvalue new_upvalue = std::make_shared<UpvalueObj>(local);
    new_upvalue->next = upvalue;
    
    if (prev_upvalue == nullptr) {
        open_upvalues = new_upvalue;
    } else {
        prev_upvalue->next = new_upvalue;
    }
    
    return new_upvalue;
}

void VirtualMachine::close_upvalues(Value* last)
{
    while (open_upvalues != nullptr && open_upvalues->location >= last){
        Upvalue& upvalue = open_upvalues;
        upvalue->closed = *upvalue->location;
        upvalue->location = &upvalue->closed;
        open_upvalues = upvalue->next;
    }
}

void VirtualMachine::define_method(const std::string& name)
{
    Closure method = std::get<Closure>(peek(0));
    Class class_value = std::get<Class>(peek(1));
    class_value->methods[name] = method;
    pop();
}

bool VirtualMachine::call(const Closure& closure, int arg_count)
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
    CallFrame& frame = frames.back();
    frame.ip = 0;
    frame.closure = closure;
    frame.stack_offset = static_cast<unsigned long>(stack.size() - arg_count - 1);
    
    return true;
}

InterpretResult VirtualMachine::interpret(const std::string& source)
{
    Parser parser = Parser(source);
    std::optional<Func> opt = parser.compile();
    if (!opt) { return InterpretResult::CompileError; }

    Func& function = *opt;
    Closure closure = std::make_shared<ClosureObj>(function);
    push(closure);
    call(closure, 0);

    return run();
}

void VirtualMachine::runtime_error(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    std::cerr << std::endl;
    
    for (auto i = frames.size(); i-- > 0; ) {
        CallFrame& frame = frames[i];
        Func function = frame.closure->function;
        int line = function->get_chunk().get_line(frame.ip - 1);

        std::cerr << "[line " << line << "] in ";
        if (function->name.empty()) {
            std::cerr << "script" << std::endl;
        } else {
            std::cerr << function->name << "()" << std::endl;
        }
    }

    reset_stack();
}

void VirtualMachine::define_native(const std::string& name, NativeFn function)
{
    NativeFunc obj = std::make_shared<NativeFuncObj>();
    obj->function = function;
    globals[name] = obj;
}

void VirtualMachine::define_native_const(const std::string& name, Value value)
{
    globals[name] = value;
}

template <typename F>
bool VirtualMachine::binary_op(F op)
{
    try {
        const double& b = std::get<double>(peek(0));
        const double& a = std::get<double>(peek(1));
        
        double_pop_and_push(op(a, b));
        return true;
    } catch (std::bad_variant_access&) {
        runtime_error("Operands must be numbers.");
        return false;
    }
}

void VirtualMachine::double_pop_and_push(const Value& v)
{
    pop();
    pop();
    push(v);
}

InterpretResult VirtualMachine::run()
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
            return InterpretResult::RuntimeError; \
        } \
    } while (false)
        
#define INTEGER_BINARY_OP(op) \
    do { \
        if (!binary_op([](int a, int b) -> Value { return static_cast<double>(a op b); })) { \
            return InterpretResult::RuntimeError; \
        } \
    } while (false)
        
        Opcode instruction = Opcode(read_byte());
        switch (instruction) {
            case Opcode::Constant: {
                Value constant = read_constant();
                push(constant);
                break;
            }
            case Opcode::Nop:   push(std::monostate()); break;
            case Opcode::True:  push(true); break;
            case Opcode::False: push(false); break;
            case Opcode::Pop:   pop(); break;
                
            case Opcode::GetLocal: {
                uint8_t slot = read_byte();
                push(stack[frames.back().stack_offset + slot]);
                break;
            }
                
            case Opcode::GetGlobal: {
                const std::string& name = read_string();
                auto found = globals.find(name);
                if (found == globals.end()) {
                    runtime_error("Undefined variable '%s'.", name.c_str());
                    return InterpretResult::RuntimeError;
                }
                Value value = found->second;
                push(value);
                break;
            }
                
            case Opcode::DefineGlobal: {
                const std::string& name = read_string();
                globals[name] = peek(0);
                pop();
                break;
            }
                
            case Opcode::SetLocal: {
                uint8_t slot = read_byte();
                stack[frames.back().stack_offset + slot] = peek(0);
                break;
            }
                
            case Opcode::SetGlobal: {
                const std::string& name = read_string();
                auto found = globals.find(name);
                if (found == globals.end()) {
                    runtime_error("Undefined variable '%s'.", name.c_str());
                    return InterpretResult::RuntimeError;
                }
                found->second = peek(0);
                break;
            }
            case Opcode::GetUpvalue: {
                uint8_t slot = read_byte();
                push(*frames.back().closure->upvalues[slot]->location);
                break;
            }
            case Opcode::SetUpvalue: {
                uint8_t slot = read_byte();
                *frames.back().closure->upvalues[slot]->location = peek(0);
                break;
            }
            case Opcode::GetProperty: {
                Instance instance;

                try {
                    instance = std::get<Instance>(peek(0));
                } catch (std::bad_variant_access&) {
                    runtime_error("Only instances have properties.");
                    return InterpretResult::RuntimeError;
                }

                const std::string& name = read_string();
                auto found = instance->fields.find(name);

                if (found != instance->fields.end()) {
                    Value value = found->second;
                    pop(); // Instance.
                    push(value);
                    break;
                }

                if (!bind_method(instance->class_value, name)) {
                    return InterpretResult::RuntimeError;
                }
                break;
            }
            case Opcode::SetProperty: {
                try {
                    Instance instance = std::get<Instance>(peek(1));
                    const std::string& name = read_string();
                    instance->fields[name] = peek(0);
                    
                    auto value = pop();
                    pop();
                    push(value);
                } catch (std::bad_variant_access&) {
                    runtime_error("Only instances have fields.");
                    return InterpretResult::RuntimeError;
                }
                break;
            }
            case Opcode::GetSuper: {
                const std::string& name = read_string();
                Class superclass = std::get<Class>(pop());
                
                if (!bind_method(superclass, name)) {
                    return InterpretResult::RuntimeError;
                }
                break;
            }
            case Opcode::Equal: {
                double_pop_and_push(peek(0) == peek(1));
                break;
            }
                
            case Opcode::Greater:   BINARY_OP(>); break;
            case Opcode::Less:      BINARY_OP(<); break;
                
            case Opcode::Add: {
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
						std::string bstr = std::to_string(b);
                        bstr.erase(bstr.find_last_not_of('0') + 1, std::string::npos);
                        bstr.erase(bstr.find_last_not_of('.') + 1, std::string::npos);
						this->double_pop_and_push(bstr + a);
						return true;
					},
					[this](double a, std::string b) -> bool {
						std::string astr = std::to_string(a);
                        astr.erase(astr.find_last_not_of('0') + 1, std::string::npos);
                        astr.erase(astr.find_last_not_of('.') + 1, std::string::npos);
						this->double_pop_and_push(b + astr);
						return true;
					},
                    [this](auto a, auto b) -> bool {
                        this->runtime_error("Operands must be two numbers or two strings.");
                        return false;
                    }
                }, peek(0), peek(1));
                
                if (!success)
                    return InterpretResult::RuntimeError;
                
                break;
            }
                
            case Opcode::Subtract:  BINARY_OP(-); break;
            case Opcode::Multiply:  BINARY_OP(*); break;
            case Opcode::Divide:    BINARY_OP(/); break;
            case Opcode::BwAnd:    INTEGER_BINARY_OP(&); break;
            case Opcode::BwOr:     INTEGER_BINARY_OP(|); break;
            case Opcode::BwXor:    INTEGER_BINARY_OP(^); break;
            case Opcode::Not: push(is_false(pop())); break;
            
            case Opcode::BwNot: {
                try {
                    int bw_notted = ~static_cast<int>(std::get<double>(peek(0)));
                    pop();
                    push(static_cast<double>(bw_notted));
                } catch (std::bad_variant_access&) {
                    runtime_error("Operand must be a number.");
                    return InterpretResult::RuntimeError;
                }
                break;
            }
                
            case Opcode::Negate:
                try {
                    double negated = -std::get<double>(peek(0));
                    pop(); // if we get here it means it was good
                    push(negated);
                } catch (std::bad_variant_access&) {
                    runtime_error("Operand must be a number.");
                    return InterpretResult::RuntimeError;
                }
                break;
                
            case Opcode::Print: {
                std::cout << pop() << std::endl;
                break;
            }
            
            case Opcode::Jump: {
                uint16_t offset = read_short();
                frames.back().ip += offset;
                break;
            }
                
            case Opcode::Loop: {
                uint16_t offset = read_short();
                frames.back().ip -= offset;
                break;
            }
                
            case Opcode::JumpIfFalse: {
                uint16_t offset = read_short();
                if (is_false(peek(0))) {
                    frames.back().ip += offset;
                }
                break;
            }
                
            case Opcode::Call: {
                int arg_count = read_byte();
                if (!call_value(peek(arg_count), arg_count)) {
                    return InterpretResult::RuntimeError;
                }
                break;
            }
                
            case Opcode::Invoke: {
                const std::string& method = read_string();
                int arg_count = read_byte();
                if (!invoke(method, arg_count)) {
                    return InterpretResult::RuntimeError;
                }
                break;
            }
                
            case Opcode::SuperInvoke: {
                const std::string& method = read_string();
                int arg_count = read_byte();
                Class superclass = std::get<Class>(pop());
                if (!invoke_from_class(superclass, method, arg_count)) {
                    return InterpretResult::RuntimeError;
                }
                break;
            }
                
            case Opcode::Closure: {
                Func function = std::get<Func>(read_constant());
                Closure closure = std::make_shared<ClosureObj>(function);
                push(closure);
                for (int i = 0; i < static_cast<int>(closure->upvalues.size()); i++) {
                    auto is_local = read_byte();
                    auto index = read_byte();
                    if (is_local) {
                        closure->upvalues[i] = capture_upvalue(&stack[frames.back().stack_offset + index]);
                    } else {
                        closure->upvalues[i] = frames.back().closure->upvalues[index];
                    }
                }
                break;
            }
            
            case Opcode::CloseUpvalue:
                close_upvalues(&stack.back());
                pop();
                break;
                
            case Opcode::Return: {
                Value result = pop();
                close_upvalues(&stack[frames.back().stack_offset]);
                
                size_t last_offset = frames.back().stack_offset;
                frames.pop_back();
                if (frames.empty()) {
                    pop();
                    return InterpretResult::Ok;
                }

                stack.resize(last_offset);
                stack.reserve(STACK_MAX);
                push(result);
                break;
            }

            case Opcode::ArrBuild: {
                Array new_arr = std::make_shared<ArrayObj>();
                uint8_t item_count = read_byte();

				push(new_arr);
				for (int i = item_count; i > 0; i--) {
					new_arr->values.push_back(peek(i));
            	}
				pop();

				while (item_count-- > 0) {
					pop();
				}

				push(new_arr);
				break;
			}

            case Opcode::ArrIndex: {
                try {
                    size_t index = static_cast<size_t>(std::get<double>(pop()));
                    Array list;

                    try {
                        list = std::get<Array>(pop());
                    }
                    catch (std::bad_variant_access&) {
                        runtime_error("Object is not an array");
                        return InterpretResult::RuntimeError;
                    }

                    if (index < 0 || index >= list->values.size()) {
                        runtime_error("Array index out of bounds");
                        return InterpretResult::RuntimeError;
                    }

                    Value& result = list->values[index];
                    push(result);

                } catch (std::bad_variant_access&) {
                    runtime_error("Index is not a number");
                    return InterpretResult::RuntimeError;
                }

                break;
            }

            case Opcode::ArrStore: {
                try {
                    Value item = pop();
                    size_t index = static_cast<size_t>(std::get<double>(pop()));
                    Array list;

                    try {
                        list = std::get<Array>(pop());
                    }
                    catch (std::bad_variant_access&) {
                        runtime_error("Object is not an array");
                        return InterpretResult::RuntimeError;
                    }

                    if (index < 0 || index >= list->values.size()) {
                        runtime_error("Array index out of bounds");
                        return InterpretResult::RuntimeError;
                    }

                    list->values[index] = item;

                    push(item);
                }
                catch (std::bad_variant_access&) {
                    runtime_error("Index is not a number");
                    return InterpretResult::RuntimeError;
                }

                break;
            }
                
            case Opcode::Class:
                push(std::make_shared<ClassObj>(read_string()));
                break;
                
            case Opcode::Inherit: {
                try {
                    const Class& superclass = std::get<Class>(peek(1));
                    const Class& subclass = std::get<Class>(peek(0));
                    subclass->methods = superclass->methods;
                    pop(); // Subclass.
                    break;
                } catch (std::bad_variant_access&) {
                    runtime_error("Superclass must be a class.");
                    return InterpretResult::RuntimeError;
                }
            }
                
            case Opcode::Method:
                define_method(read_string());
                break;
        }
    }
    
    return InterpretResult::Ok;

#undef BINARY_OP
}
