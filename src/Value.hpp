#ifndef VALUE_HPP
#define VALUE_HPP

#include "Common.hpp"
#include "Opcodes.hpp"

#include <variant>
#include <memory>
#include <unordered_map>
#include <map>
#include <functional>
#include <cstdio>

struct NativeFunctionObject;
struct UpvalueObject;
struct ClassObject;
struct InstanceObject;
struct MemberFuncObject;
struct FileObject;
struct ArrayObject;
class FunctionObject;
class ClosureObject;
class Compiler;
class Parser;
class VM;
using Func = std::shared_ptr<FunctionObject>;
using NativeFunction = std::shared_ptr<NativeFunctionObject>;
using Closure = std::shared_ptr<ClosureObject>;
using UpvalueValue = std::shared_ptr<UpvalueObject>;
using ClassValue = std::shared_ptr<ClassObject>;
using InstanceValue = std::shared_ptr<InstanceObject>;
using BoundMethodValue = std::shared_ptr<MemberFuncObject>;
using File = std::shared_ptr<FileObject>;
using Array = std::shared_ptr<ArrayObject>;

using Value = std::variant<
    double, bool, std::monostate,
    std::string, Func, NativeFunction,
    Closure, UpvalueValue, ClassValue,
    InstanceValue, BoundMethodValue,
    File, Array, FILE*>;

class Chunk {
    std::vector<uint8_t> code;
    std::vector<Value> constants;
    std::vector<int> lines;

public:
    uint8_t get_code(int offset) const { return code[offset]; };
    void set_code(int offset, uint8_t value) { code[offset] = value; }
    const Value& get_constant(int constant) const { return constants[constant]; };
    void write(uint8_t byte, int line);
    void write(OpCode opcode, int line);
    unsigned long add_constant(Value value);
    int disas_instruction(int offset);
    void disassemble(const std::string& name);
    int get_line(int instruction) { return lines[instruction]; }
    int count() { return static_cast<int>(code.size()); }
};

using NativeFn = std::function<Value(int, std::vector<Value>::iterator)>;

struct NativeFunctionObject {
    NativeFn function;
};

struct UpvalueObject {
    Value* location;
    Value closed;
    UpvalueValue next;
    UpvalueObject(Value* slot): location(slot), closed(std::monostate()), next(nullptr) {}
};

struct ClassObject {
    std::string name;
    std::unordered_map<std::string, Closure> methods;
    explicit ClassObject(std::string name): name(name) {}
};

struct InstanceObject {
    ClassValue class_value;
    std::unordered_map<std::string, Value> fields;
    explicit InstanceObject(ClassValue klass): class_value(klass) {}
};

struct MemberFuncObject {
    InstanceValue receiver;
    Closure method;
    explicit MemberFuncObject(InstanceValue receiver, Closure method)
        : receiver(receiver), method(method) {}
};

class FunctionObject {
private:
    int arity;
    int upvalue_count = 0;
    std::string name;
    Chunk chunk;

public:
    FunctionObject(int arity, const std::string& name)
        : arity(arity), name(name), chunk(Chunk()) {}
    
    const std::string& get_name() const
    {
        return name;
    }

    bool operator==(const Func& rhs) const
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
    
    const Value& get_const(int constant) const
    {
        return chunk.get_constant(constant);
    }

    friend Compiler;
    friend Parser;
    friend VM;
    friend Chunk;
    friend ClosureObject;
};

class ClosureObject {
public:
    Func function;
    std::vector<UpvalueValue> upvalues;
    explicit ClosureObject(Func function): function(function)
    {
        upvalues.resize(function->upvalue_count, nullptr);
    };
};

struct FileObject {
    const std::string path;
    std::FILE* file;

    FileObject(const std::string& path, const std::string& mode)
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

    ~FileObject()
    {
        close();
    }
};

struct ArrayObject {
    std::vector<Value> values;
};

std::ostream& operator<<(std::ostream& os, const Value& v);

struct OutputVisitor {
    void operator()(const double d) const { std::cout << d; }
    void operator()(const bool b) const { std::cout << (b ? "true" : "false"); }
    void operator()(const std::monostate n) const { std::cout << "null"; }
    void operator()(const std::string& s) const { std::cout << s; }
    void operator()(const Func& f) const
    {
        if (f->get_name().empty()) {
            std::cout << "<script>";
        } else {
            std::cout << "<fn " << f->get_name() << ">";
        }
    }
    void operator()(const NativeFunction& f) const { std::cout << "<native fn>"; }
    void operator()(const Closure& c) const { std::cout << Value(c->function); }
    void operator()(const UpvalueValue& u) const { std::cout << "upvalue"; }
    void operator()(const ClassValue& c) const { std::cout << c->name; }
    void operator()(const InstanceValue& i) const { std::cout << i->class_value->name << " instance"; }
    void operator()(const BoundMethodValue& m) const { std::cout << Value(m->method->function); }
    void operator()(const File& f) const
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
    void operator()(const Array& a) const
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

inline std::ostream& operator<<(std::ostream& os, const Value& v)
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

inline bool is_false(const Value& v)
{
    return std::visit(FalseVisitor(), v);
}

#endif
