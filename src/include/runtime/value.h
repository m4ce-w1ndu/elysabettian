#ifndef ELY_RT_VALUE_HPP
#define ELY_RT_VALUE_HPP

#include "common.h"
#include "opcodes.h"

#include <variant>
#include <memory>
#include <unordered_map>
#include <map>
#include <functional>
#include <cstdio>

/**
 * @brief Native function object, holds a representation of a
 * native function.
*/
struct NativeFuncObj;

/**
 * @brief Upvalue object, holds a representationo of an upvalue
 * (local value inside functions and stack frames)
*/
struct UpvalueObj;

/**
 * @brief Class object, holds the representation of a class
 * in the Elysabettian language.
*/
struct ClassObj;

/**
 * @brief Instance object, holds the represantion of a class
 * instance.
*/
struct InstanceObj;

/**
 * @brief Member function (method) object, holds the representation
 * of a member function in the Elysabettian language.
*/
struct MemberFuncObj;

/**
 * @brief File object, holds the representation of a file object
 * in the Elysabettian language.
*/
struct FileObj;

/**
 * @brief Array object, holds the representation of an array in
 * the Elysabettian language.
*/
struct ArrayObj;

/**
 * @brief Function object, holds the representation of a function
 * in the Elysabettian language.
*/
class FunctionObj;

/**
 * @brief Closure object, holds the representation of a function
 * closure in the Elysabettian language.
*/
class ClosureObj;

/**
 * @brief Compiler turns the high-level Elysabettian language into
 * a more manageable bytecode, which is then executed by the language's
 * runtime.
*/
class Compiler;

/**
 * @brief Parser parses the language and converts it into an abstract
 * syntax representation of statements and expressions, used by the
 * compiler to emit the correct bytecode.
*/
class Parser;

/**
 * @brief Executes the bytecode on the target architecture.
*/
class VirtualMachine;

/**
 * @brief Elysabettian language function.
*/
using Func = std::shared_ptr<FunctionObj>;

/**
 * @brief Runtime native function.
*/
using NativeFunc = std::shared_ptr<NativeFuncObj>;

/**
 * @brief Function closure.
*/
using Closure = std::shared_ptr<ClosureObj>;

/**
 * @brief Local upvalue (local variable).
*/
using Upvalue = std::shared_ptr<UpvalueObj>;

/**
 * @brief Elysabettian language class.
*/
using Class = std::shared_ptr<ClassObj>;

/**
 * @brief Elysabettian object instance.
*/
using Instance = std::shared_ptr<InstanceObj>;

/**
 * @brief Elysabettian function encapsulated in class (method).
*/
using MemberFunc = std::shared_ptr<MemberFuncObj>;

/**
 * @brief Elysabettian file object.
*/
using File = std::shared_ptr<FileObj>;

/**
 * @brief Elysabettian array object.
*/
using Array = std::shared_ptr<ArrayObj>;

/**
 * @brief Elysabettian language value.
*/
using Value = std::variant<double, bool, std::monostate,
                           std::string, Func, NativeFunc,
                           Closure, Upvalue, Class, Instance,
                           MemberFunc, File, Array, FILE*>;

/**
 * @brief Memory chunk with support for different operation.
 * This can be perceived as the "RAM" of the VM.
*/
class Chunk {
    std::vector<uint8_t> code;
    std::vector<Value> constants;
    std::vector<int> lines;

public:
    uint8_t get_code(int offset) const { return code[offset]; };
    void set_code(int offset, uint8_t value) { code[offset] = value; }
    const Value& get_constant(int constant) const { return constants[constant]; };
    void write(uint8_t byte, int line);
    void write(Opcode opcode, int line);
    size_t add_constant(Value value);
    int disas_instruction(int offset);
    void disassemble(const std::string& name);
    int get_line(int instruction) { return lines[instruction]; }
    int count() { return static_cast<int>(code.size()); }
};

/**
 * @brief Native Function type.
*/
using NativeFn = std::function<Value(int, std::vector<Value>::iterator)>;

/**
 * @brief Native function object, holds a representation of a
 * native function.
*/
struct NativeFuncObj {
    NativeFn function;
};

/**
 * @brief Upvalue object, holds a representationo of an upvalue
 * (local value inside functions and stack frames)
*/
struct UpvalueObj {
    Value* location;
    Value closed;
    Upvalue next;
    UpvalueObj(Value* slot): location(slot), closed(std::monostate()), next(nullptr) {}
};

/**
 * @brief Class object, holds the representation of a class
 * in the Elysabettian language.
*/
struct ClassObj {
    std::string name;
    std::unordered_map<std::string, Closure> methods;
    explicit ClassObj(std::string name): name(name) {}
};

/**
 * @brief Instance object, holds the represantion of a class
 * instance.
*/
struct InstanceObj {
    Class class_value;
    std::unordered_map<std::string, Value> fields;
    explicit InstanceObj(Class klass): class_value(klass) {}
};

/**
 * @brief Member function (method) object, holds the representation
 * of a member function in the Elysabettian language.
*/
struct MemberFuncObj {
    Instance receiver;
    Closure method;
    explicit MemberFuncObj(Instance receiver, Closure method)
        : receiver(receiver), method(method) {}
};

/**
 * @brief Function object, holds the representation of a function
 * in the Elysabettian language.
*/
class FunctionObj {
private:
    int arity;
    int upvalue_count = 0;
    std::string name;
    Chunk chunk;

public:
    FunctionObj(int arity, const std::string& name)
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
    friend VirtualMachine;
    friend Chunk;
    friend ClosureObj;
};

/**
 * @brief Closure object, holds the representation of a function
 * closure in the Elysabettian language.
*/
class ClosureObj {
public:
    Func function;
    std::vector<Upvalue> upvalues;
    explicit ClosureObj(Func function): function(function)
    {
        upvalues.resize(function->upvalue_count, nullptr);
    };
};

/**
 * @brief File object, holds the representation of a file object
 * in the Elysabettian language.
*/
struct FileObj {
    const std::string path;
    std::FILE* file;

    FileObj(const std::string& path, const std::string& mode)
        : path(path), file(fopen(path.c_str(), mode.c_str()))
    {}

    inline bool is_open() const { return file != nullptr; }

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

    ~FileObj()
    {
        close();
    }
};

/**
 * @brief Array object, holds the representation of an array in
 * the Elysabettian language.
*/
struct ArrayObj {
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
    void operator()(const NativeFunc& f) const { std::cout << "<native fn>"; }
    void operator()(const Closure& c) const { std::cout << Value(c->function); }
    void operator()(const Upvalue& u) const { std::cout << "upvalue"; }
    void operator()(const Class& c) const { std::cout << c->name; }
    void operator()(const Instance& i) const { std::cout << i->class_value->name << " instance"; }
    void operator()(const MemberFunc& m) const { std::cout << Value(m->method->function); }
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
        std::cout << "[ ";
        for (size_t i = 0; i < a->values.size(); ++i) {
            if (i < (a->values.size() - 1))
                std::cout << a->values[i] << ", ";
            else
                std::cout << a->values[i];
        }
        std::cout << " ]";
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
