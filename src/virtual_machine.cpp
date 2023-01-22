#include "virtual_machine.hpp"

#include <cstdarg>
#include <exception>

/**
 * @brief Creates a recursive list of overloads for the given visitor function.
 */
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

struct call_visitor_t {
    const int arg_count;
    virtual_machine_t& vm;
    
    call_visitor_t(int arg_count, virtual_machine_t& vm)
        : arg_count(arg_count), vm(vm) {}
    
    bool operator()(const native_function_t& native) const
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
    
    bool operator()(const closure_t& closure) const
    {
        return vm.call(closure, arg_count);
    }
    
    bool operator()(const class_value_t& class_value) const
    {
        // increment reference count to avoid premature deletion
        auto class_val = class_value;
        vm.stack[vm.stack.size() - arg_count - 1] = std::make_shared<instance_obj>(class_value);
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
    
    bool operator()(const member_func_value_t& bound) const
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

bool virtual_machine_t::call_value(const value_t& callee, int arg_count)
{
    return std::visit(call_visitor_t(arg_count, *this), callee);
}

bool virtual_machine_t::invoke(const std::string& name, int arg_count)
{
    try {
        auto instance = std::get<instance_value_t>(peek(arg_count));
        
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

bool virtual_machine_t::invoke_from_class(class_value_t class_value, const std::string& name, int arg_count)
{
    auto found = class_value->methods.find(name);
    if (found == class_value->methods.end()) {
        runtime_error("Undefined property '%s'.", name.c_str());
        return false;
    }
    auto method = found->second;
    return call(method, arg_count);
}

bool virtual_machine_t::bind_method(class_value_t class_value, const std::string& name)
{
    auto found = class_value->methods.find(name);
    if (found == class_value->methods.end()) {
        runtime_error("Undefined property '%s'.", name.c_str());
        return false;
    }
    auto method = found->second;
    auto instance = std::get<instance_value_t>(peek(0));
    auto bound = std::make_shared<member_func_obj>(instance, method);
    
    pop();
    push(bound);
    
    return true;
}

upvalue_value_t virtual_machine_t::capture_upvalue(value_t* local)
{
    upvalue_value_t prev_upvalue = nullptr;
    auto upvalue = open_upvalues;
    
    while (upvalue != nullptr && upvalue->location > local) {
        prev_upvalue = upvalue;
        upvalue = upvalue->next;
    }
    
    if (upvalue != nullptr && upvalue->location == local) {
        return upvalue;
    }
    
    auto new_upvalue = std::make_shared<upvalue_obj>(local);
    new_upvalue->next = upvalue;
    
    if (prev_upvalue == nullptr) {
        open_upvalues = new_upvalue;
    } else {
        prev_upvalue->next = new_upvalue;
    }
    
    return new_upvalue;
}

void virtual_machine_t::close_upvalues(value_t* last)
{
    while (open_upvalues != nullptr && open_upvalues->location >= last){
        auto& upvalue = open_upvalues;
        upvalue->closed = *upvalue->location;
        upvalue->location = &upvalue->closed;
        open_upvalues = upvalue->next;
    }
}

void virtual_machine_t::define_method(const std::string& name)
{
    auto method = std::get<closure_t>(peek(0));
    auto class_value = std::get<class_value_t>(peek(1));
    class_value->methods[name] = method;
    pop();
}

bool virtual_machine_t::call(const closure_t& closure, int arg_count)
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

    frames.emplace_back(call_frame_t());
    auto& frame = frames.back();
    frame.ip = 0;
    frame.closure = closure;
    frame.stack_offset = static_cast<unsigned long>(stack.size() - arg_count - 1);
    
    return true;
}

interpret_result_t virtual_machine_t::Interpret(const std::string& source)
{
    auto parser = parser_t(source);
    auto opt = parser.compile();
    if (!opt) { return interpret_result_t::COMPILE_ERROR; }

    auto& function = *opt;
    auto closure = std::make_shared<closure_obj>(function);
    push(closure);
    call(closure, 0);

    return Run();
}

void virtual_machine_t::runtime_error(const char* format, ...)
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

void virtual_machine_t::define_native(const std::string& name, native_fn_t function)
{
    auto obj = std::make_shared<native_func_obj>();
    obj->function = function;
    globals[name] = obj;
}

void virtual_machine_t::define_native_const(const std::string& name, value_t value)
{
    globals[name] = value;
}

template <typename F>
bool virtual_machine_t::binary_op(F op)
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

void virtual_machine_t::double_pop_and_push(const value_t& v)
{
    pop();
    pop();
    push(v);
}

interpret_result_t virtual_machine_t::Run()
{
    auto read_byte = [this]() -> uint8_t {
        return this->frames.back().closure->function->get_code(this->frames.back().ip++);
    };
    
    auto read_constant = [this, read_byte]() -> const value_t& {
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
        if (!binary_op([](double a, double b) -> value_t { return a op b; })) { \
            return interpret_result_t::RUNTIME_ERROR; \
        } \
    } while (false)
        
#define INTEGER_BINARY_OP(op) \
    do { \
        if (!binary_op([](int a, int b) -> value_t { return static_cast<double>(a op b); })) { \
            return interpret_result_t::RUNTIME_ERROR; \
        } \
    } while (false)
        
        auto instruction = opcode_t(read_byte());
        switch (instruction) {
            case opcode_t::CONSTANT: {
                auto constant = read_constant();
                push(constant);
                break;
            }
            case opcode_t::NULLOP: push(std::monostate()); break;
            case opcode_t::TRUE: push(true); break;
            case opcode_t::FALSE: push(false); break;
            case opcode_t::POP: pop(); break;
                
            case opcode_t::GET_LOCAL: {
                uint8_t slot = read_byte();
                push(stack[frames.back().stack_offset + slot]);
                break;
            }
                
            case opcode_t::GET_GLOBAL: {
                auto name = read_string();
                auto found = globals.find(name);
                if (found == globals.end()) {
                    runtime_error("Undefined variable '%s'.", name.c_str());
                    return interpret_result_t::RUNTIME_ERROR;
                }
                auto value = found->second;
                push(value);
                break;
            }
                
            case opcode_t::DEFINE_GLOBAL: {
                auto name = read_string();
                globals[name] = peek(0);
                pop();
                break;
            }
                
            case opcode_t::SET_LOCAL: {
                uint8_t slot = read_byte();
                stack[frames.back().stack_offset + slot] = peek(0);
                break;
            }
                
            case opcode_t::SET_GLOBAL: {
                auto name = read_string();
                auto found = globals.find(name);
                if (found == globals.end()) {
                    runtime_error("Undefined variable '%s'.", name.c_str());
                    return interpret_result_t::RUNTIME_ERROR;
                }
                found->second = peek(0);
                break;
            }
            case opcode_t::GET_UPVALUE: {
                auto slot = read_byte();
                push(*frames.back().closure->upvalues[slot]->location);
                break;
            }
            case opcode_t::SET_UPVALUE: {
                auto slot = read_byte();
                *frames.back().closure->upvalues[slot]->location = peek(0);
                break;
            }
            case opcode_t::GET_PROPERTY: {
                instance_value_t instance;

                try {
                    instance = std::get<instance_value_t>(peek(0));
                } catch (std::bad_variant_access&) {
                    runtime_error("Only instances have properties.");
                    return interpret_result_t::RUNTIME_ERROR;
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
                    return interpret_result_t::RUNTIME_ERROR;
                }
                break;
            }
            case opcode_t::SET_PROPERTY: {
                try {
                    auto instance = std::get<instance_value_t>(peek(1));
                    auto name = read_string();
                    instance->fields[name] = peek(0);
                    
                    auto value = pop();
                    pop();
                    push(value);
                } catch (std::bad_variant_access&) {
                    runtime_error("Only instances have fields.");
                    return interpret_result_t::RUNTIME_ERROR;
                }
                break;
            }
            case opcode_t::GET_SUPER: {
                auto name = read_string();
                auto superclass = std::get<class_value_t>(pop());
                
                if (!bind_method(superclass, name)) {
                    return interpret_result_t::RUNTIME_ERROR;
                }
                break;
            }
            case opcode_t::EQUAL: {
                double_pop_and_push(peek(0) == peek(1));
                break;
            }
                
            case opcode_t::GREATER:   BINARY_OP(>); break;
            case opcode_t::LESS:      BINARY_OP(<); break;
                
            case opcode_t::ADD: {
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
                    return interpret_result_t::RUNTIME_ERROR;
                
                break;
            }
                
            case opcode_t::SUBTRACT:  BINARY_OP(-); break;
            case opcode_t::MULTIPLY:  BINARY_OP(*); break;
            case opcode_t::DIVIDE:    BINARY_OP(/); break;
            case opcode_t::BW_AND:    INTEGER_BINARY_OP(&); break;
            case opcode_t::BW_OR:     INTEGER_BINARY_OP(|); break;
            case opcode_t::BW_XOR:    INTEGER_BINARY_OP(^); break;
            case opcode_t::NOT: push(is_false(pop())); break;
            
            case opcode_t::BW_NOT: {
                try {
                    auto bw_notted = ~static_cast<int>(std::get<double>(peek(0)));
                    pop();
                    push(static_cast<double>(bw_notted));
                } catch (std::bad_variant_access&) {
                    runtime_error("Operand must be a number.");
                    return interpret_result_t::RUNTIME_ERROR;
                }
            }
                
            case opcode_t::NEGATE:
                try {
                    auto negated = -std::get<double>(peek(0));
                    pop(); // if we get here it means it was good
                    push(negated);
                } catch (std::bad_variant_access&) {
                    runtime_error("Operand must be a number.");
                    return interpret_result_t::RUNTIME_ERROR;
                }
                break;
                
            case opcode_t::PRINT: {
                std::cout << pop() << std::endl;
                break;
            }
            
            case opcode_t::JUMP: {
                auto offset = read_short();
                frames.back().ip += offset;
                break;
            }
                
            case opcode_t::LOOP: {
                auto offset = read_short();
                frames.back().ip -= offset;
                break;
            }
                
            case opcode_t::JUMP_IF_FALSE: {
                auto offset = read_short();
                if (is_false(peek(0))) {
                    frames.back().ip += offset;
                }
                break;
            }
                
            case opcode_t::CALL: {
                int argCount = read_byte();
                if (!call_value(peek(argCount), argCount)) {
                    return interpret_result_t::RUNTIME_ERROR;
                }
                break;
            }
                
            case opcode_t::INVOKE: {
                auto method = read_string();
                int argCount = read_byte();
                if (!invoke(method, argCount)) {
                    return interpret_result_t::RUNTIME_ERROR;
                }
                break;
            }
                
            case opcode_t::SUPER_INVOKE: {
                auto method = read_string();
                int argCount = read_byte();
                auto superclass = std::get<class_value_t>(pop());
                if (!invoke_from_class(superclass, method, argCount)) {
                    return interpret_result_t::RUNTIME_ERROR;
                }
                break;
            }
                
            case opcode_t::CLOSURE: {
                auto function = std::get<func_t>(read_constant());
                auto closure = std::make_shared<closure_obj>(function);
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
            
            case opcode_t::CLOSE_UPVALUE:
                close_upvalues(&stack.back());
                pop();
                break;
                
            case opcode_t::RETURN: {
                auto result = pop();
                close_upvalues(&stack[frames.back().stack_offset]);
                
                auto lastOffset = frames.back().stack_offset;
                frames.pop_back();
                if (frames.empty()) {
                    pop();
                    return interpret_result_t::OK;
                }

                stack.resize(lastOffset);
                stack.reserve(STACK_MAX);
                push(result);
                break;
            }

            case opcode_t::ARR_BUILD: {
                array_t new_arr = std::make_shared<array_obj>();
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

            case opcode_t::ARR_INDEX: {
                try {
                    auto index = static_cast<size_t>(std::get<double>(pop()));
                    array_t list;

                    try {
                        list = std::get<array_t>(pop());
                    }
                    catch (std::bad_variant_access&) {
                        runtime_error("Object is not an array");
                        return interpret_result_t::RUNTIME_ERROR;
                    }

                    if (index < 0 || index >= list->values.size()) {
                        runtime_error("Array index out of bounds");
                        return interpret_result_t::RUNTIME_ERROR;
                    }

                    auto& result = list->values[index];
                    push(result);

                } catch (std::bad_variant_access&) {
                    runtime_error("Index is not a number");
                    return interpret_result_t::RUNTIME_ERROR;
                }

                break;
            }

            case opcode_t::ARR_STORE: {
                try {
                    auto item = pop();
                    auto index = static_cast<size_t>(std::get<double>(pop()));
                    array_t list;

                    try {
                        list = std::get<array_t>(pop());
                    }
                    catch (std::bad_variant_access&) {
                        runtime_error("Object is not an array");
                        return interpret_result_t::RUNTIME_ERROR;
                    }

                    if (index < 0 || index >= list->values.size()) {
                        runtime_error("Array index out of bounds");
                        return interpret_result_t::RUNTIME_ERROR;
                    }

                    list->values[index] = item;

                    push(item);
                }
                catch (std::bad_variant_access&) {
                    runtime_error("Index is not a number");
                    return interpret_result_t::RUNTIME_ERROR;
                }

                break;
            }
                
            case opcode_t::CLASS:
                push(std::make_shared<class_obj>(read_string()));
                break;
                
            case opcode_t::INHERIT: {
                try {
                    auto& superclass = std::get<class_value_t>(peek(1));
                    auto& subclass = std::get<class_value_t>(peek(0));
                    subclass->methods = superclass->methods;
                    pop(); // Subclass.
                    break;
                } catch (std::bad_variant_access&) {
                    runtime_error("Superclass must be a class.");
                    return interpret_result_t::RUNTIME_ERROR;
                }
            }
                
            case opcode_t::METHOD:
                define_method(read_string());
                break;
        }
    }
    
    return interpret_result_t::OK;

#undef BINARY_OP
}
