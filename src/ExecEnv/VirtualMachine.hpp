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

#include "CommonLibrary.hpp"

#include <unordered_map>
#include <map>
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
    std::map<std::string, std::vector<Value>> arrays;

    
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
        auto importLibrary = [this](int argc, std::vector<Value>::iterator args) -> Value {
            if (argc < 1 || argc > 1) {
                RuntimeError("Error: loadLibrary(name) expects 1 string argument. Got %d.", argc);
                return std::monostate();
            }
            try {
                auto name = std::get<std::string>(*args);
                const auto library = Library::GetLibrary(name);
                for (size_t i = 0; i < library.size(); ++i)
                    DefineNative(library[i].first, library[i].second);
                return std::monostate();
            } catch (std::bad_variant_access) {
                RuntimeError("Error: argument must be of string type.");
                return std::monostate();
            }
        };

        // array creation
        auto arrayCreateNative = [this](int argc, std::vector<Value>::iterator args) -> Value {
            if (argc < 1 || argc > 1) {
                RuntimeError("arrayCreate(name) expects 1 parameter. Got %d.", argc);
                return std::monostate();
            }
            try {
                auto name = std::get<std::string>(*args);
                this->arrays[name] = std::vector<Value>();
                return std::monostate();
            } catch (std::bad_variant_access) {
                RuntimeError("Array name must be of string type.");
                return std::monostate();
            }
        };

        // array set value
        auto arraySetNative = [this](int argc, std::vector<Value>::iterator args) -> Value {
            if (argc < 3 | argc > 3) {
                RuntimeError("arraySet(name, index, value) expects 3 parameter. Got %d.", argc);
                return std::monostate();
            }
            try {
                auto name = std::get<std::string>(*args);
                auto index = static_cast<size_t>(std::get<double>(*(args + 1)));
                auto value = *(args + 3);
                this->arrays[name].at(index) = value;
                return value;
            } catch (std::bad_variant_access) {
                RuntimeError("Array name must be of string type.");
                return std::monostate();
            } catch (std::length_error) {
                RuntimeError("Index out of bounds.");
                return std::monostate();
            } catch (std::out_of_range) {
                RuntimeError("There is no array declared with the specified name.");
                return std::monostate();
            }
        };

        // array get value
        auto arrayGetValue = [this](int argc, std::vector<Value>::iterator args) -> Value {
            if (argc < 2 || argc > 2) {
                RuntimeError("arrayGet(name, index) expects 2 parameters. Got %d.", argc);
                return std::monostate();
            }
            try {
                auto name = std::get<std::string>(*args);
                auto index = static_cast<size_t>(std::get<double>(*(args + 1)));
                return this->arrays[name].at(index);
            } catch (std::bad_variant_access) {
                RuntimeError("Array name must be of string type.");
                return std::monostate();
            } catch (std::out_of_range) {
                RuntimeError("There is no array declared with the specified name.");
                return std::monostate();
            }
        };

        // array push
        auto arrayPushNative = [this](int argc, std::vector<Value>::iterator args) -> Value {
            if (argc < 2 || argc > 2) {
                RuntimeError("arrayPush(name, value) expects 2 paramters. Got %d.", argc);
                return std::monostate();
            }
            try {
                auto name = std::get<std::string>(*args);
                auto value = *(args + 1);
                this->arrays[name].push_back(value);
                return value;
            } catch (std::bad_variant_access) {
                RuntimeError("Array name must be of string type.");
                return std::monostate();
            } catch (std::out_of_range) {
                RuntimeError("There is no array declared with the specified name.");
                return std::monostate();
            }
        };

        auto clockNative = [](int argc, std::vector<Value>::iterator args) -> Value {
            return static_cast<double>(clock() / CLOCKS_PER_SEC);
        };

        auto dateNative = [](int argc, std::vector<Value>::iterator args) -> Value {
            time_t now = time(nullptr);
            tm newTime;
#if defined(__linux__) || defined(__APPLE__)
            localtime_r(&now, &newTime);
#elif defined(_WIN32)
            localtime_s(&newTime, &now);
#endif
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

        auto exitFromEnv = [](int argc, std::vector<Value>::iterator args) -> Value {
            std::cout << "Bye..." << std::endl;
            exit(EXIT_SUCCESS);
        };

        stack.reserve(STACK_MAX);
        openUpvalues = nullptr;
        DefineNative("exit", exitFromEnv);
        DefineNative("clock", clockNative);
        DefineNative("date", dateNative);
        DefineNative("version", versionNative);
        DefineNative("read", readPromptNative);
        DefineNative("import", importLibrary);
        DefineNative("arrayCreate", arrayCreateNative);
        DefineNative("arraySet", arraySetNative);
        DefineNative("arrayGet", arrayGetValue);
        DefineNative("arrayPush", arrayPushNative);
    }
    IResult Interpret(const std::string& source);
    IResult Run();
    
    friend CallVisitor;
};

#endif /* vm_hpp */
