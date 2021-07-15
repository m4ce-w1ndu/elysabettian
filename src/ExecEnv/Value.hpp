//
//  Value.hpp
//  CElysabettian
//
//  Created by Simone Rolando on 11/07/21.
//

#ifndef VALUE_HPP
#define VALUE_HPP

#include "../Application/Common.hpp"
#include "Opcodes.hpp"

#include <variant>
#include <memory>
#include <unordered_map>
#include <map>
#include <functional>
#include <fstream>

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

using Value = std::variant<double, bool, std::monostate, std::string, Func, NativeFunction, Closure, UpvalueValue, ClassValue, InstanceValue, BoundMethodValue, File, Array>;

class Chunk {
    std::vector<uint8_t> code;
    std::vector<Value> constants;
    std::vector<int> lines;

public:
    uint8_t GetCode(int offset) const { return code[offset]; };
    void SetCode(int offset, uint8_t value) { code[offset] = value; }
    const Value& GetConstant(int constant) const { return constants[constant]; };
    void Write(uint8_t byte, int line);
    void Write(OpCode opcode, int line);
    unsigned long AddConstant(Value value);
    int DisasIntruction(int offset);
    void Disassemble(const std::string& name);
    int GetLine(int instruction) { return lines[instruction]; }
    int Count() { return static_cast<int>(code.size()); }
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
    std::unordered_map<std::string, Closure> memberFuncs;
    explicit ClassObject(std::string name): name(name) {}
};

struct InstanceObject {
    ClassValue classValue;
    std::unordered_map<std::string, Value> fields;
    explicit InstanceObject(ClassValue klass): classValue(klass) {}
};

struct MemberFuncObject {
    InstanceValue receiver;
    Closure memberFunc;
    explicit MemberFuncObject(InstanceValue receiver, Closure method)
        : receiver(receiver), memberFunc(method) {}
};

class FunctionObject {
private:
    int arity;
    int upvalueCount = 0;
    std::string name;
    Chunk chunk;

public:
    FunctionObject(int arity, const std::string& name)
        : arity(arity), name(name), chunk(Chunk()) {}
    
    const std::string& GetName() const
    {
        return name;
    }

    bool operator==(const Func& rhs) const
    {
        return false;
    }
    
    Chunk& GetChunk()
    {
        return chunk;
    }
    
    uint8_t GetCode(int offset)
    {
        return chunk.GetCode(offset);
    }
    
    const Value& GetConst(int constant) const
    {
        return chunk.GetConstant(constant);
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
        upvalues.resize(function->upvalueCount, nullptr);
    };
};

struct FileObject {
    const std::string path;
    std::fstream file;

    inline bool IsOpen() { return file.is_open(); }

    FileObject(const std::string& path, const std::ios::openmode mode)
        : path(path), file(std::fstream(path, mode))
    {}

    ~FileObject()
    {
        file.close();
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
        if (f->GetName().empty()) {
            std::cout << "<script>";
        } else {
            std::cout << "<fn " << f->GetName() << ">";
        }
    }
    void operator()(const NativeFunction& f) const { std::cout << "<native fn>"; }
    void operator()(const Closure& c) const { std::cout << Value(c->function); }
    void operator()(const UpvalueValue& u) const { std::cout << "upvalue"; }
    void operator()(const ClassValue& c) const { std::cout << c->name; }
    void operator()(const InstanceValue& i) const { std::cout << i->classValue->name << " instance"; }
    void operator()(const BoundMethodValue& m) const { std::cout << Value(m->memberFunc->function); }
    void operator()(const File& f) const
    {
        std::cout << "path: " << f->path << ", open: "
            << std::boolalpha << f->IsOpen();
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

inline bool IsFalse(const Value& v)
{
    return std::visit(FalseVisitor(), v);
}

#endif /* value_hpp */
