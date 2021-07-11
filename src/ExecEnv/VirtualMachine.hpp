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
        auto clockNative = [](int argc, std::vector<Value>::iterator args) -> Value {
            return static_cast<double>(clock() / CLOCKS_PER_SEC);
        };

        auto dateNative = [](int argc, std::vector<Value>::iterator args) -> Value {
            time_t now = time(nullptr);
            tm newTime;
            localtime_r(&now, &newTime);
            char dateTime[30];
            strftime(dateTime, sizeof(dateTime) / sizeof(char), "%d/%m/%y, %H:%M:%S", &newTime);
            return std::string(dateTime);
        };

        auto versionNative = [](int argc, std::vector<Value>::iterator args) -> Value {
            return std::string("CElysabettian 1.0 Maurizio");
        };

        auto readPromptNative = [this](int argc, std::vector<Value>::iterator args) -> Value {

            std::string input;
            try {
                // Printing prompt message (if present)
                if (argc > 0) {
                    auto message = std::get<std::string>(*args);
                    std::cout << message;
                }
                
                // Reading input
                std::getline(std::cin, input);
                // trying conversion to double
                auto doubleval = std::stod(input);
                return doubleval;
            } catch (std::invalid_argument) {
                return input;
            } catch (std::bad_variant_access) {
                RuntimeError("Invalid type in read buffer. Aborting.");
                return "";
            } catch (std::length_error) {
                return "";
            }
        };

        auto sqrtNative = [this](int argc, std::vector<Value>::iterator args) -> Value {
            if (argc != 1) {
                RuntimeError("Error: expected 1 argument. Got %d.", argc);
                return std::monostate();
            }
            try {
                auto value = std::get<double>(*args);
                return sqrt(value);
            } catch (std::bad_variant_access) {
                std::cerr << "Operand must be a number.";
                return "";
            } catch (std::length_error) {
                return "";
            }
        };

        auto powNative = [this](int argc, std::vector<Value>::iterator args) -> Value {
            if (argc != 2) {
                RuntimeError("Error: expected 2 arguments. Got %d.", argc);
                return std::monostate();
            }
            try {
                auto base = std::get<double>(*args);
                auto ex = std::get<double>(*(args + 1));
                return std::pow(base, ex);
            } catch (std::bad_variant_access) {
                std::cerr << "Operands must be numbers.";
                return std::monostate();
            } catch (std::length_error) {
                return std::monostate();
            }
        };

        auto sumNative = [this](int argc, std::vector<Value>::iterator args) -> Value {
            if (argc < 1) {
                RuntimeError("Error: expected at least 1 argument. Got %d", argc);
                return std::monostate();
            }
            double sumVal = 0;
            try {
                for (int i = 0; i < argc; ++i)
                    sumVal += std::get<double>(*(args + i));
                return sumVal;
            } catch (std::bad_variant_access) {
                RuntimeError("Invalid operand detected in sum(). Only numbers are allowed.");
                return "";
            }
        };

        auto mathAbs = [this](int argc, std::vector<Value>::iterator args) -> Value {
            if (argc != 1) {
                RuntimeError("abs(x) expects 1 argument. Got %d.", argc);
                return std::monostate();
            }

            try {
                auto val = std::get<double>(*args);
                if (val < 0) return -val;
                return val;
            } catch (std::bad_variant_access) {
                RuntimeError("Operand must be of numeric type.");
                return std::monostate();
            }
        };

        auto mathAcos = [this](int argc, std::vector<Value>::iterator args) -> Value {
            if (argc < 1) {
                RuntimeError("acos(x) expects 1 argument. Got %d.", argc);
                return std::monostate();
            }
            try {
                auto val = std::get<double>(*args);
                return std::acos(val);
            } catch (std::bad_variant_access) {
                RuntimeError("Operand must be of numeric type.");
                return std::monostate();
            }
        };

        stack.reserve(STACK_MAX);
        openUpvalues = nullptr;
        DefineNative("clock", clockNative);
        DefineNative("date", dateNative);
        DefineNative("version", versionNative);
        DefineNative("sqrt", sqrtNative);
        DefineNative("pow", powNative);
        DefineNative("abs", mathAbs);
        DefineNative("acos", mathAcos);
        DefineNative("sum", sumNative);
        DefineNative("read", readPromptNative);
    }
    IResult Interpret(const std::string& source);
    IResult Run();
    
    friend CallVisitor;
};

#endif /* vm_hpp */
