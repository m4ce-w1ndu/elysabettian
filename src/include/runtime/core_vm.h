#ifndef ELY_RT_COREVM_H
#define ELY_RT_COREVM_H

// Elysabettian language headers.
#include "runtime/value.h"
#include "runtime/compiler.h"

#include "provider/earray.h"
#include "provider/emath.h"
#include "provider/estdio.h"
#include "provider/cstdio.h"

// Standard C++ headers.
#include <map>
#include <ctime>
#include <cmath>
#include <sstream>
#include <iomanip>
#include <functional>
#include <unordered_map>

/**
 * @brief Max number of stack frames.
*/
constexpr size_t FRAMES_MAX = 64;

/**
 * @brief Max size of stack.
*/
constexpr size_t STACK_MAX = (FRAMES_MAX* UINT8_COUNT);

/**
 * @brief Lib import data type.
*/
using IntFunc = std::function <Value(int, std::vector<Value>::iterator)>;

/**
 * @brief Possible result of the interpretation.
 * As the code is converted to bytecode, standard checks
 * are performed, and the general result is described
 * by this enum.
*/
enum class InterpretResult {
    Ok,
    CompileError,
    RuntimeError
};

/**
 * @brief Describes a call stack function call frame
*/
struct CallFrame {
    Closure closure;
    size_t ip = 0;
    size_t stack_offset = 0;
};

/**
 * @brief Function call visitor
*/
struct CallVisitor;

/**
 * @brief Runtime Virtual Machine. Executes the code
 * using the stored bytecode compilation result.
*/
class VirtualMachine {
public:
    /**
     * @brief Create a new Virtual Machine. It is responsible for the effective
     * execution of the previously compiled bytecode.
    */
    explicit VirtualMachine()
    {
        // Library loader
        IntFunc import_lib = [this](int argc, std::vector<Value>::iterator args) -> Value {
            if (argc < 1 || argc > 1) {
                runtime_error("import(libnamestr) expects 1 parameter. Got %d.", argc);
                return std::monostate();
            }

            std::string libname;
            try {
                libname = std::get<std::string>(*args);
                const std::shared_ptr<stdlib::ELibrary>& lib = libraries.at(libname);

                // Loading functions
                for (const std::pair<std::string, NativeFn> f : lib->functions)
                    define_native(f.first, f.second);
                // Loading constants
                for (const std::pair<std::string, Value> c : lib->constants)
                    define_native_const(c.first, c.second);

                return true;
            }
            catch (std::bad_variant_access) {
                runtime_error("Array name must be of string type.");
                return std::monostate();
            }
            catch (std::out_of_range&) {
                runtime_error("Library %s does not exist.", libname.c_str());
                return std::monostate();
            }
        };

        // to string
        IntFunc to_native_string = [this](int argc, std::vector<Value>::iterator args) -> Value {
            if (argc < 1 || argc > 1) {
                runtime_error("toString expects 1 parameter. Got %d.", argc);
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
                    if (f->get_name().empty())
                        return "<script>";
                    return "<func " + f->get_name() + ">";
                }
                std::string operator()(const NativeFunc& nf) const { return "<native func>"; }
                std::string operator()(const Closure& c) const { return "<closure>"; }
                std::string operator()(const Upvalue& uv) const { return "<upvalue>"; }
                std::string operator()(const Class& cv) const { return cv->name; }
                std::string operator()(const Instance& iv) const { return iv->class_value->name + " instance"; }
                std::string operator()(const MemberFunc& bm) const { return bm->method->function->get_name(); }
                std::string operator()(const File& f) const { return f->path; }
                std::string operator()(const Array& a) const { return "<array[" + std::to_string(a->values.size()) + "]"; }
                std::string operator()(const FILE* f) const { return "<native stream>"; }
            };

            return std::visit(TypeVisitor(), *args);
        };

        IntFunc native_clock = [](int argc, std::vector<Value>::iterator args) -> Value {
            return static_cast<double>(clock() / CLOCKS_PER_SEC);
        };

        IntFunc native_date = [](int argc, std::vector<Value>::iterator args) -> Value {
            time_t now_time = time(nullptr);
            tm new_time;
#if defined(__linux__) || defined(__APPLE__)
            localtime_r(&now_time, &new_time);
#elif defined(_WIN32)
            localtime_s(&new_time, &now_time);
#endif
            char datetime[30];
            strftime(datetime, sizeof(datetime) / sizeof(char), "%d/%m/%y, %H:%M:%S", &new_time);
            return std::string(datetime);
        };

        IntFunc native_version = [](int argc, std::vector<Value>::iterator args) -> Value {
            fmt::print("{}\n", VERSION_FULLNAME);
            return "Elysabettian 1.1 Maurizio";
        };

        IntFunc exit_env = [](int argc, std::vector<Value>::iterator args) -> Value {
            fmt::print("Bye...\n");
            exit(EXIT_SUCCESS);
        };

        // Load arrays
        for (const std::pair<std::string, NativeFn> func : array_lib.functions)
            define_native(func.first, func.second);

        stack.reserve(STACK_MAX);
        open_upvalues = nullptr;
        define_native("exit", exit_env);
        define_native("clock", native_clock);
        define_native("date", native_date);
        define_native("version", native_version);
        define_native("import", import_lib);
        define_native("string", to_native_string);
    }

    /**
     * @brief Interpret given source code. This starts the compilation
     * and execution chain, making calls to Parser and Compiler.
     * @param source Source code to run.
     * @return Result of the interpretation.
    */
    InterpretResult interpret(const std::string& source);

    /**
     * @brief Runs the interpreted source code.
     * @return Result of the interpretation.
    */
    InterpretResult run();

private:
    /**
     * @brief Stack of the VM.
    */
    std::vector<Value> stack;

    /**
     * @brief VM call stack frames storage.
    */
    std::vector<CallFrame> frames;

    /**
     * @brief Global variables.
    */

    std::unordered_map<std::string, Value> globals;

    /**
     * @brief List of open (valid) upvalues.
    */
    Upvalue open_upvalues;

    /**
     * @brief Initializer string.
    */
    const std::string init_string = "init";

    /**
     * @brief Arrays library.
    */
    const stdlib::EArray array_lib;

    /**
     * @brief Libraries that can be loaded by the runtime on startup
    */
    const std::unordered_map<std::string, std::shared_ptr<stdlib::ELibrary>> libraries = {
        { "math", std::make_shared<stdlib::ELibrary>(stdlib::EMath()) },
        { "stdio", std::make_shared<stdlib::ELibrary>(stdlib::EStdio()) },
        { "cstdio", std::make_shared<stdlib::ELibrary>(stdlib::CStdio()) }
    };

    /**
     * @brief Reset the VM stack.
    */
    inline void reset_stack()
    {
        stack.clear();
        frames.clear();
        stack.reserve(STACK_MAX);
        open_upvalues = nullptr;
    }

    /**
     * @brief Raise a runtime error.  
    */
    void runtime_error(const char* format, ...);

    /**
     * @brief Define a native function.
    */
    void define_native(const std::string& name, NativeFn function);

    /**
     * @brief Define a native constant.
    */
    void define_native_const(const std::string& name, Value value);

    /**
     * @brief Perform a binary operation.
    */
    template <typename F>
    bool binary_op(F op);

    /**
     * @brief Perform a double pop and a push operation.
    */
    void double_pop_and_push(const Value& v);

    /**
     * @brief Push onto stack.
    */
    inline void push(const Value& v)
    {
        stack.push_back(v);
    }

    /**
     * @brief Pop from the stack.
    */
    inline Value pop()
    {
        const Value v = std::move(stack.back());
        stack.pop_back();
        return v;
    }

    /**
     * @brief Peek the stack.
    */
    inline const Value& peek(int distance)
    {
        return stack[stack.size() - 1 - distance];

    }

    /**
     * @brief Call a callable value.
    */
    bool call_value(const Value& callee, int arg_count);

    /**
     * @brief Invoke a callable object.
    */
    bool invoke(const std::string& name, int arg_count);

    /**
     * @brief Invoke a class member.
    */
    bool invoke_from_class(Class class_value, const std::string& name, int arg_count);

    /**
     * @brief Bind method to class.
    */
    bool bind_method(Class class_value, const std::string& name);

    /**
     * @brief Capture an upvalue.
    */
    Upvalue capture_upvalue(Value* local);

    /**
     * @brief Close all upvalues.
    */
    void close_upvalues(Value* last);

    /**
     * @brief Define a new method.
    */
    void define_method(const std::string& name);

    /**
     * @brief Perform a function call.
    */
    bool call(const Closure& closure, int arg_count);
    
    friend CallVisitor;
};

#endif
