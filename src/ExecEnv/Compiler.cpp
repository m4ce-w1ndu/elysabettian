//
//  Compiler.cpp
//  CElysabettian
//
//  Created by Simone Rolando on 11/07/21.
//

#include "Compiler.hpp"

Compiler::Compiler(Parser* parser, FunctionType type, std::unique_ptr<Compiler> enclosing)
    : parser(parser), type(type), function(std::make_shared<FunctionObject>(0, "")), enclosing(std::move(enclosing))
{
        locals.emplace_back(Local(type == TYPE_FUNCTION ? "" : "this", 0));
        if (type != TYPE_SCRIPT) {
            function->name = parser->previous.GetText();
        }
};

void Compiler::AddLocal(const std::string& name)
{
    if (locals.size() == UINT8_COUNT) {
        parser->Error("Too many local variables in function.");
        return;
    }
    locals.emplace_back(Local(name, -1));
}

void Compiler::DeclareVariable(const std::string& name)
{
    if (scopeDepth == 0) return;
    
    for (size_t i = locals.size() - 1; i >= 0; i--) {
        if (locals[i].depth != -1 && locals[i].depth < scopeDepth) break;
        if (locals[i].name == name) {
            parser->Error("Already a variable with this name in this scope.");
        }
    }
    
    AddLocal(name);
}

void Compiler::MarkInitialized()
{
    if (scopeDepth == 0) return;
    locals.back().depth = scopeDepth;
}

int Compiler::ResolveLocal(const std::string& name)
{
    for (long i = locals.size() - 1; i >=0; i--) {
        if (locals[i].name == name) {
            if (locals[i].depth == -1) {
                parser->Error("Can't read local variable in its own initializer.");
            }
            return static_cast<int>(i);
        }
    }
    
    return -1;
}

int Compiler::ResolveUpvalue(const std::string& name)
{
    if (enclosing == nullptr) return -1;
    
    int local = enclosing->ResolveLocal(name);
    if (local != -1) {
        enclosing->locals[local].isCaptured = true;
        return AddUpvalue(static_cast<uint8_t>(local), true);
    }
    
    int upvalue = enclosing->ResolveUpvalue(name);
    if (upvalue != -1) {
        return AddUpvalue(static_cast<uint8_t>(upvalue), false);
    }
    
    return -1;
}

int Compiler::AddUpvalue(uint8_t index, bool isLocal)
{
    for (long i = 0; i < upvalues.size(); i++) {
        if (upvalues[i].index == index && upvalues[i].isLocal == isLocal) {
            return static_cast<int>(i);
        }
    }
    
    if (upvalues.size() == UINT8_COUNT) {
        parser->Error("Too many closure variables in function.");
        return 0;
    }

    upvalues.emplace_back(Upvalue(index, isLocal));
    auto upvalueCount = static_cast<int>(upvalues.size());
    function->upvalueCount = upvalueCount;
    return upvalueCount - 1;
}

void Compiler::BeginScope()
{
    scopeDepth++;
}

void Compiler::EndScope()
{
    scopeDepth--;
    while (!locals.empty() && locals.back().depth > scopeDepth) {
        if (locals.back().isCaptured) {
            parser->Emit(OpCode::CLOSE_UPVALUE);
        } else {
            parser->Emit(OpCode::POP);
        }
        locals.pop_back();
    }
}

bool Compiler::IsLocal()
{
    return scopeDepth > 0;
}

ClassCompiler::ClassCompiler(std::unique_ptr<ClassCompiler> enclosing)
    : enclosing(std::move(enclosing)), hasSuperclass(false) {};

Parser::Parser(const std::string& source) :
    previous(Token(TokenType::_EOF, source, 0)),
    current(Token(TokenType::_EOF, source, 0)),
    scanner(Tokenizer(source)),
    classCompiler(nullptr),
    hadError(false), panicMode(false)
{
    compiler = std::make_unique<Compiler>(this, TYPE_SCRIPT, nullptr);
    Advance();
}

std::optional<Func> Parser::Compile()
{
    while (!Match(TokenType::_EOF)) {
        Declaration();
    }
    auto function = EndCompiler();
    
    if (hadError) {
        return std::nullopt;
    } else {
        return std::make_optional(function);
    }
}

void Parser::Advance()
{
    previous = current;
    
    while (true) {
        current = scanner.ScanToken();
        if (current.GetType() != TokenType::ERROR) break;
        
        ErrorAtCurrent(std::string(current.GetText()));
    }
}

void Parser::Consume(TokenType type, const std::string& message)
{
    if (current.GetType() == type) {
        Advance();
        return;
    }
    
    ErrorAtCurrent(message);
}

bool Parser::Check(TokenType type)
{
    return current.GetType() == type;
}

bool Parser::Match(TokenType type)
{
    if (!Check(type)) return false;
    Advance();
    return true;
}

void Parser::Emit(uint8_t byte)
{
    CurrentChunk().Write(byte, previous.GetLine());
}

void Parser::Emit(OpCode op)
{
    CurrentChunk().Write(op, previous.GetLine());
}

void Parser::Emit(OpCode op, uint8_t byte)
{
    Emit(op);
    Emit(byte);
}

void Parser::Emit(OpCode op1, OpCode op2)
{
    Emit(op1);
    Emit(op2);
}

void Parser::EmitLoop(int loopStart)
{
    Emit(OpCode::LOOP);
    
    int offset = CurrentChunk().Count() - loopStart + 2;
    if (offset > UINT16_MAX) { Error("Loop body too large."); }
    
    Emit((offset >> 8) & 0xff);
    Emit(offset & 0xff);
}

int Parser::EmitJump(OpCode op)
{
    Emit(op);
    Emit(0xff);
    Emit(0xff);
    return CurrentChunk().Count() - 2;
}

void Parser::EmitReturn()
{
    if (compiler->type == TYPE_INITIALIZER) {
        Emit(OpCode::GET_LOCAL, 0);
    } else {
        Emit(OpCode::NULLOP);
    }
    Emit(OpCode::RETURN);
}

uint8_t Parser::MakeConstant(Value value)
{
    auto constant = CurrentChunk().AddConstant(value);
    if (constant > UINT8_MAX) {
        Error("Too many constants in one chunk.");
        return 0;
    }
    
    return (uint8_t)constant;
}

void Parser::EmitConstant(Value value)
{
    Emit(OpCode::CONSTANT, MakeConstant(value));
}

void Parser::PatchJump(int offset)
{
    // -2 to adjust for the bytecode for the jump offset itself.
    int jump = CurrentChunk().Count() - offset - 2;
    
    if (jump > UINT16_MAX) {
        Error("Too much code to jump over.");
    }
    
    CurrentChunk().SetCode(offset, (jump >> 8) & 0xff);
    CurrentChunk().SetCode(offset + 1, jump & 0xff);
}

Func Parser::EndCompiler()
{
    EmitReturn();
    
    auto function = compiler->function;
    
#ifdef DEBUG_PRINT_CODE
    if (!hadError) {
        auto name = function->GetName().empty() ? "<main>" : function->GetName();
        CurrentChunk().Disassemble(name);
    }
#endif

    return function;
}

void Parser::Binary(bool canAssign)
{
    // Remember the operator.
    auto operatorType = previous.GetType();
    
    // Compile the right operand.
    auto rule = GetRule(operatorType);
    ParsePrecedence(Precedence(static_cast<int>(rule.precedence) + 1));
    
    // Emit the operator instruction.
    switch (operatorType) {
        case TokenType::EXCL_EQUAL:    Emit(OpCode::EQUAL, OpCode::NOT); break;
        case TokenType::EQUAL_EQUAL:   Emit(OpCode::EQUAL); break;
        case TokenType::GREATER:       Emit(OpCode::GREATER); break;
        case TokenType::GREATER_EQUAL: Emit(OpCode::LESS, OpCode::NOT); break;
        case TokenType::LESS:          Emit(OpCode::LESS); break;
        case TokenType::LESS_EQUAL:    Emit(OpCode::GREATER, OpCode::NOT); break;
        case TokenType::PLUS:          Emit(OpCode::ADD); break;
        case TokenType::MINUS:         Emit(OpCode::SUBTRACT); break;
        case TokenType::STAR:          Emit(OpCode::MULTIPLY); break;
        case TokenType::SLASH:         Emit(OpCode::DIVIDE); break;
        case TokenType::BW_OR:         Emit(OpCode::BW_OR); break;
        case TokenType::BW_AND:        Emit(OpCode::BW_AND); break;
        case TokenType::BW_XOR:        Emit(OpCode::BW_XOR); break;
        default:
            return; // Unreachable.
    }
}

void Parser::Call(bool canAssign)
{
    auto argCount = ArgumentList();
    Emit(OpCode::CALL, argCount);
}

void Parser::Dot(bool canAssign)
{
    Consume(TokenType::IDENTIFIER, "Expected property name after '.'.");
    auto name = IdentifierConstant(std::string(previous.GetText()));
    
    if (canAssign && Match(TokenType::EQUAL)) {
        Expression();
        Emit(OpCode::SET_PROPERTY, name);
    } else if (Match(TokenType::OPEN_PAREN)) {
        auto argCount = ArgumentList();
        Emit(OpCode::INVOKE, name);
        Emit(argCount);
    } else {
        Emit(OpCode::GET_PROPERTY, name);
    }
}

void Parser::Literal(bool canAssign)
{
    switch (previous.GetType()) {
        case TokenType::FALSE: Emit(OpCode::FALSE); break;
        case TokenType::NULLVAL:   Emit(OpCode::NULLOP); break;
        case TokenType::TRUE:  Emit(OpCode::TRUE); break;
        default:
            return; // Unreachable.
    }
}

void Parser::Grouping(bool canAssign)
{
    Expression();
    Consume(TokenType::CLOSE_PAREN, "Expected ')' after expression.");
}

void Parser::Number(bool canAssign)
{
    Value value = std::stod(std::string(previous.GetText()));
    EmitConstant(value);
}

void Parser::Or(bool canAssign)
{
    int elseJump = EmitJump(OpCode::JUMP_IF_FALSE);
    int endJump = EmitJump(OpCode::JUMP);
    
    PatchJump(elseJump);
    Emit(OpCode::POP);
    
    ParsePrecedence(Precedence::OR);
    PatchJump(endJump);
}

void Parser::String(bool canAssign)
{
    auto str = previous.GetText();
    str.remove_prefix(1);
    str.remove_suffix(1);
    EmitConstant(std::string(str));
}

void Parser::NamedVariable(const std::string& name, bool canAssign)
{
    OpCode getOp, setOp;
    auto arg = compiler->ResolveLocal(name);
    if (arg != -1) {
        getOp = OpCode::GET_LOCAL;
        setOp = OpCode::SET_LOCAL;
    } else if ((arg = compiler->ResolveUpvalue(name)) != -1) {
        getOp = OpCode::GET_UPVALUE;
        setOp = OpCode::SET_UPVALUE;
    } else {
        arg = IdentifierConstant(name);
        getOp = OpCode::GET_GLOBAL;
        setOp = OpCode::SET_GLOBAL;
    }
    
    if (canAssign && Match(TokenType::EQUAL)) {
        Expression();
        Emit(setOp, (uint8_t)arg);
    } else {
        Emit(getOp, (uint8_t)arg);
    }
}

void Parser::Variable(bool canAssign)
{
    NamedVariable(std::string(previous.GetText()), canAssign);
}

void Parser::Super(bool canAssign)
{
    if (classCompiler == nullptr) {
        Error("'super' cannot be used outside of a class.");
    } else if (!classCompiler->hasSuperclass) {
        Error("'super' cannot be called in a class without superclass.");
    }
    
    Consume(TokenType::DOT, "Expected '.' after 'super'.");
    Consume(TokenType::IDENTIFIER, "Expected superclass method name.");
    auto name = IdentifierConstant(std::string(previous.GetText()));
    
    NamedVariable("this", false);
    NamedVariable("super", false);
    Emit(OpCode::GET_SUPER, name);
}

void Parser::This(bool canAssign)
{
    if (classCompiler == nullptr) {
        Error("'this' cannot be outside of a class.");
        return;
    }
    Variable(false);
}

void Parser::And(bool canAssign)
{
    int endJump = EmitJump(OpCode::JUMP_IF_FALSE);
    
    Emit(OpCode::POP);
    ParsePrecedence(Precedence::AND);
    
    PatchJump(endJump);
}

void Parser::Unary(bool canAssign)
{
    auto operatorType = previous.GetType();
    
    // Compile the operand.
    ParsePrecedence(Precedence::UNARY);
    
    // Emit the operator instruciton.
    switch (operatorType) {
        case TokenType::EXCL: Emit(OpCode::NOT); break;
        case TokenType::MINUS: Emit(OpCode::NEGATE); break;
        case TokenType::BW_NOT: Emit(OpCode::BW_NOT); break;
            
        default:
            return; // Unreachable.
    }
}

ParseRule& Parser::GetRule(TokenType type)
{
    auto grouping = [this](bool canAssign) { this->Grouping(canAssign); };
    auto unary = [this](bool canAssign) { this->Unary(canAssign); };
    auto binary = [this](bool canAssign) { this->Binary(canAssign); };
    auto call = [this](bool canAssign) { this->Call(canAssign); };
    auto dot = [this](bool canAssign) { this->Dot(canAssign); };
    auto number = [this](bool canAssign) { this->Number(canAssign); };
    auto string = [this](bool canAssign) { this->String(canAssign); };
    auto literal = [this](bool canAssign) { this->Literal(canAssign); };
    auto variable = [this](bool canAssign) { this->Variable(canAssign); };
    auto super_ = [this](bool canAssign) { this->Super(canAssign); };
    auto this_ = [this](bool canAssign) { this->This(canAssign); };
    auto and_ = [this](bool canAssign) { this->And(canAssign); };
    auto or_ = [this](bool canAssign) { this->Or(canAssign); };
    
    static ParseRule rules[] = {
        { grouping,    call,       Precedence::CALL },       // TOKEN_LEFT_PAREN
        { nullptr,     nullptr,    Precedence::NONE },       // TOKEN_RIGHT_PAREN
        { nullptr,     nullptr,    Precedence::NONE },       // TOKEN_LEFT_BRACE
        { nullptr,     nullptr,    Precedence::NONE },       // TOKEN_RIGHT_BRACE
        { nullptr,     nullptr,    Precedence::NONE },       // TOKEN_COMMA
        { nullptr,     dot,        Precedence::CALL },       // TOKEN_DOT
        { unary,       binary,     Precedence::TERM },       // TOKEN_MINUS
        { nullptr,     binary,     Precedence::TERM },       // TOKEN_PLUS
        { nullptr,     nullptr,    Precedence::NONE },       // TOKEN_SEMICOLON
        { nullptr,     binary,     Precedence::FACTOR },     // TOKEN_SLASH
        { nullptr,     binary,     Precedence::FACTOR },     // TOKEN_STAR
        { unary,       nullptr,    Precedence::NONE },       // TOKEN_BANG
        { nullptr,     binary,     Precedence::EQUALITY },   // TOKEN_BANG_EQUAL
        { nullptr,     nullptr,    Precedence::NONE },       // TOKEN_EQUAL
        { nullptr,     binary,     Precedence::EQUALITY },   // TOKEN_EQUAL_EQUAL
        { nullptr,     binary,     Precedence::COMPARISON }, // TOKEN_GREATER
        { nullptr,     binary,     Precedence::COMPARISON }, // TOKEN_GREATER_EQUAL
        { nullptr,     binary,     Precedence::COMPARISON }, // TOKEN_LESS
        { nullptr,     binary,     Precedence::COMPARISON }, // TOKEN_LESS_EQUAL
        { variable,    nullptr,    Precedence::NONE },       // TOKEN_IDENTIFIER
        { string,      nullptr,    Precedence::NONE },       // TOKEN_STRING
        { number,      nullptr,    Precedence::NONE },       // TOKEN_NUMBER
        { nullptr,     and_,       Precedence::AND },        // TOKEN_AND
        { nullptr,     nullptr,    Precedence::NONE },       // TOKEN_CLASS
        { nullptr,     nullptr,    Precedence::NONE },       // TOKEN_ELSE
        { literal,     nullptr,    Precedence::NONE },       // TOKEN_FALSE
        { nullptr,     nullptr,    Precedence::NONE },       // TOKEN_FUN
        { nullptr,     nullptr,    Precedence::NONE },       // TOKEN_FOR
        { nullptr,     nullptr,    Precedence::NONE },       // TOKEN_IF
        { literal,     nullptr,    Precedence::NONE },       // TOKEN_NIL
        { nullptr,     or_,        Precedence::OR },         // TOKEN_OR
        { nullptr,     nullptr,    Precedence::NONE },       // TOKEN_PRINT
        { nullptr,     nullptr,    Precedence::NONE },       // TOKEN_RETURN
        { super_,      nullptr,    Precedence::NONE },       // TOKEN_SUPER
        { this_,       nullptr,    Precedence::NONE },       // TOKEN_THIS
        { literal,     nullptr,    Precedence::NONE },       // TOKEN_TRUE
        { nullptr,     nullptr,    Precedence::NONE },       // TOKEN_VAR
        { nullptr,     nullptr,    Precedence::NONE },       // TOKEN_WHILE
        { nullptr,     nullptr,    Precedence::NONE },       // TOKEN_ERROR
        { nullptr,     nullptr,    Precedence::NONE },       // TOKEN_EOF
    };
    
    return rules[static_cast<int>(type)];
}

void Parser::ParsePrecedence(Precedence precedence)
{
    Advance();
    auto prefixRule = GetRule(previous.GetType()).prefix;
    if (prefixRule == nullptr) {
        Error("Expected expression.");
        return;
    }
    
    auto canAssign = precedence <= Precedence::ASSIGNMENT;
    prefixRule(canAssign);
    
    while (precedence <= GetRule(current.GetType()).precedence) {
        Advance();
        auto infixRule = GetRule(previous.GetType()).infix;
        infixRule(canAssign);
    }
    
    if (canAssign && Match(TokenType::EQUAL)) {
        Error("Invalid assignment target.");
        Expression();
    }
}

int Parser::IdentifierConstant(const std::string& name)
{
    return MakeConstant(name);
}

uint8_t Parser::ParseVariable(const std::string& errorMessage)
{
    Consume(TokenType::IDENTIFIER, errorMessage);
    
    compiler->DeclareVariable(std::string(previous.GetText()));
    if (compiler->IsLocal()) return 0;
    
    return IdentifierConstant(std::string(previous.GetText()));
}

void Parser::DefineVariable(uint8_t global)
{
    if (compiler->IsLocal()) {
        compiler->MarkInitialized();
        return;
    }
    
    Emit(OpCode::DEFINE_GLOBAL, global);
}

uint8_t Parser::ArgumentList()
{
    uint8_t argCount = 0;
    if (!Check(TokenType::CLOSE_PAREN)) {
        do {
            Expression();
            if (argCount == 255) {
                Error("A function cannot have more than 255 arguments.");
            }
            argCount++;
        } while (Match(TokenType::COMMA));
    }
    Consume(TokenType::CLOSE_PAREN, "Expected ')' after arguments.");
    return argCount;
}

void Parser::Expression()
{
    ParsePrecedence(Precedence::ASSIGNMENT);
}

void Parser::Block()
{
    while (!Check(TokenType::CLOSE_CURLY) && !Check(TokenType::_EOF)) {
        Declaration();
    }
    
    Consume(TokenType::CLOSE_CURLY, "Expected '}' after block.");
}

void Parser::Function(FunctionType type)
{
    compiler = std::make_unique<Compiler>(this, type, std::move(compiler));
    compiler->BeginScope();

    Consume(TokenType::OPEN_PAREN, "Expected '(' after function name.");
    if (!Check(TokenType::CLOSE_PAREN)) {
        do {
            compiler->function->arity++;
            if (compiler->function->arity > 255)
                ErrorAtCurrent("A function cannot have more than 255 parameters.");
            auto constant = ParseVariable("Expected parameter name.");
            DefineVariable(constant);
        } while (Match(TokenType::COMMA));
    }
    Consume(TokenType::CLOSE_PAREN, "Expected ')' after parameters.");
    Consume(TokenType::OPEN_CURLY, "Expected '{' before function body.");
    Block();
    
    auto function = EndCompiler();
    auto newCompiler = std::move(compiler);
    compiler = std::move(newCompiler->enclosing);

    Emit(OpCode::CLOSURE, MakeConstant(function));
    
    for (const auto& upvalue : newCompiler->upvalues) {
        Emit(upvalue.isLocal ? 1 : 0);
        Emit(upvalue.index);
    }
}

void Parser::Method()
{
    Consume(TokenType::IDENTIFIER, "Expected method name.");
    auto constant = IdentifierConstant(std::string(previous.GetText()));
    auto type = previous.GetText() == "init" ? TYPE_INITIALIZER : TYPE_METHOD;
    Function(type);
    Emit(OpCode::METHOD, constant);
}

void Parser::ClassDeclaration()
{
    Consume(TokenType::IDENTIFIER, "Expected class name.");
    auto className = std::string(previous.GetText());
    auto nameConstant = IdentifierConstant(className);
    compiler->DeclareVariable(std::string(previous.GetText()));
    
    Emit(OpCode::CLASS, nameConstant);
    DefineVariable(nameConstant);
    
    classCompiler = std::make_unique<ClassCompiler>(std::move(classCompiler));
    
    if (Match(TokenType::LESS)) {
        Consume(TokenType::IDENTIFIER, "Expected superclass name.");
        Variable(false);
        
        if (className == previous.GetText()) {
            Error("A class cannot inherit from itself.");
        }
        
        compiler->BeginScope();
        compiler->AddLocal("super");
        DefineVariable(0);
        
        NamedVariable(className, false);
        Emit(OpCode::INHERIT);
        classCompiler->hasSuperclass = true;
    }
    
    NamedVariable(className, false);
    Consume(TokenType::OPEN_CURLY, "Expected '{' before class body.");
    while (!Check(TokenType::CLOSE_CURLY) && !Check(TokenType::_EOF)) {
        Method();
    }
    Consume(TokenType::CLOSE_CURLY, "Expected '}' after class body.");
    Emit(OpCode::POP);
    
    if (classCompiler->hasSuperclass) {
        compiler->EndScope();
    }
    
    classCompiler = std::move(classCompiler->enclosing);
}

void Parser::FuncDeclaration()
{
    auto global = ParseVariable("Expected function name.");
    compiler->MarkInitialized();
    Function(TYPE_FUNCTION);
    DefineVariable(global);
}

void Parser::VarDeclaration()
{
    auto global = ParseVariable("Expected variable name.");

    if (Match(TokenType::EQUAL)) {
        Expression();
    } else {
        Emit(OpCode::NULLOP);
    }
    Consume(TokenType::SEMICOLON, "Expected ';' after variable declaration.");
    
    DefineVariable(global);
}

void Parser::ExpressionStatement()
{
    Expression();
    Emit(OpCode::POP);
    Consume(TokenType::SEMICOLON, "Expected ';' after expression.");
}

void Parser::ForStatement()
{
    compiler->BeginScope();
    
    Consume(TokenType::OPEN_PAREN, "Expected '(' after 'for'.");
    if (Match(TokenType::VAR)) {
        VarDeclaration();
    } else if (Match(TokenType::SEMICOLON)) {
        // no initializer.
    } else {
        ExpressionStatement();
    }
    
    int loopStart = CurrentChunk().Count();
    
    int exitJump = -1;
    if (!Match(TokenType::SEMICOLON)) {
        Expression();
        Consume(TokenType::SEMICOLON, "Expected ';' after loop condition.");
        
        // Jump out of the loop if the condition is false.
        exitJump = EmitJump(OpCode::JUMP_IF_FALSE);
        Emit(OpCode::POP);
    }

    if (!Match(TokenType::CLOSE_PAREN)) {
        int bodyJump = EmitJump(OpCode::JUMP);
        
        int incrementStart = CurrentChunk().Count();
        Expression();
        Emit(OpCode::POP);
        Consume(TokenType::CLOSE_PAREN, "Expected ')' after for clauses.");
        
        EmitLoop(loopStart);
        loopStart = incrementStart;
        PatchJump(bodyJump);
    }
    
    Statement();
    
    EmitLoop(loopStart);
    
    if (exitJump != -1) {
        PatchJump(exitJump);
        Emit(OpCode::POP); // Condition.
    }

    compiler->EndScope();
}

void Parser::IfStatement()
{
    Consume(TokenType::OPEN_PAREN, "Expected '(' after 'if'.");
    Expression();
    Consume(TokenType::CLOSE_PAREN, "Expected ')' after condition.");
    
    int thenJump = EmitJump(OpCode::JUMP_IF_FALSE);
    Emit(OpCode::POP);
    Statement();
    int elseJump = EmitJump(OpCode::JUMP);
    
    PatchJump(thenJump);
    Emit(OpCode::POP);
    if (Match(TokenType::ELSE)) { Statement(); }
    PatchJump(elseJump);
}

void Parser::Declaration()
{
    if (Match(TokenType::CLASS)) {
        ClassDeclaration();
    } else if (Match(TokenType::FUN)) {
        FuncDeclaration();
    } else if (Match(TokenType::VAR)) {
        VarDeclaration();
    } else {
        Statement();
    }
    
    if (panicMode) Sync();
}

void Parser::Statement()
{
    if (Match(TokenType::PRINT)) {
        PrintStatement();
    } else if (Match(TokenType::FOR)) {
        ForStatement();
    } else if (Match(TokenType::IF)) {
        IfStatement();
    } else if (Match(TokenType::RETURN)) {
        ReturnStatement();
    } else if (Match(TokenType::WHILE)) {
        WhileStatement();
    } else if (Match(TokenType::OPEN_CURLY)) {
        compiler->BeginScope();
        Block();
        compiler->EndScope();
    } else {
        ExpressionStatement();
    }
}

void Parser::PrintStatement()
{
    Expression();
    Consume(TokenType::SEMICOLON, "Expected ';' after value.");
    Emit(OpCode::PRINT);
}

void Parser::ReturnStatement()
{
    if (compiler->type == TYPE_SCRIPT) {
        Error("Cannot return from top-level code.");
    }
    
    if (Match(TokenType::SEMICOLON)) {
        EmitReturn();
    } else {
        if (compiler->type == TYPE_INITIALIZER) {
            Error("Cannot return a value from an initializer.");
        }
        
        Expression();
        Consume(TokenType::SEMICOLON, "Expected ';' after return value.");
        Emit(OpCode::RETURN);
    }
}

void Parser::WhileStatement()
{
    int loopStart = CurrentChunk().Count();
    
    Consume(TokenType::OPEN_PAREN, "Expected '(' after 'while'.");
    Expression();
    Consume(TokenType::CLOSE_PAREN, "Expected ')' after condition.");
    
    int exitJump = EmitJump(OpCode::JUMP_IF_FALSE);
    
    Emit(OpCode::POP);
    Statement();
    
    EmitLoop(loopStart);
    
    PatchJump(exitJump);
    Emit(OpCode::POP);
}

void Parser::Sync()
{
    panicMode = false;
    
    while (current.GetType() != TokenType::_EOF) {
        if (previous.GetType() == TokenType::SEMICOLON) return;
        
        switch (current.GetType()) {
            case TokenType::CLASS:
            case TokenType::FUN:
            case TokenType::IF:
            case TokenType::WHILE:
            case TokenType::PRINT:
            case TokenType::RETURN:
                return;
                
            default:
                // Do nothing.
                ;
        }
        
        Advance();
    }
}

void Parser::ErrorAt(const Token& token, const std::string& message)
{
    if (panicMode) return;
    
    panicMode = true;
    
    std::cerr << "[line " << token.GetLine() << "] Error";
    if (token.GetType() == TokenType::_EOF) {
        std::cerr << " at end";
    } else if (token.GetType() == TokenType::ERROR) {
        // Nothing.
    } else {
        std::cerr << " at '" << token.GetText() << "'";
    }
    
    std::cerr << ": " << message << std::endl;
    hadError = true;
}
