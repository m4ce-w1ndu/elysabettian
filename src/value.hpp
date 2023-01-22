#ifndef VALUE_HPP
#define VALUE_HPP

#include "common.hpp"
#include "opcodes.hpp"

#include <variant>
#include <memory>
#include <unordered_map>
#include <map>
#include <functional>
#include <cstdio>

struct native_func_obj;
struct upvalue_obj;
struct class_obj;
struct instance_obj;
struct member_func_obj;
struct file_obj;
struct array_obj;
class function_obj;
class closure_obj;
class compiler_t;
class parser_t;
class VirtualMachine;
using func_t = std::shared_ptr<function_obj>;
using native_function_t = std::shared_ptr<native_func_obj>;
using closure_t = std::shared_ptr<closure_obj>;
using upvalue_value_t = std::shared_ptr<upvalue_obj>;
using class_value_t = std::shared_ptr<class_obj>;
using instance_value_t = std::shared_ptr<instance_obj>;
using member_func_value_t = std::shared_ptr<member_func_obj>;
using file_t = std::shared_ptr<file_obj>;
using array_t = std::shared_ptr<array_obj>;

using value_t = std::variant<
    double, bool, std::monostate,
    std::string, func_t, native_function_t,
    closure_t, upvalue_value_t, class_value_t,
    instance_value_t, member_func_value_t,
    file_t, array_t, FILE*>;

class Chunk {
    std::vector<uint8_t> code;
    std::vector<value_t> constants;
    std::vector<int> lines;

public:
    uint8_t get_code(int offset) const { return code[offset]; };
    void set_code(int offset, uint8_t value) { code[offset] = value; }
    const value_t& get_constant(int constant) const { return constants[constant]; };
    void write(uint8_t byte, int line);
    void write(opcode_t opcode, int line);
    unsigned long add_constant(value_t value);
    int disas_instruction(int offset);
    void disassemble(const std::string& name);
    int get_line(int instruction) { return lines[instruction]; }
    int count() { return static_cast<int>(code.size()); }
};

using native_fn_t = std::function<value_t(int, std::vector<value_t>::iterator)>;

struct native_func_obj {
    native_fn_t function;
};

struct upvalue_obj {
    value_t* location;
    value_t closed;
    upvalue_value_t next;
    upvalue_obj(value_t* slot): location(slot), closed(std::monostate()), next(nullptr) {}
};

struct class_obj {
    std::string name;
    std::unordered_map<std::string, closure_t> methods;
    explicit class_obj(std::string name): name(name) {}
};

struct instance_obj {
    class_value_t class_value;
    std::unordered_map<std::string, value_t> fields;
    explicit instance_obj(class_value_t klass): class_value(klass) {}
};

struct member_func_obj {
    instance_value_t receiver;
    closure_t method;
    explicit member_func_obj(instance_value_t receiver, closure_t method)
        : receiver(receiver), method(method) {}
};

class function_obj {
private:
    int arity;
    int upvalue_count = 0;
    std::string name;
    Chunk chunk;

public:
    function_obj(int arity, const std::string& name)
        : arity(arity), name(name), chunk(Chunk()) {}
    
    const std::string& get_name() const
    {
        return name;
    }

    bool operator==(const func_t& rhs) const
    {
        return false;
    }
    
    Chunk& get_chunk()
    {
        return chunk;
    }
    
    uint8_t get_code(int offset)
    {
        return chunk.get_code(offset);
    }
    
    const value_t& get_const(int constant) const
    {
        return chunk.get_constant(constant);
    }

    friend compiler_t;
    friend parser_t;
    friend VirtualMachine;
    friend Chunk;
    friend closure_obj;
};

class closure_obj {
public:
    func_t function;
    std::vector<upvalue_value_t> upvalues;
    explicit closure_obj(func_t function): function(function)
    {
        upvalues.resize(function->upvalue_count, nullptr);
    };
};

struct file_obj {
    const std::string path;
    std::FILE* file;

    file_obj(const std::string& path, const std::string& mode)
        : path(path), file(fopen(path.c_str(), mode.c_str()))
    {}

    inline bool is_open() { return file != nullptr; }

    inline void close()
    { 
        if (file != nullptr) {
            fclose(file);
            file = nullptr;
        }
    }

    inline void flush()
    {
        if (this->is_open()) {
            fflush(file);
        }
    }

    inline std::string read_all()
    {
        if (!this->is_open()) {
            return "";
        }

        // Get the size of the file
        fseek(file, 0, SEEK_END);
        size_t size = ftell(file);
        fseek(file, 0, SEEK_SET);
        // Allocate a string buffer the size of the file
        std::string buffer(size, '\0');
        // Read the file into the buffer
        fread(buffer.data(), 1, size, file);
        // Return the buffer
        return buffer;
    }

    inline void write_all(const std::string& data)
    {
        if (!this->is_open()) {
            return;
        }

        fwrite(data.data(), 1, data.size(), file);
    }

    ~file_obj()
    {
        close();
    }
};

struct array_obj {
    std::vector<value_t> values;
};

std::ostream& operator<<(std::ostream& os, const value_t& v);

struct OutputVisitor {
    void operator()(const double d) const { std::cout << d; }
    void operator()(const bool b) const { std::cout << (b ? "true" : "false"); }
    void operator()(const std::monostate n) const { std::cout << "null"; }
    void operator()(const std::string& s) const { std::cout << s; }
    void operator()(const func_t& f) const
    {
        if (f->get_name().empty()) {
            std::cout << "<script>";
        } else {
            std::cout << "<fn " << f->get_name() << ">";
        }
    }
    void operator()(const native_function_t& f) const { std::cout << "<native fn>"; }
    void operator()(const closure_t& c) const { std::cout << value_t(c->function); }
    void operator()(const upvalue_value_t& u) const { std::cout << "upvalue"; }
    void operator()(const class_value_t& c) const { std::cout << c->name; }
    void operator()(const instance_value_t& i) const { std::cout << i->class_value->name << " instance"; }
    void operator()(const member_func_value_t& m) const { std::cout << value_t(m->method->function); }
    void operator()(const file_t& f) const
    {
        if (f->file == stdin) {
            std::cout << "stdin" << std::endl;
            return;
        } else if (f->file == stdout) {
            std::cout << "stdout" << std::endl;
            return;
        } else if (f->file == stderr) {
            std::cout << "stderr" << std::endl;
            return;
        }

        std::cout << "path: " << f->path << ", open: "
            << std::boolalpha << f->is_open();
    }
    void operator()(const array_t& a) const
    {
        std::cout << "array { ";
        for (size_t i = 0; i < a->values.size(); ++i) {
            if (i < (a->values.size() - 1))
                std::cout << a->values[i] << ", ";
            else
                std::cout << a->values[i];
        }
        std::cout << " }";
    }
};

inline std::ostream& operator<<(std::ostream& os, const value_t& v)
{
    std::visit(OutputVisitor(), v);
    return os;
}

struct FalseVisitor {
    bool operator()(const bool b) const { return !b; }
    bool operator()(const std::monostate n) const { return true; }
    
    template <typename T>
    bool operator()(const T& value) const { return false; }
};

inline bool is_false(const value_t& v)
{
    return std::visit(FalseVisitor(), v);
}

#endif
