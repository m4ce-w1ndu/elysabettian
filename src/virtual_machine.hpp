#ifndef VIRTUALMACHINE_HPP
#define VIRTUALMACHINE_HPP

#include "value.hpp"
#include "compiler.hpp"

#include "library.hpp"

#include <unordered_map>
#include <map>
#include <ctime>
#include <cmath>
#include <sstream>
#include <iomanip>

constexpr auto FRAMES_MAX = 64;
constexpr auto STACK_MAX = (FRAMES_MAX* UINT8_COUNT);

enum class InterpretResult {
    Ok,
    CompileError,
    RuntimeError
};

struct CallFrame {
    closure_t closure;
    unsigned ip;
    unsigned long stack_offset;
};

struct CallVisitor;

class VirtualMachine {
    // TODO: Switch to a fixed array to prevent pointer invalidation
    std::vector<value_t> stack;
    std::vector<CallFrame> frames;
    std::unordered_map<std::string, value_t> globals;
    upvalue_value_t open_upvalues;
    std::string init_string = "init";

    // Load arrays by default
    const stdlib::libnativearray array_lib;
    
    inline void reset_stack()
    {
        stack.clear();
        frames.clear();
        stack.reserve(STACK_MAX);
        open_upvalues = nullptr;
    }
    
    void runtime_error(const char* format, ...);
    void define_native(const std::string& name, native_fn_t function);
    void define_native_const(const std::string& name, value_t value);
    template <typename F>
    bool binary_op(F op);
    void double_pop_and_push(const value_t& v);
    
    inline void push(const value_t& v)
    {
        stack.push_back(v);
    }
    
    inline value_t pop()
    {
        auto v = std::move(stack.back());
        stack.pop_back();
        return v;
    }
    
    inline const value_t& peek(int distance)
    {
        return stack[stack.size() - 1 - distance];
        
    }

    bool call_value(const value_t& callee, int arg_count);
    bool invoke(const std::string& name, int arg_count);
    bool invoke_from_class(class_value_t class_value, const std::string& name, int arg_count);
    bool bind_method(class_value_t class_value, const std::string& name);
    upvalue_value_t capture_upvalue(value_t* local);
    void close_upvalues(value_t* last);
    void define_method(const std::string& name);
    bool call(const closure_t& closure, int arg_count);

public:
    explicit VirtualMachine()
    {
        // Library loader
        auto import_lib = [this](int argc, std::vector<value_t>::iterator args) -> value_t {
            if (argc < 1 || argc > 1) {
                runtime_error("import(libnamestr) expects 1 parameter. Got %d.", argc);
                return std::monostate();
            }

            std::string libname;
            try {
                libname = std::get<std::string>(*args);
                const auto& lib = stdlib::Libraries.at(libname);

                // Loading functions
                for (const auto& f : lib->functions)
                    define_native(f.first, f.second);
                // Loading constants
                for (const auto& c : lib->constants)
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
        auto to_native_string = [this](int argc, std::vector<value_t>::iterator args) -> value_t {
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
                std::string operator()(const func_t& f) const
                {
                    if (f->get_name().empty())
                        return "<script>";
                    return "<func " + f->get_name() + ">";
                }
                std::string operator()(const native_function_t& nf) const { return "<native func>"; }
                std::string operator()(const closure_t& c) const { return "<closure>"; }
                std::string operator()(const upvalue_value_t& uv) const { return "<upvalue>"; }
                std::string operator()(const class_value_t& cv) const { return cv->name; }
                std::string operator()(const instance_value_t& iv) const { return iv->class_value->name + " instance"; }
                std::string operator()(const member_func_value_t& bm) const { return bm->method->function->get_name(); }
                std::string operator()(const file_t& f) const { return f->path; }
                std::string operator()(const array_t& a) const { return "<array[" + std::to_string(a->values.size()) + "]"; }
                std::string operator()(const FILE* f) const { return "<native stream>"; }
            };

            return std::visit(TypeVisitor(), *args);
        };

        auto native_clock = [](int argc, std::vector<value_t>::iterator args) -> value_t {
            return static_cast<double>(clock() / CLOCKS_PER_SEC);
        };

        auto native_date = [](int argc, std::vector<value_t>::iterator args) -> value_t {
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

        auto native_version = [](int argc, std::vector<value_t>::iterator args) -> value_t {
            fmt::print("{}\n", VERSION_FULLNAME);
            return "Elysabettian 1.1 Maurizio";
        };

        auto exit_env = [](int argc, std::vector<value_t>::iterator args) -> value_t {
            fmt::print("Bye...\n");
            exit(EXIT_SUCCESS);
        };

        // Load arrays
        for (const auto& func : array_lib.functions)
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
    InterpretResult Interpret(const std::string& source);
    InterpretResult Run();
    
    friend CallVisitor;
};

#endif /* vm_hpp */
