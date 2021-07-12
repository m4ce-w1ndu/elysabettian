//
//  Compiler.hpp
//  CElysabettian
//
//  Created by Simone Rolando on 11/07/21.
//

#ifndef COMPILER_HPP
#define COMPILER_HPP

#include "../Lexer/Tokenizer.hpp"
#include "Value.hpp"

#include <iostream>
#include <memory>
#include <optional>

enum class Precedence {
    NONE,
    ASSIGNMENT,  // =
    OR,          // or
    AND,         // and
    EQUALITY,    // == !=
    COMPARISON,  // < > <= >=
    TERM,        // + -
    FACTOR,      // * /
    UNARY,       // ! - +
    CALL,        // . () []
    PRIMARY
};

class Parser;

typedef void (Parser::*ParseFn)(bool canAssign);

struct ParseRule {
    std::function<void(bool)> prefix;
    std::function<void(bool)> infix;
    Precedence precedence;
};

struct Local {
    std::string name;
    int depth;
    bool isCaptured;
    Local(std::string name, int depth): name(name), depth(depth), isCaptured(false) {};
};

class Upvalue {
public:
    uint8_t index;
    bool isLocal;
    explicit Upvalue(uint8_t index, bool isLocal)
        : index(index), isLocal(isLocal) {}
};

class Parser;

typedef enum {
    TYPE_FUNCTION, TYPE_INITIALIZER, TYPE_METHOD, TYPE_SCRIPT
} FunctionType;

class Compiler {
    Parser* parser;

    FunctionType type;
    Func function;

    std::unique_ptr<Compiler> enclosing;
    
    std::vector<Local> locals;
    std::vector<Upvalue> upvalues;
    int scopeDepth = 0;

public:
    explicit Compiler(Parser* parser, FunctionType type, std::unique_ptr<Compiler> enclosing);
    void AddLocal(const std::string& name);
    void DeclareVariable(const std::string& name);
    void MarkInitialized();
    int ResolveLocal(const std::string& name);
    int ResolveUpvalue(const std::string& name);
    int AddUpvalue(uint8_t index, bool isLocal);
    void BeginScope();
    void EndScope();
    bool IsLocal();

    friend Parser;
};

class ClassCompiler {
    std::unique_ptr<ClassCompiler> enclosing;
    bool hasSuperclass;
public:
    explicit ClassCompiler(std::unique_ptr<ClassCompiler> enclosing);
    friend Parser;
};

class Parser {
    Token previous;
    Token current;
    Tokenizer scanner;
    std::unique_ptr<Compiler> compiler;
    std::unique_ptr<ClassCompiler> classCompiler;
    
    bool hadError;
    bool panicMode;
    
    void Advance();
    void Consume(TokenType type, const std::string& message);
    bool Check(TokenType type);
    bool Match(TokenType type);
    
    void Emit(uint8_t byte);
    void Emit(OpCode op);
    void Emit(OpCode op, uint8_t byte);
    void Emit(OpCode op1, OpCode op2);
    void EmitLoop(int loopStart);
    int EmitJump(OpCode op);
    void EmitReturn();
    uint8_t MakeConstant(Value value);
    void EmitConstant(Value value);
    void PatchJump(int offset);
    
    Func EndCompiler();
    
    void Binary(bool canAssign);
    void Call(bool canAssign);
    void Dot(bool canAssign);
    void Literal(bool canAssign);
    void Grouping(bool canAssign);
    void Number(bool canAssign);
    void Or(bool canAssign);
    void String(bool canAssign);
    void NamedVariable(const std::string& name, bool canAssign);
    void Variable(bool canAssign);
    void Super(bool canAssign);
    void This(bool canAssign);
    void And(bool canAssign);
    void Unary(bool canAssign);
    ParseRule& GetRule(TokenType type);
    void ParsePrecedence(Precedence precedence);
    int IdentifierConstant(const std::string& name);
    uint8_t ParseVariable(const std::string& errorMessage);
    void DefineVariable(uint8_t global);
    uint8_t ArgumentList();
    void Expression();
    void Block();
    void Function(FunctionType type);
    void Method();
    void ClassDeclaration();
    void FuncDeclaration();
    void VarDeclaration();
    void ExpressionStatement();
    void ForStatement();
    void IfStatement();
    void Declaration();
    void Statement();
    void PrintStatement();
    void ReturnStatement();
    void WhileStatement();
    void Sync();

    void ErrorAt(const Token& token, const std::string& message);
    
    void Error(const std::string& message)
    {
        ErrorAt(previous, message);
    };
    
    void ErrorAtCurrent(const std::string& message)
    {
        ErrorAt(current, message);
    };
    
    friend Compiler;
    
public:
    Parser(const std::string& source);
    Chunk& CurrentChunk()
    {
        return compiler->function->GetChunk();
    }
    std::optional<Func> Compile();
};

#endif /* compiler_hpp */
