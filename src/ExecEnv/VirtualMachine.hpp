//
//  VirtualMachine.hpp
//  CElysabettian
//
//  Created by Simone Rolando on 11/07/21.
//

#ifndef VIRTUALMACHINE_HPP
#define VIRTUALMACHINE_HPP

#include "Value.hpp"
#include "Compiler.hpp"
#include <unordered_map>
#include <ctime>

#define FRAMES_MAX 64
#define STACK_MAX (FRAMES_MAX * UINT8_COUNT)

enum class IResult {
    OK,
    COMPILE_ERROR,
    RUNTIME_ERROR
};

struct CallFrame {
    Closure closure;
    unsigned ip;
    unsigned long stackOffset;
};

static Value ClockNative(int argCount, std::vector<Value>::iterator args)
{
    return (double)clock() / CLOCKS_PER_SEC;
}

static Value DateNative(int argCount, std::vector<Value>::iterator args)
{
    time_t now = time(nullptr);
    tm newTime;
    localtime_r(&now, &newTime);
    char dateTime[30];
    strftime(dateTime, sizeof(dateTime) / sizeof(char), "%d/%m/%y, %H:%M:%S", &newTime);
    return std::string(dateTime);
}

static Value VersionNative(int argc, std::vector<Value>::iterator args)
{
    return std::string("CElysabettian 1.0 Maurizio");
}

struct CallVisitor;

class VM {
    // TODO: Switch to a fixed array to prevent pointer invalidation
    std::vector<Value> stack;
    std::vector<CallFrame> frames;
    std::unordered_map<std::string, Value> globals;
    UpvalueValue openUpvalues;
    std::string initString = "init";
    
    inline void ResetStack()
    {
        stack.clear();
        frames.clear();
        stack.reserve(STACK_MAX);
        openUpvalues = nullptr;
    }
    
    void RuntimeError(const char* format, ...);
    void DefineNative(const std::string& name, NativeFn function);
    template <typename F>
    bool BinaryOp(F op);
    void DoublePopAndPush(const Value& v);
    
    inline void Push(const Value& v)
    {
        stack.push_back(v);
        
    }
    
    inline Value Pop()
    {
        auto v = std::move(stack.back());
        stack.pop_back();
        return v;
    }
    
    inline const Value& Peek(int distance)
    {
        return stack[stack.size() - 1 - distance];
        
    }
    bool CallValue(const Value& callee, int argCount);
    bool Invoke(const std::string& name, int argCount);
    bool InvokeFromClass(ClassValue klass, const std::string& name, int argCount);
    bool BindMethod(ClassValue klass, const std::string& name);
    UpvalueValue CaptureUpvalue(Value* local);
    void CloseUpvalues(Value* last);
    void DefineMethod(const std::string& name);
    bool Call(const Closure& closure, int argCount);
    
public:
    explicit VM()
    {
        stack.reserve(STACK_MAX);
        openUpvalues = nullptr;
        DefineNative("clock", ClockNative);
        DefineNative("date", DateNative);
        DefineNative("version", VersionNative);
    }
    IResult Interpret(const std::string& source);
    IResult Run();
    
    friend CallVisitor;
};

#endif /* vm_hpp */
