//
//  VirtualMachine.cpp
//  CElysabettian
//
//  Created by Simone Rolando on 11/07/21.
//

#include "VirtualMachine.hpp"

template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

struct CallVisitor {
    const int argCount;
    VM& vm;
    
    CallVisitor(int argCount, VM& vm)
        : argCount(argCount), vm(vm) {}
    
    bool operator()(const NativeFunction& native) const
    {
        auto result = native->function(argCount, vm.stack.end() - argCount);
        try {
            vm.stack.resize(vm.stack.size() - argCount - 1);
            vm.stack.reserve(STACK_MAX);
            vm.Push(std::move(result));
        } catch (std::length_error) {
            return false;
        }
        return true;
    }
    
    bool operator()(const Closure& closure) const
    {
        return vm.Call(closure, argCount);
    }
    
    bool operator()(const ClassValue& klass) const
    {
        vm.stack[vm.stack.size() - argCount - 1] = std::make_shared<InstanceObject>(klass);
        auto found = klass->memberFuncs.find(vm.initString);
        if (found != klass->memberFuncs.end()) {
            auto initializer = found->second;
            return vm.Call(initializer, argCount);
        } else if (argCount != 0) {
            vm.RuntimeError("Expected 0 arguments but got %d.", argCount);
            return false;
        }
        return true;
    }
    
    bool operator()(const BoundMethodValue& bound) const
    {
        vm.stack[vm.stack.size() - argCount - 1] = bound->receiver;
        return vm.Call(bound->memberFunc, argCount);
    }

    template <typename T>
    bool operator()(const T& value) const
    {
        vm.RuntimeError("Can only call functions and classes.");
        return false;
    }
};

bool VM::CallValue(const Value& callee, int argCount)
{
    return std::visit(CallVisitor(argCount, *this), callee);
}

bool VM::Invoke(const std::string& name, int argCount)
{
    try {
        auto instance = std::get<InstanceValue>(Peek(argCount));
        
        auto found = instance->fields.find(name);
        if (found != instance->fields.end()) {
            auto value = found->second;
            stack[stack.size() - argCount - 1] = value;
            return CallValue(value, argCount);
        }
        
        return InvokeFromClass(instance->classValue, name, argCount);
    } catch (std::bad_variant_access&) {
        RuntimeError("Only instances have methods.");
        return false;
    }
}

bool VM::InvokeFromClass(ClassValue klass, const std::string& name, int argCount)
{
    auto found = klass->memberFuncs.find(name);
    if (found == klass->memberFuncs.end()) {
        RuntimeError("Undefined property '%s'.", name.c_str());
        return false;
    }
    auto method = found->second;
    return Call(method, argCount);
}

bool VM::BindMethod(ClassValue klass, const std::string& name)
{
    auto found = klass->memberFuncs.find(name);
    if (found == klass->memberFuncs.end()) {
        RuntimeError("Undefined property '%s'.", name.c_str());
        return false;
    }
    auto method = found->second;
    auto instance = std::get<InstanceValue>(Peek(0));
    auto bound = std::make_shared<MemberFuncObject>(instance, method);
    
    Pop();
    Push(bound);
    
    return true;
}

UpvalueValue VM::CaptureUpvalue(Value* local)
{
    UpvalueValue prevUpvalue = nullptr;
    auto upvalue = openUpvalues;
    
    while (upvalue != nullptr && upvalue->location > local) {
        prevUpvalue = upvalue;
        upvalue = upvalue->next;
    }
    
    if (upvalue != nullptr && upvalue->location == local) {
        return upvalue;
    }
    
    auto createdUpvalue = std::make_shared<UpvalueObject>(local);
    createdUpvalue->next = upvalue;
    
    if (prevUpvalue == nullptr) {
        openUpvalues = createdUpvalue;
    } else {
        prevUpvalue->next = createdUpvalue;
    }
    
    return createdUpvalue;
}

void VM::CloseUpvalues(Value* last)
{
    while (openUpvalues != nullptr && openUpvalues->location >= last){
        auto& upvalue = openUpvalues;
        upvalue->closed = *upvalue->location;
        upvalue->location = &upvalue->closed;
        openUpvalues = upvalue->next;
    }
}

void VM::DefineMethod(const std::string& name)
{
    auto method = std::get<Closure>(Peek(0));
    auto klass = std::get<ClassValue>(Peek(1));
    klass->memberFuncs[name] = method;
    Pop();
}

bool VM::Call(const Closure& closure, int argCount)
{
    if (argCount != closure->function->arity) {
        RuntimeError("Expected %d arguments but got %d.",
                     closure->function->arity, argCount);
        return false;
    }
    
    if (frames.size() + 1 == FRAMES_MAX) {
        RuntimeError("Stack overflow.");
        return false;
    }

    frames.emplace_back(CallFrame());
    auto& frame = frames.back();
    frame.ip = 0;
    frame.closure = closure;
    frame.stackOffset = stack.size() - argCount - 1;
    
    return true;
}

IResult VM::Interpret(const std::string& source)
{
    auto parser = Parser(source);
    auto opt = parser.Compile();
    if (!opt) { return IResult::COMPILE_ERROR; }

    auto& function = *opt;
    auto closure = std::make_shared<ClosureObject>(function);
    Push(closure);
    Call(closure, 0);

    return Run();
}

void VM::RuntimeError(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    std::cerr << std::endl;
    
    for (auto i = frames.size(); i-- > 0; ) {
        auto& frame = frames[i];
        auto function = frame.closure->function;
        auto line = function->GetChunk().GetLine(frame.ip - 1);
        std::cerr << "[line " << line << "] in ";
        if (function->name.empty()) {
            std::cerr << "script" << std::endl;
        } else {
            std::cerr << function->name << "()" << std::endl;
        }
    }

    ResetStack();
}

void VM::DefineNative(const std::string& name, NativeFn function)
{
    auto obj = std::make_shared<NativeFunctionObject>();
    obj->function = function;
    globals[name] = obj;
}

template <typename F>
bool VM::BinaryOp(F op)
{
    try {
        auto b = std::get<double>(Peek(0));
        auto a = std::get<double>(Peek(1));
        
        DoublePopAndPush(op(a, b));
        return true;
    } catch (std::bad_variant_access&) {
        RuntimeError("Operands must be numbers.");
        return false;
    }
}

void VM::DoublePopAndPush(const Value& v)
{
    Pop();
    Pop();
    Push(v);
}

IResult VM::Run()
{
    auto readByte = [this]() -> uint8_t {
        return this->frames.back().closure->function->GetCode(this->frames.back().ip++);
    };
    
    auto readConstant = [this, readByte]() -> const Value& {
        return this->frames.back().closure->function->GetConst(readByte());
    };
    
    auto readShort = [this]() -> uint16_t {
        this->frames.back().ip += 2;
        return ((this->frames.back().closure->function->GetCode(this->frames.back().ip - 2) << 8) | (this->frames.back().closure->function->GetCode(this->frames.back().ip - 1)));
    };
    
    auto readString = [readConstant]() -> const std::string& {
        return std::get<std::string>(readConstant());
    };
    
    while (true) {
#ifdef DEBUG_TRACE_EXECUTION
        std::cout << "          ";
        for (auto value: stack) {
            std::cout << "[ " << value << " ]";
        }
        std::cout << std::endl;
        
        frames.back().closure->function->GetChunk().DisasIntruction(frames.back().ip);
#endif

#define BINARY_OP(op) \
    do { \
        if (!BinaryOp([](double a, double b) -> Value { return a op b; })) { \
            return IResult::RUNTIME_ERROR; \
        } \
    } while (false)
        
#define INTEGER_BINARY_OP(op) \
    do { \
        if (!BinaryOp([](int a, int b) -> Value { return static_cast<double>(a op b); })) { \
            return IResult::RUNTIME_ERROR; \
        } \
    } while (false)
        
        auto instruction = OpCode(readByte());
        switch (instruction) {
            case OpCode::CONSTANT: {
                auto constant = readConstant();
                Push(constant);
                break;
            }
            case OpCode::NULLOP: Push(std::monostate()); break;
            case OpCode::TRUE: Push(true); break;
            case OpCode::FALSE: Push(false); break;
            case OpCode::POP: Pop(); break;
                
            case OpCode::GET_LOCAL: {
                uint8_t slot = readByte();
                Push(stack[frames.back().stackOffset + slot]);
                break;
            }
                
            case OpCode::GET_GLOBAL: {
                auto name = readString();
                auto found = globals.find(name);
                if (found == globals.end()) {
                    RuntimeError("Undefined variable '%s'.", name.c_str());
                    return IResult::RUNTIME_ERROR;
                }
                auto value = found->second;
                Push(value);
                break;
            }
                
            case OpCode::DEFINE_GLOBAL: {
                auto name = readString();
                globals[name] = Peek(0);
                Pop();
                break;
            }
                
            case OpCode::SET_LOCAL: {
                uint8_t slot = readByte();
                stack[frames.back().stackOffset + slot] = Peek(0);
                break;
            }
                
            case OpCode::SET_GLOBAL: {
                auto name = readString();
                auto found = globals.find(name);
                if (found == globals.end()) {
                    RuntimeError("Undefined variable '%s'.", name.c_str());
                    return IResult::RUNTIME_ERROR;
                }
                found->second = Peek(0);
                break;
            }
            case OpCode::GET_UPVALUE: {
                auto slot = readByte();
                Push(*frames.back().closure->upvalues[slot]->location);
                break;
            }
            case OpCode::SET_UPVALUE: {
                auto slot = readByte();
                *frames.back().closure->upvalues[slot]->location = Peek(0);
                break;
            }
            case OpCode::GET_PROPERTY: {
                InstanceValue instance;

                try {
                    instance = std::get<InstanceValue>(Peek(0));
                } catch (std::bad_variant_access&) {
                    RuntimeError("Only instances have properties.");
                    return IResult::RUNTIME_ERROR;
                }

                auto name = readString();
                auto found = instance->fields.find(name);
                if (found != instance->fields.end()) {
                    auto value = found->second;
                    Pop(); // Instance.
                    Push(value);
                    break;
                }

                if (!BindMethod(instance->classValue, name)) {
                    return IResult::RUNTIME_ERROR;
                }
                break;
            }
            case OpCode::SET_PROPERTY: {
                try {
                    auto instance = std::get<InstanceValue>(Peek(1));
                    auto name = readString();
                    instance->fields[name] = Peek(0);
                    
                    auto value = Pop();
                    Pop();
                    Push(value);
                } catch (std::bad_variant_access&) {
                    RuntimeError("Only instances have fields.");
                    return IResult::RUNTIME_ERROR;
                }
                break;
            }
            case OpCode::GET_SUPER: {
                auto name = readString();
                auto superclass = std::get<ClassValue>(Pop());
                
                if (!BindMethod(superclass, name)) {
                    return IResult::RUNTIME_ERROR;
                }
                break;
            }
            case OpCode::EQUAL: {
                DoublePopAndPush(Peek(0) == Peek(1));
                break;
            }
                
            case OpCode::GREATER:   BINARY_OP(>); break;
            case OpCode::LESS:      BINARY_OP(<); break;
                
            case OpCode::ADD: {
                auto success = std::visit(overloaded {
                    [this](double b, double a) -> bool {
                        this->DoublePopAndPush(a + b);
                        return true;
                    },
                    [this](std::string b, std::string a) -> bool {
                        this->DoublePopAndPush(a + b);
                        return true;
                    },
                    [this](auto a, auto b) -> bool {
                        this->RuntimeError("Operands must be two numbers or two strings.");
                        return false;
                    }
                }, Peek(0), Peek(1));
                
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
            case OpCode::NOT: Push(IsFalse(Pop())); break;
            
            case OpCode::BW_NOT: {
                try {
                    auto bwNotted = ~static_cast<int>(std::get<double>(Peek(0)));
                    Pop();
                    Push(static_cast<double>(bwNotted));
                } catch (std::bad_variant_access) {
                    RuntimeError("Operand must be a number.");
                    return IResult::RUNTIME_ERROR;
                }
            }
                
            case OpCode::NEGATE:
                try {
                    auto negated = -std::get<double>(Peek(0));
                    Pop(); // if we get here it means it was good
                    Push(negated);
                } catch (std::bad_variant_access&) {
                    RuntimeError("Operand must be a number.");
                    return IResult::RUNTIME_ERROR;
                }
                break;
                
            case OpCode::PRINT: {
                std::cout << Pop() << std::endl;
                break;
            }
            
            case OpCode::JUMP: {
                auto offset = readShort();
                frames.back().ip += offset;
                break;
            }
                
            case OpCode::LOOP: {
                auto offset = readShort();
                frames.back().ip -= offset;
                break;
            }
                
            case OpCode::JUMP_IF_FALSE: {
                auto offset = readShort();
                if (IsFalse(Peek(0))) {
                    frames.back().ip += offset;
                }
                break;
            }
                
            case OpCode::CALL: {
                int argCount = readByte();
                if (!CallValue(Peek(argCount), argCount)) {
                    return IResult::RUNTIME_ERROR;
                }
                break;
            }
                
            case OpCode::INVOKE: {
                auto method = readString();
                int argCount = readByte();
                if (!Invoke(method, argCount)) {
                    return IResult::RUNTIME_ERROR;
                }
                break;
            }
                
            case OpCode::SUPER_INVOKE: {
                auto method = readString();
                int argCount = readByte();
                auto superclass = std::get<ClassValue>(Pop());
                if (!InvokeFromClass(superclass, method, argCount)) {
                    return IResult::RUNTIME_ERROR;
                }
                break;
            }
                
            case OpCode::CLOSURE: {
                auto function = std::get<Func>(readConstant());
                auto closure = std::make_shared<ClosureObject>(function);
                Push(closure);
                for (int i = 0; i < static_cast<int>(closure->upvalues.size()); i++) {
                    auto isLocal = readByte();
                    auto index = readByte();
                    if (isLocal) {
                        closure->upvalues[i] = CaptureUpvalue(&stack[frames.back().stackOffset + index]);
                    } else {
                        closure->upvalues[i] = frames.back().closure->upvalues[index];
                    }
                }
                break;
            }
            
            case OpCode::CLOSE_UPVALUE:
                CloseUpvalues(&stack.back());
                Pop();
                break;
                
            case OpCode::RETURN: {
                auto result = Pop();
                CloseUpvalues(&stack[frames.back().stackOffset]);
                
                auto lastOffset = frames.back().stackOffset;
                frames.pop_back();
                if (frames.empty()) {
                    Pop();
                    return IResult::OK;
                }

                stack.resize(lastOffset);
                stack.reserve(STACK_MAX);
                Push(result);
                break;
            }
                
            case OpCode::CLASS:
                Push(std::make_shared<ClassObject>(readString()));
                break;
                
            case OpCode::INHERIT: {
                try {
                    auto superclass = std::get<ClassValue>(Peek(1));
                    auto subclass = std::get<ClassValue>(Peek(0));
                    subclass->memberFuncs = superclass->memberFuncs;
                    Pop(); // Subclass.
                    break;
                } catch (std::bad_variant_access&) {
                    RuntimeError("Superclass must be a class.");
                    return IResult::RUNTIME_ERROR;
                }
            }
                
            case OpCode::METHOD:
                DefineMethod(readString());
                break;
        }
    }
    
    return IResult::OK;

#undef BINARY_OP
}
