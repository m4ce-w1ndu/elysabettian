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
            function->name = parser->previous.get_text();
        }
};

void Compiler::add_local(const std::string& name)
{
    if (locals.size() == UINT8_COUNT) {
        parser->error("Too many local variables in function.");
        return;
    }
    locals.emplace_back(Local(name, -1));
}

void Compiler::declare_variable(const std::string& name)
{
    if (scopeDepth == 0) return;
    
    for (size_t i = locals.size() - 1; i >= 0; i--) {
        if (locals[i].depth != -1 && locals[i].depth < scopeDepth) break;
        if (locals[i].name == name) {
            parser->error("Already a variable with this name in this scope.");
        }
    }
    
    add_local(name);
}

void Compiler::mark_initialized()
{
    if (scopeDepth == 0) return;
    locals.back().depth = scopeDepth;
}

int Compiler::resolve_local(const std::string& name)
{
    for (long i = locals.size() - 1; i >=0; i--) {
        if (locals[i].name == name) {
            if (locals[i].depth == -1) {
                parser->error("Can't read local variable in its own initializer.");
            }
            return static_cast<int>(i);
        }
    }
    
    return -1;
}

int Compiler::resolve_upvalue(const std::string& name)
{
    if (enclosing == nullptr) return -1;
    
    int local = enclosing->resolve_local(name);
    if (local != -1) {
        enclosing->locals[local].is_captured = true;
        return add_upvalue(static_cast<uint8_t>(local), true);
    }
    
    int upvalue = enclosing->resolve_upvalue(name);
    if (upvalue != -1) {
        return add_upvalue(static_cast<uint8_t>(upvalue), false);
    }
    
    return -1;
}

int Compiler::add_upvalue(uint8_t index, bool isLocal)
{
    for (long i = 0; i < upvalues.size(); i++) {
        if (upvalues[i].index == index && upvalues[i].is_local == isLocal) {
            return static_cast<int>(i);
        }
    }
    
    if (upvalues.size() == UINT8_COUNT) {
        parser->error("Too many closure variables in function.");
        return 0;
    }

    upvalues.emplace_back(Upvalue(index, isLocal));
    auto upvalue_count = static_cast<int>(upvalues.size());
    function->upvalue_count = upvalue_count;
    return upvalue_count - 1;
}

void Compiler::begin_scope()
{
    scopeDepth++;
}

void Compiler::end_scope()
{
    scopeDepth--;
    while (!locals.empty() && locals.back().depth > scopeDepth) {
        if (locals.back().is_captured) {
            parser->emit(OpCode::CLOSE_UPVALUE);
        } else {
            parser->emit(OpCode::POP);
        }
        locals.pop_back();
    }
}

bool Compiler::is_local()
{
    return scopeDepth > 0;
}

ClassCompiler::ClassCompiler(std::unique_ptr<ClassCompiler> enclosing)
    : enclosing(std::move(enclosing)), has_superclass(false) {};

Parser::Parser(const std::string& source) :
    previous(Token(TokenType::_EOF, source, 0)),
    current(Token(TokenType::_EOF, source, 0)),
    scanner(Tokenizer(source)),
    class_compiler(nullptr),
    had_error(false), panic_mode(false)
{
    compiler = std::make_unique<Compiler>(this, TYPE_SCRIPT, nullptr);
    advance();
}

std::optional<Func> Parser::Compile()
{
    while (!match(TokenType::_EOF)) {
        declaration();
    }
    auto function = end_compiler();
    
    if (had_error) {
        return std::nullopt;
    } else {
        return std::make_optional(function);
    }
}

void Parser::advance()
{
    previous = current;
    
    while (true) {
        current = scanner.scan_token();
        if (current.get_type() != TokenType::ERROR) break;
        
        error_at_current(std::string(current.get_text()));
    }
}

void Parser::consume(TokenType type, const std::string& message)
{
    if (current.get_type() == type) {
        advance();
        return;
    }
    
    error_at_current(message);
}

bool Parser::check(TokenType type)
{
    return current.get_type() == type;
}

bool Parser::match(TokenType type)
{
    if (!check(type)) return false;
    advance();
    return true;
}

void Parser::emit(uint8_t byte)
{
    CurrentChunk().write(byte, previous.get_line());
}

void Parser::emit(OpCode op)
{
    CurrentChunk().write(op, previous.get_line());
}

void Parser::emit(OpCode op, uint8_t byte)
{
    emit(op);
    emit(byte);
}

void Parser::emit(OpCode op1, OpCode op2)
{
    emit(op1);
    emit(op2);
}

void Parser::emit_loop(int loopStart)
{
    emit(OpCode::LOOP);
    
    int offset = CurrentChunk().count() - loopStart + 2;
    if (offset > UINT16_MAX) { error("Loop body too large."); }
    
    emit((offset >> 8) & 0xff);
    emit(offset & 0xff);
}

int Parser::emit_jump(OpCode op)
{
    emit(op);
    emit(0xff);
    emit(0xff);
    return CurrentChunk().count() - 2;
}

void Parser::emit_return()
{
    if (compiler->type == TYPE_INITIALIZER) {
        emit(OpCode::GET_LOCAL, 0);
    } else {
        emit(OpCode::NULLOP);
    }
    emit(OpCode::RETURN);
}

uint8_t Parser::make_constant(Value value)
{
    auto constant = CurrentChunk().add_constant(value);
    if (constant > UINT8_MAX) {
        error("Too many constants in one chunk.");
        return 0;
    }
    
    return (uint8_t)constant;
}

void Parser::emit_constant(Value value)
{
    emit(OpCode::CONSTANT, make_constant(value));
}

void Parser::patch_jump(int offset)
{
    // -2 to adjust for the bytecode for the jump offset itself.
    int jump = CurrentChunk().count() - offset - 2;
    
    if (jump > UINT16_MAX) {
        error("Too much code to jump over.");
    }
    
    CurrentChunk().set_code(offset, (jump >> 8) & 0xff);
    CurrentChunk().set_code(offset + 1, jump & 0xff);
}

Func Parser::end_compiler()
{
    emit_return();
    
    auto function = compiler->function;
    
#ifdef DEBUG_PRINT_CODE
    if (!had_error) {
        auto name = function->get_name().empty() ? "<main>" : function->get_name();
        CurrentChunk().disassemble(name);
    }
#endif

    return function;
}

void Parser::binary(bool canAssign)
{
    // Remember the operator.
    auto operatorType = previous.get_type();
    
    // Compile the right operand.
    auto rule = get_rule(operatorType);
    parse_precedence(Precedence(static_cast<int>(rule.precedence) + 1));
    
    // Emit the operator instruction.
    switch (operatorType) {
        case TokenType::EXCL_EQUAL:    emit(OpCode::EQUAL, OpCode::NOT); break;
        case TokenType::EQUAL_EQUAL:   emit(OpCode::EQUAL); break;
        case TokenType::GREATER:       emit(OpCode::GREATER); break;
        case TokenType::GREATER_EQUAL: emit(OpCode::LESS, OpCode::NOT); break;
        case TokenType::LESS:          emit(OpCode::LESS); break;
        case TokenType::LESS_EQUAL:    emit(OpCode::GREATER, OpCode::NOT); break;
        case TokenType::PLUS:          emit(OpCode::ADD); break;
        case TokenType::MINUS:         emit(OpCode::SUBTRACT); break;
        case TokenType::STAR:          emit(OpCode::MULTIPLY); break;
        case TokenType::SLASH:         emit(OpCode::DIVIDE); break;
        case TokenType::BW_OR:         emit(OpCode::BW_OR); break;
        case TokenType::BW_AND:        emit(OpCode::BW_AND); break;
        case TokenType::BW_XOR:        emit(OpCode::BW_XOR); break;
        default:
            return; // Unreachable.
    }
}

void Parser::call(bool canAssign)
{
    auto argCount = args_list();
    emit(OpCode::CALL, argCount);
}

void Parser::dot(bool canAssign)
{
    consume(TokenType::IDENTIFIER, "Expected property name after '.'.");
    auto name = identifier_constant(std::string(previous.get_text()));
    
    if (canAssign && match(TokenType::EQUAL)) {
        expression();
        emit(OpCode::SET_PROPERTY, name);
    } else if (match(TokenType::OPEN_PAREN)) {
        auto argCount = args_list();
        emit(OpCode::INVOKE, name);
        emit(argCount);
    } else {
        emit(OpCode::GET_PROPERTY, name);
    }
}

void Parser::literal(bool canAssign)
{
    switch (previous.get_type()) {
        case TokenType::FALSE: emit(OpCode::FALSE); break;
        case TokenType::NULLVAL:   emit(OpCode::NULLOP); break;
        case TokenType::TRUE:  emit(OpCode::TRUE); break;
        default:
            return; // Unreachable.
    }
}

void Parser::grouping(bool canAssign)
{
    expression();
    consume(TokenType::CLOSE_PAREN, "Expected ')' after expression.");
}

void Parser::number(bool canAssign)
{
    Value value = std::stod(std::string(previous.get_text()));
    emit_constant(value);
}

void Parser::or_(bool canAssign)
{
    int elseJump = emit_jump(OpCode::JUMP_IF_FALSE);
    int endJump = emit_jump(OpCode::JUMP);
    
    patch_jump(elseJump);
    emit(OpCode::POP);
    
    parse_precedence(Precedence::OR);
    patch_jump(endJump);
}

void Parser::string_(bool canAssign)
{
    auto str = previous.get_text();
    str.remove_prefix(1);
    str.remove_suffix(1);
    emit_constant(std::string(str));
}

void Parser::named_variable(const std::string& name, bool canAssign)
{
    OpCode getOp, setOp;
    auto arg = compiler->resolve_local(name);
    if (arg != -1) {
        getOp = OpCode::GET_LOCAL;
        setOp = OpCode::SET_LOCAL;
    } else if ((arg = compiler->resolve_upvalue(name)) != -1) {
        getOp = OpCode::GET_UPVALUE;
        setOp = OpCode::SET_UPVALUE;
    } else {
        arg = identifier_constant(name);
        getOp = OpCode::GET_GLOBAL;
        setOp = OpCode::SET_GLOBAL;
    }
    
    if (canAssign && match(TokenType::EQUAL)) {
        expression();
        emit(setOp, (uint8_t)arg);
    } else {
        emit(getOp, (uint8_t)arg);
    }
}

void Parser::variable(bool canAssign)
{
    named_variable(std::string(previous.get_text()), canAssign);
}

void Parser::super(bool canAssign)
{
    if (class_compiler == nullptr) {
        error("'super' cannot be used outside of a class.");
    } else if (!class_compiler->has_superclass) {
        error("'super' cannot be called in a class without superclass.");
    }
    
    consume(TokenType::DOT, "Expected '.' after 'super'.");
    consume(TokenType::IDENTIFIER, "Expected superclass method name.");
    auto name = identifier_constant(std::string(previous.get_text()));
    
    named_variable("this", false);
    named_variable("super", false);
    emit(OpCode::GET_SUPER, name);
}

void Parser::this_(bool canAssign)
{
    if (class_compiler == nullptr) {
        error("'this' cannot be outside of a class.");
        return;
    }
    variable(false);
}

void Parser::and_(bool canAssign)
{
    int endJump = emit_jump(OpCode::JUMP_IF_FALSE);
    
    emit(OpCode::POP);
    parse_precedence(Precedence::AND);
    
    patch_jump(endJump);
}

void Parser::unary(bool canAssign)
{
    auto operatorType = previous.get_type();
    
    // Compile the operand.
    parse_precedence(Precedence::UNARY);
    
    // Emit the operator instruciton.
    switch (operatorType) {
        case TokenType::EXCL: emit(OpCode::NOT); break;
        case TokenType::MINUS: emit(OpCode::NEGATE); break;
        case TokenType::BW_NOT: emit(OpCode::BW_NOT); break;
            
        default:
            return; // Unreachable.
    }
}

ParseRule& Parser::get_rule(TokenType type)
{
    auto grouping = [this](bool canAssign) { this->grouping(canAssign); };
    auto unary = [this](bool canAssign) { this->unary(canAssign); };
    auto binary = [this](bool canAssign) { this->binary(canAssign); };
    auto call = [this](bool canAssign) { this->call(canAssign); };
    auto dot = [this](bool canAssign) { this->dot(canAssign); };
    auto number = [this](bool canAssign) { this->number(canAssign); };
    auto string = [this](bool canAssign) { this->string_(canAssign); };
    auto literal = [this](bool canAssign) { this->literal(canAssign); };
    auto variable = [this](bool canAssign) { this->variable(canAssign); };
    auto super_ = [this](bool canAssign) { this->super(canAssign); };
    auto this_ = [this](bool canAssign) { this->this_(canAssign); };
    auto and_ = [this](bool canAssign) { this->and_(canAssign); };
    auto or_ = [this](bool canAssign) { this->or_(canAssign); };
    
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

void Parser::parse_precedence(Precedence precedence)
{
    advance();
    auto prefixRule = get_rule(previous.get_type()).prefix;
    if (prefixRule == nullptr) {
        error("Expected expression.");
        return;
    }
    
    auto canAssign = precedence <= Precedence::ASSIGNMENT;
    prefixRule(canAssign);
    
    while (precedence <= get_rule(current.get_type()).precedence) {
        advance();
        auto infixRule = get_rule(previous.get_type()).infix;
        infixRule(canAssign);
    }
    
    if (canAssign && match(TokenType::EQUAL)) {
        error("Invalid assignment target.");
        expression();
    }
}

int Parser::identifier_constant(const std::string& name)
{
    return make_constant(name);
}

uint8_t Parser::parse_variable(const std::string& errorMessage)
{
    consume(TokenType::IDENTIFIER, errorMessage);
    
    compiler->declare_variable(std::string(previous.get_text()));
    if (compiler->is_local()) return 0;
    
    return identifier_constant(std::string(previous.get_text()));
}

void Parser::define_variable(uint8_t global)
{
    if (compiler->is_local()) {
        compiler->mark_initialized();
        return;
    }
    
    emit(OpCode::DEFINE_GLOBAL, global);
}

uint8_t Parser::args_list()
{
    uint8_t argCount = 0;
    if (!check(TokenType::CLOSE_PAREN)) {
        do {
            expression();
            if (argCount == 255) {
                error("A function cannot have more than 255 arguments.");
            }
            argCount++;
        } while (match(TokenType::COMMA));
    }
    consume(TokenType::CLOSE_PAREN, "Expected ')' after arguments.");
    return argCount;
}

void Parser::expression()
{
    parse_precedence(Precedence::ASSIGNMENT);
}

void Parser::block()
{
    while (!check(TokenType::CLOSE_CURLY) && !check(TokenType::_EOF)) {
        declaration();
    }
    
    consume(TokenType::CLOSE_CURLY, "Expected '}' after block.");
}

void Parser::function(FunctionType type)
{
    compiler = std::make_unique<Compiler>(this, type, std::move(compiler));
    compiler->begin_scope();

    consume(TokenType::OPEN_PAREN, "Expected '(' after function name.");
    if (!check(TokenType::CLOSE_PAREN)) {
        do {
            compiler->function->arity++;
            if (compiler->function->arity > 255)
                error_at_current("A function cannot have more than 255 parameters.");
            auto constant = parse_variable("Expected parameter name.");
            define_variable(constant);
        } while (match(TokenType::COMMA));
    }
    consume(TokenType::CLOSE_PAREN, "Expected ')' after parameters.");
    consume(TokenType::OPEN_CURLY, "Expected '{' before function body.");
    block();
    
    auto function = end_compiler();
    auto newCompiler = std::move(compiler);
    compiler = std::move(newCompiler->enclosing);

    emit(OpCode::CLOSURE, make_constant(function));
    
    for (const auto& upvalue : newCompiler->upvalues) {
        emit(upvalue.is_local ? 1 : 0);
        emit(upvalue.index);
    }
}

void Parser::method()
{
    consume(TokenType::IDENTIFIER, "Expected method name.");
    auto constant = identifier_constant(std::string(previous.get_text()));
    auto type = previous.get_text() == "init" ? TYPE_INITIALIZER : TYPE_METHOD;
    function(type);
    emit(OpCode::METHOD, constant);
}

void Parser::class_declaration()
{
    consume(TokenType::IDENTIFIER, "Expected class name.");
    auto className = std::string(previous.get_text());
    auto nameConstant = identifier_constant(className);
    compiler->declare_variable(std::string(previous.get_text()));
    
    emit(OpCode::CLASS, nameConstant);
    define_variable(nameConstant);
    
    class_compiler = std::make_unique<ClassCompiler>(std::move(class_compiler));
    
    if (match(TokenType::LESS)) {
        consume(TokenType::IDENTIFIER, "Expected superclass name.");
        variable(false);
        
        if (className == previous.get_text()) {
            error("A class cannot inherit from itself.");
        }
        
        compiler->begin_scope();
        compiler->add_local("super");
        define_variable(0);
        
        named_variable(className, false);
        emit(OpCode::INHERIT);
        class_compiler->has_superclass = true;
    }
    
    named_variable(className, false);
    consume(TokenType::OPEN_CURLY, "Expected '{' before class body.");
    while (!check(TokenType::CLOSE_CURLY) && !check(TokenType::_EOF)) {
        method();
    }
    consume(TokenType::CLOSE_CURLY, "Expected '}' after class body.");
    emit(OpCode::POP);
    
    if (class_compiler->has_superclass) {
        compiler->end_scope();
    }
    
    class_compiler = std::move(class_compiler->enclosing);
}

void Parser::func_declaration()
{
    auto global = parse_variable("Expected function name.");
    compiler->mark_initialized();
    function(TYPE_FUNCTION);
    define_variable(global);
}

void Parser::var_declaration()
{
    auto global = parse_variable("Expected variable name.");

    if (match(TokenType::EQUAL)) {
        expression();
    } else {
        emit(OpCode::NULLOP);
    }
    consume(TokenType::SEMICOLON, "Expected ';' after variable declaration.");
    
    define_variable(global);
}

void Parser::expression_statement()
{
    expression();
    emit(OpCode::POP);
    consume(TokenType::SEMICOLON, "Expected ';' after expression.");
}

void Parser::for_statement()
{
    compiler->begin_scope();
    
    consume(TokenType::OPEN_PAREN, "Expected '(' after 'for'.");
    if (match(TokenType::VAR)) {
        var_declaration();
    } else if (match(TokenType::SEMICOLON)) {
        // no initializer.
    } else {
        expression_statement();
    }
    
    int loopStart = CurrentChunk().count();
    
    int exitJump = -1;
    if (!match(TokenType::SEMICOLON)) {
        expression();
        consume(TokenType::SEMICOLON, "Expected ';' after loop condition.");
        
        // Jump out of the loop if the condition is false.
        exitJump = emit_jump(OpCode::JUMP_IF_FALSE);
        emit(OpCode::POP);
    }

    if (!match(TokenType::CLOSE_PAREN)) {
        int bodyJump = emit_jump(OpCode::JUMP);
        
        int incrementStart = CurrentChunk().count();
        expression();
        emit(OpCode::POP);
        consume(TokenType::CLOSE_PAREN, "Expected ')' after for clauses.");
        
        emit_loop(loopStart);
        loopStart = incrementStart;
        patch_jump(bodyJump);
    }
    
    statement();
    
    emit_loop(loopStart);
    
    if (exitJump != -1) {
        patch_jump(exitJump);
        emit(OpCode::POP); // Condition.
    }

    compiler->end_scope();
}

void Parser::if_statement()
{
    consume(TokenType::OPEN_PAREN, "Expected '(' after 'if'.");
    expression();
    consume(TokenType::CLOSE_PAREN, "Expected ')' after condition.");
    
    int thenJump = emit_jump(OpCode::JUMP_IF_FALSE);
    emit(OpCode::POP);
    statement();
    int elseJump = emit_jump(OpCode::JUMP);
    
    patch_jump(thenJump);
    emit(OpCode::POP);
    if (match(TokenType::ELSE)) { statement(); }
    patch_jump(elseJump);
}

void Parser::declaration()
{
    if (match(TokenType::CLASS)) {
        class_declaration();
    } else if (match(TokenType::FUN)) {
        func_declaration();
    } else if (match(TokenType::VAR)) {
        var_declaration();
    } else {
        statement();
    }
    
    if (panic_mode) sync();
}

void Parser::statement()
{
    if (match(TokenType::PRINT)) {
        print_statement();
    } else if (match(TokenType::FOR)) {
        for_statement();
    } else if (match(TokenType::IF)) {
        if_statement();
    } else if (match(TokenType::RETURN)) {
        return_statement();
    } else if (match(TokenType::WHILE)) {
        while_statement();
    } else if (match(TokenType::OPEN_CURLY)) {
        compiler->begin_scope();
        block();
        compiler->end_scope();
    } else {
        expression_statement();
    }
}

void Parser::print_statement()
{
    expression();
    consume(TokenType::SEMICOLON, "Expected ';' after value.");
    emit(OpCode::PRINT);
}

void Parser::return_statement()
{
    if (compiler->type == TYPE_SCRIPT) {
        error("Cannot return from top-level code.");
    }
    
    if (match(TokenType::SEMICOLON)) {
        emit_return();
    } else {
        if (compiler->type == TYPE_INITIALIZER) {
            error("Cannot return a value from an initializer.");
        }
        
        expression();
        consume(TokenType::SEMICOLON, "Expected ';' after return value.");
        emit(OpCode::RETURN);
    }
}

void Parser::while_statement()
{
    int loopStart = CurrentChunk().count();
    
    consume(TokenType::OPEN_PAREN, "Expected '(' after 'while'.");
    expression();
    consume(TokenType::CLOSE_PAREN, "Expected ')' after condition.");
    
    int exitJump = emit_jump(OpCode::JUMP_IF_FALSE);
    
    emit(OpCode::POP);
    statement();
    
    emit_loop(loopStart);
    
    patch_jump(exitJump);
    emit(OpCode::POP);
}

void Parser::sync()
{
    panic_mode = false;
    
    while (current.get_type() != TokenType::_EOF) {
        if (previous.get_type() == TokenType::SEMICOLON) return;
        
        switch (current.get_type()) {
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
        
        advance();
    }
}

void Parser::error_at(const Token& token, const std::string& message)
{
    if (panic_mode) return;
    
    panic_mode = true;
    
    std::cerr << "[line " << token.get_line() << "] Error";
    if (token.get_type() == TokenType::_EOF) {
        std::cerr << " at end";
    } else if (token.get_type() == TokenType::ERROR) {
        // Nothing.
    } else {
        std::cerr << " at '" << token.get_text() << "'";
    }
    
    std::cerr << ": " << message << std::endl;
    had_error = true;
}
