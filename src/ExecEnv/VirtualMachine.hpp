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

#include <Libraries/Library.hpp>

#include <unordered_map>
#include <map>
#include <ctime>
#include <cmath>
#include <sstream>
#include <iomanip>

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

    // Load arrays by default
    Library::NativeArray arrayLibrary;
    
    inline void ResetStack()
    {
        stack.clear();
        frames.clear();
        stack.reserve(STACK_MAX);
        openUpvalues = nullptr;
    }
    
    void RuntimeError(const char* format, ...);
    void DefineNative(const std::string& name, NativeFn function);
    void DefineNativeConst(const std::string& name, Value value);
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
    bool InvokeFromClass(ClassValue classValue, const std::string& name, int argCount);
    bool BindMethod(ClassValue classValue, const std::string& name);
    UpvalueValue CaptureUpvalue(Value* local);
    void CloseUpvalues(Value* last);
    void DefineMethod(const std::string& name);
    bool Call(const Closure& closure, int argCount);
public:
    explicit VM()
    {
        // Library loader
        auto importLibrary = [this](int argc, std::vector<Value>::iterator args) -> Value {
            if (argc < 1 || argc > 1) {
                RuntimeError("import(libnamestr) expects 1 parameter. Got %d.", argc);
                return std::monostate();
            }

            std::string libName;
            try {
                libName = std::get<std::string>(*args);
                const auto& lib = Library::Libraries.at(libName);

                // Loading functions
                for (const auto& f : lib->functions)
                    DefineNative(f.first, f.second);
                // Loading constants
                for (const auto& c : lib->constants)
                    DefineNativeConst(c.first, c.second);

                return true;
            }
            catch (std::bad_variant_access) {
                RuntimeError("Array name must be of string type.");
                return std::monostate();
            }
            catch (std::out_of_range&) {
                RuntimeError("Library %s does not exist.", libName.c_str());
                return std::monostate();
            }
        };

        // to string
        auto toStringNative = [this](int argc, std::vector<Value>::iterator args) -> Value {
            if (argc < 1 || argc > 1) {
                RuntimeError("toString expects 1 parameter. Got %d.", argc);
                return std::monostate();
            }

            struct TypeVisitor {
                std::string operator()(const double d) const
                {
                    std::ostringstream ss;
                    ss << std::setprecision(32) << std::noshowpoint << d;
                    return ss.str();
                }
                std::string operator()(const std::string& s) const { return s; }
                std::string operator()(const std::monostate&) const { return "null"; }
                std::string operator()(const Func& f) const
                {
                    if (f->GetName().empty())
                        return "<script>";
                    return "<func " + f->GetName() + ">";
                }
                std::string operator()(const NativeFunction& nf) const { return "<native func>"; }
                std::string operator()(const Closure& c) const { return "<closure>"; }
                std::string operator()(const UpvalueValue& uv) const { return "<upvalue>"; }
                std::string operator()(const ClassValue& cv) const { return cv->name; }
                std::string operator()(const InstanceValue& iv) const { return iv->classValue->name + " instance"; }
                std::string operator()(const BoundMethodValue& bm) const { return bm->memberFunc->function->GetName(); }
                std::string operator()(const File& f) const { return f->path; }
                std::string operator()(const Array& a) const { return "<array[" + std::to_string(a->values.size()) + "]"; }
                std::string operator()(const FILE* f) const { return "<native stream>"; }
            };

            return std::visit(TypeVisitor(), *args);
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
            std::cout << "Elysabettian 1.0 Maurizio" << std::endl;
            return "Elysabettian 1.0 Maurizio";
        };

        auto exitFromEnv = [](int argc, std::vector<Value>::iterator args) -> Value {
            std::cout << "Bye..." << std::endl;
            exit(EXIT_SUCCESS);
        };

        // Load arrays
        for (const auto& func : arrayLibrary.functions)
            DefineNative(func.first, func.second);


        stack.reserve(STACK_MAX);
        openUpvalues = nullptr;
        DefineNative("exit", exitFromEnv);
        DefineNative("clock", clockNative);
        DefineNative("date", dateNative);
        DefineNative("version", versionNative);
        DefineNative("import", importLibrary);
        DefineNative("toString", toStringNative);
    }
    IResult Interpret(const std::string& source);
    IResult Run();
    
    friend CallVisitor;
};

#endif /* vm_hpp */
