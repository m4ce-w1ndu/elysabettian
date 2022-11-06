#include "compiler.hpp"

#include <array>

compiler_t::compiler_t(parser_t* parser, function_type_t type, std::unique_ptr<compiler_t> enclosing)
    : parser(parser), type(type), function{default_function}, enclosing(std::move(enclosing))
{
        locals.emplace_back(type == function_type_t::TYPE_FUNCTION ? "" : "this", 0);
        if (type != function_type_t::TYPE_SCRIPT) {
            function->name = parser->previous.get_text();
        }
};

void compiler_t::add_local(const std::string& name)
{
    if (locals.size() == UINT8_COUNT) {
        parser->error("Too many local variables in function.");
        return;
    }
    locals.emplace_back(name, -1);
}

void compiler_t::declare_variable(const std::string& name)
{
    if (scope_depth == 0) return;
    
    for (auto i = static_cast<long long>(locals.size() - 1); i >= 0; i--) {
        if (locals[i].depth != -1 && locals[i].depth < scope_depth) break;
        if (locals[i].name == name) {
            parser->error("Already a variable with this name in this scope.");
        }
    }
    
    add_local(name);
}

void compiler_t::mark_initialized()
{
    if (scope_depth == 0) return;
    locals.back().depth = scope_depth;
}

int compiler_t::resolve_local(const std::string_view& name)
{
    for (auto i = static_cast<long>(locals.size() - 1); i >=0; i--) {
        if (locals[i].name == name) {
            if (locals[i].depth == -1) {
                parser->error("Can't read local variable in its own initializer.");
            }
            return static_cast<int>(i);
        }
    }
    
    return -1;
}

int compiler_t::resolve_upvalue(const std::string& name)
{
    if (enclosing == nullptr) return -1;

    if (int local = enclosing->resolve_local(name); local != -1) {
        enclosing->locals[local].is_captured = true;
        return add_upvalue(static_cast<uint8_t>(local), true);
    }

    if (int upvalue = enclosing->resolve_upvalue(name); upvalue != -1) {
        return add_upvalue(static_cast<uint8_t>(upvalue), false);
    }
    
    return -1;
}

int compiler_t::add_upvalue(uint8_t index, bool is_local)
{
    for (long i = 0; i < upvalues.size(); i++) {
        if (upvalues[i].index == index && upvalues[i].is_local == is_local) {
            return static_cast<int>(i);
        }
    }
    
    if (upvalues.size() == UINT8_COUNT) {
        parser->error("Too many closure variables in function.");
        return 0;
    }

    upvalues.emplace_back(upvalue_t(index, is_local));
    auto upvalue_count = static_cast<int>(upvalues.size());
    function->upvalue_count = upvalue_count;
    return upvalue_count - 1;
}

void compiler_t::begin_scope()
{
    scope_depth++;
}

void compiler_t::end_scope()
{
    scope_depth--;
    while (!locals.empty() && locals.back().depth > scope_depth) {
        if (locals.back().is_captured) {
            parser->emit(OpCode::CLOSE_UPVALUE);
        } else {
            parser->emit(OpCode::POP);
        }
        locals.pop_back();
    }
}

bool compiler_t::is_local() const
{
    return scope_depth > 0;
}

class_compiler_t::class_compiler_t(std::unique_ptr<class_compiler_t> enclosing)
    : enclosing(std::move(enclosing)), has_superclass{superclass_default} {};

parser_t::parser_t(const std::string& source) :
    previous(Token(TokenType::_EOF, source, 0)),
    current(Token(TokenType::_EOF, source, 0)),
    scanner(Tokenizer(source)),
    class_compiler{null_value},
    had_error{false_value}, panic_mode{false_value}
{
    compiler = std::make_unique<compiler_t>(this, function_type_t::TYPE_SCRIPT, nullptr);
    advance();
}

std::optional<Func> parser_t::compile()
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

void parser_t::advance()
{
    previous = current;
    
    while (true) {
        current = scanner.scan_token();
        if (current.get_type() != TokenType::ERROR) break;
        
        error_at_current(std::string(current.get_text()));
    }
}

void parser_t::consume(TokenType type, const std::string& message)
{
    if (current.get_type() == type) {
        advance();
        return;
    }
    
    error_at_current(message);
}

bool parser_t::check(TokenType type) const
{
    return current.get_type() == type;
}

bool parser_t::match(TokenType type)
{
    if (!check(type)) return false;
    advance();
    return true;
}

void parser_t::emit(uint8_t byte)
{
    CurrentChunk().write(byte, previous.get_line());
}

void parser_t::emit(OpCode op)
{
    CurrentChunk().write(op, previous.get_line());
}

void parser_t::emit(OpCode op, uint8_t byte)
{
    emit(op);
    emit(byte);
}

void parser_t::emit(OpCode op1, OpCode op2)
{
    emit(op1);
    emit(op2);
}

void parser_t::emit_loop(int loopStart)
{
    emit(OpCode::LOOP);
    
    int offset = CurrentChunk().count() - loopStart + 2;
    if (offset > UINT16_MAX) { error("Loop body too large."); }
    
    emit((offset >> 8) & 0xff);
    emit(offset & 0xff);
}

int parser_t::emit_jump(OpCode op)
{
    emit(op);
    emit(0xff);
    emit(0xff);
    return CurrentChunk().count() - 2;
}

void parser_t::emit_return()
{
    if (compiler->type == function_type_t::TYPE_INITIALIZER) {
        emit(OpCode::GET_LOCAL, 0);
    } else {
        emit(OpCode::NULLOP);
    }
    emit(OpCode::RETURN);
}

uint8_t parser_t::make_constant(const Value& value)
{
    auto constant = CurrentChunk().add_constant(value);
    if (constant > UINT8_MAX) {
        error("Too many constants in one chunk.");
        return 0;
    }
    
    return (uint8_t)constant;
}

void parser_t::emit_constant(const Value& value)
{
    emit(OpCode::CONSTANT, make_constant(value));
}

void parser_t::patch_jump(int offset)
{
    // -2 to adjust for the bytecode for the jump offset itself.
    int jump = CurrentChunk().count() - offset - 2;
    
    if (jump > UINT16_MAX) {
        error("Too much code to jump over.");
    }
    
    CurrentChunk().set_code(offset, (jump >> 8) & 0xff);
    CurrentChunk().set_code(offset + 1, jump & 0xff);
}

Func parser_t::end_compiler()
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

void parser_t::binary([[maybe_unused]] bool can_assign)
{
    // Remember the operator.
    auto operatorType = previous.get_type();
    
    // Compile the right operand.
    auto rule = get_rule(operatorType);
    parse_precedence(precedence_t(static_cast<int>(rule.precedence) + 1));
    
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

void parser_t::call([[maybe_unused]] bool can_assign)
{
    auto arg_count = args_list();
    emit(OpCode::CALL, arg_count);
}

void parser_t::dot([[maybe_unused]] bool can_assign)
{
    consume(TokenType::IDENTIFIER, "Expected property name after '.'.");
    auto name = identifier_constant(std::string(previous.get_text()));
    
    if (can_assign && match(TokenType::EQUAL)) {
        expression();
        emit(OpCode::SET_PROPERTY, static_cast<uint8_t>(name));
    } else if (match(TokenType::OPEN_PAREN)) {
        auto arg_count = args_list();
        emit(OpCode::INVOKE, static_cast<uint8_t>(name));
        emit(arg_count);
    } else {
        emit(OpCode::GET_PROPERTY, static_cast<uint8_t>(name));
    }
}

void parser_t::literal([[maybe_unused]] bool can_assign)
{
    switch (previous.get_type()) {
        case TokenType::FALSE:      emit(OpCode::FALSE); break;
        case TokenType::NULLVAL:    emit(OpCode::NULLOP); break;
        case TokenType::TRUE:       emit(OpCode::TRUE); break;
        default:                    return; // Unreachable.
    }
}

void parser_t::grouping([[maybe_unused]] bool can_assign)
{
    expression();
    consume(TokenType::CLOSE_PAREN, "Expected ')' after expression.");
}

void parser_t::number([[maybe_unused]] bool can_assign)
{
    Value value = std::stod(std::string(previous.get_text()));
    emit_constant(value);
}

void parser_t::or_([[maybe_unused]] bool can_assign)
{
    int else_jump = emit_jump(OpCode::JUMP_IF_FALSE);
    int end_jump = emit_jump(OpCode::JUMP);
    
    patch_jump(else_jump);
    emit(OpCode::POP);
    
    parse_precedence(precedence_t::OR);
    patch_jump(end_jump);
}

void parser_t::string_([[maybe_unused]] bool can_assign)
{
    auto str = previous.get_text();
    str.remove_prefix(1);
    str.remove_suffix(1);
    emit_constant(std::string(str));
}

void parser_t::named_variable(const std::string& name, bool can_assign)
{
    OpCode get_op;
    OpCode set_op;
    auto arg = compiler->resolve_local(name);
    if (arg != -1) {
        get_op = OpCode::GET_LOCAL;
        set_op = OpCode::SET_LOCAL;
    } else if ((arg = compiler->resolve_upvalue(name)) != -1) {
        get_op = OpCode::GET_UPVALUE;
        set_op = OpCode::SET_UPVALUE;
    } else {
        arg = identifier_constant(name);
        get_op = OpCode::GET_GLOBAL;
        set_op = OpCode::SET_GLOBAL;
    }
    
    if (can_assign && match(TokenType::EQUAL)) {
        expression();
        emit(set_op, (uint8_t)arg);
    } else {
        emit(get_op, (uint8_t)arg);
    }
}

void parser_t::variable([[maybe_unused]] bool can_assign)
{
    named_variable(std::string(previous.get_text()), can_assign);
}

void parser_t::super([[maybe_unused]] bool can_assign)
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
    emit(OpCode::GET_SUPER, static_cast<uint8_t>(name));
}

void parser_t::this_([[maybe_unused]] bool can_assign)
{
    if (class_compiler == nullptr) {
        error("'this' cannot be outside of a class.");
        return;
    }
    variable(false);
}

void parser_t::and_([[maybe_unused]] bool can_assign)
{
    int end_jump = emit_jump(OpCode::JUMP_IF_FALSE);
    
    emit(OpCode::POP);
    parse_precedence(precedence_t::AND);
    
    patch_jump(end_jump);
}

void parser_t::unary([[maybe_unused]] bool can_assign)
{
    auto operator_type = previous.get_type();
    
    // Compile the operand.
    parse_precedence(precedence_t::UNARY);
    
    // Emit the operator instruciton.
    switch (operator_type) {
        case TokenType::EXCL: emit(OpCode::NOT); break;
        case TokenType::MINUS: emit(OpCode::NEGATE); break;
        case TokenType::BW_NOT: emit(OpCode::BW_NOT); break;
            
        default:
            return; // Unreachable.
    }
}

parse_rule_t& parser_t::get_rule(TokenType type)
{
    auto grouping = [this](bool can_assign) { this->grouping(can_assign); };
    auto unary = [this](bool can_assign) { this->unary(can_assign); };
    auto binary = [this](bool can_assign) { this->binary(can_assign); };
    auto call = [this](bool can_assign) { this->call(can_assign); };
    auto dot = [this](bool can_assign) { this->dot(can_assign); };
    auto number = [this](bool can_assign) { this->number(can_assign); };
    auto string = [this](bool can_assign) { this->string_(can_assign); };
    auto literal = [this](bool can_assign) { this->literal(can_assign); };
    auto variable = [this](bool can_assign) { this->variable(can_assign); };
    auto super_ = [this](bool can_assign) { this->super(can_assign); };
    auto this_ = [this](bool can_assign) { this->this_(can_assign); };
    auto and_ = [this](bool can_assign) { this->and_(can_assign); };
    auto or_ = [this](bool can_assign) { this->or_(can_assign); };
    
    static std::array<parse_rule_t, 44> rules = {{
        { grouping,    call,       precedence_t::CALL },       // TOKEN_LEFT_PAREN
        { nullptr,     nullptr,    precedence_t::NONE },       // TOKEN_RIGHT_PAREN
        { nullptr,     nullptr,    precedence_t::NONE },       // TOKEN_LEFT_BRACE
        { nullptr,     nullptr,    precedence_t::NONE },       // TOKEN_RIGHT_BRACE
        { nullptr,     nullptr,    precedence_t::NONE },       // TOKEN_COMMA
        { nullptr,     dot,        precedence_t::CALL },       // TOKEN_DOT
        { unary,       binary,     precedence_t::TERM },       // TOKEN_MINUS
        { nullptr,     binary,     precedence_t::TERM },       // TOKEN_PLUS
        { nullptr,     nullptr,    precedence_t::NONE },       // TOKEN_SEMICOLON
        { nullptr,     binary,     precedence_t::FACTOR },     // TOKEN_SLASH
        { nullptr,     binary,     precedence_t::FACTOR },     // TOKEN_STAR
        { unary,       nullptr,    precedence_t::NONE },       // TOKEN_BANG
        { nullptr,     binary,     precedence_t::EQUALITY },   // TOKEN_BANG_EQUAL
        { nullptr,     nullptr,    precedence_t::NONE },       // TOKEN_EQUAL
        { nullptr,     binary,     precedence_t::EQUALITY },   // TOKEN_EQUAL_EQUAL
        { nullptr,     binary,     precedence_t::COMPARISON }, // TOKEN_GREATER
        { nullptr,     binary,     precedence_t::COMPARISON }, // TOKEN_GREATER_EQUAL
        { nullptr,     binary,     precedence_t::COMPARISON }, // TOKEN_LESS
        { nullptr,     binary,     precedence_t::COMPARISON }, // TOKEN_LESS_EQUAL
        { variable,    nullptr,    precedence_t::NONE },       // TOKEN_IDENTIFIER
        { string,      nullptr,    precedence_t::NONE },       // TOKEN_STRING
        { number,      nullptr,    precedence_t::NONE },       // TOKEN_NUMBER
        { nullptr,     and_,       precedence_t::AND },        // TOKEN_AND
        { nullptr,     nullptr,    precedence_t::NONE },       // TOKEN_CLASS
        { nullptr,     nullptr,    precedence_t::NONE },       // TOKEN_ELSE
        { literal,     nullptr,    precedence_t::NONE },       // TOKEN_FALSE
        { nullptr,     nullptr,    precedence_t::NONE },       // TOKEN_FUN
        { nullptr,     nullptr,    precedence_t::NONE },       // TOKEN_FOR
        { nullptr,     nullptr,    precedence_t::NONE },       // TOKEN_IF
        { literal,     nullptr,    precedence_t::NONE },       // TOKEN_NIL
        { nullptr,     or_,        precedence_t::OR },         // TOKEN_OR
        { nullptr,     nullptr,    precedence_t::NONE },       // TOKEN_PRINT
        { nullptr,     nullptr,    precedence_t::NONE },       // TOKEN_RETURN
        { super_,      nullptr,    precedence_t::NONE },       // TOKEN_SUPER
        { this_,       nullptr,    precedence_t::NONE },       // TOKEN_THIS
        { literal,     nullptr,    precedence_t::NONE },       // TOKEN_TRUE
        { nullptr,     nullptr,    precedence_t::NONE },       // TOKEN_VAR
        { nullptr,     nullptr,    precedence_t::NONE },       // TOKEN_WHILE
        { nullptr,     nullptr,    precedence_t::NONE },       // TOKEN_ERROR
        { nullptr,     nullptr,    precedence_t::NONE },       // TOKEN_EOF
        { nullptr,     binary,     precedence_t::TERM },       // TOKEN_BW_AND
        { nullptr,     binary,     precedence_t::TERM },       // TOKEN_BW_OR
        { nullptr,     binary,     precedence_t::TERM },       // TOKEN_BW_XOR
        { unary,       nullptr,    precedence_t::UNARY},       // TOKEN_BW_NOT
    }};
    
    return rules[static_cast<int>(type)];
}

void parser_t::parse_precedence(precedence_t precedence)
{
    advance();
    auto prefix_rule = get_rule(previous.get_type()).prefix;
    if (prefix_rule == nullptr) {
        error("Expected expression.");
        return;
    }
    
    auto can_assign = precedence <= precedence_t::ASSIGNMENT;
    prefix_rule(can_assign);
    
    while (precedence <= get_rule(current.get_type()).precedence) {
        advance();
        auto infix_rule = get_rule(previous.get_type()).infix;
        infix_rule(can_assign);
    }
    
    if (can_assign && match(TokenType::EQUAL)) {
        error("Invalid assignment target.");
        expression();
    }
}

int parser_t::identifier_constant(const std::string& name)
{
    return make_constant(name);
}

uint8_t parser_t::parse_variable(const std::string& errorMessage)
{
    consume(TokenType::IDENTIFIER, errorMessage);
    
    compiler->declare_variable(std::string(previous.get_text()));
    if (compiler->is_local()) return 0;
    
    return static_cast<uint8_t>(identifier_constant(std::string(previous.get_text())));
}

void parser_t::define_variable(uint8_t global)
{
    if (compiler->is_local()) {
        compiler->mark_initialized();
        return;
    }
    
    emit(OpCode::DEFINE_GLOBAL, global);
}

uint8_t parser_t::args_list()
{
    uint8_t arg_count = 0;
    if (!check(TokenType::CLOSE_PAREN)) {
        do {
            expression();
            if (arg_count == 255) {
                error("A function cannot have more than 255 arguments.");
            }
            arg_count++;
        } while (match(TokenType::COMMA));
    }
    consume(TokenType::CLOSE_PAREN, "Expected ')' after arguments.");
    return arg_count;
}

void parser_t::expression()
{
    parse_precedence(precedence_t::ASSIGNMENT);
}

void parser_t::block()
{
    while (!check(TokenType::CLOSE_CURLY) && !check(TokenType::_EOF)) {
        declaration();
    }
    
    consume(TokenType::CLOSE_CURLY, "Expected '}' after block.");
}

void parser_t::function(function_type_t type)
{
    compiler = std::make_unique<compiler_t>(this, type, std::move(compiler));
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
    auto new_compiler = std::move(compiler);
    compiler = std::move(new_compiler->enclosing);

    emit(OpCode::CLOSURE, make_constant(function));
    
    for (const auto& upvalue : new_compiler->upvalues) {
        emit(upvalue.is_local ? 1 : 0);
        emit(upvalue.index);
    }
}

void parser_t::method()
{
    consume(TokenType::IDENTIFIER, "Expected method name.");
    const auto constant = identifier_constant(std::string(previous.get_text()));
    const auto type = previous.get_text() == "init" ? function_type_t::TYPE_INITIALIZER : function_type_t::TYPE_METHOD;
    function(type);
    emit(OpCode::METHOD, static_cast<uint8_t>(constant));
}

void parser_t::class_declaration()
{
    consume(TokenType::IDENTIFIER, "Expected class name.");
    const auto class_name = std::string(previous.get_text());
    const auto name_constant = identifier_constant(class_name);
    compiler->declare_variable(std::string(previous.get_text()));
    
    emit(OpCode::CLASS, static_cast<uint8_t>(name_constant));
    define_variable(static_cast<uint8_t>(name_constant));
    
    class_compiler = std::make_unique<class_compiler_t>(std::move(class_compiler));
    
    if (match(TokenType::LESS)) {
        consume(TokenType::IDENTIFIER, "Expected superclass name.");
        variable(false);
        
        if (class_name == previous.get_text()) {
            error("A class cannot inherit from itself.");
        }
        
        compiler->begin_scope();
        compiler->add_local("super");
        define_variable(0);
        
        named_variable(class_name, false);
        emit(OpCode::INHERIT);
        class_compiler->has_superclass = true;
    }
    
    named_variable(class_name, false);
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

void parser_t::func_declaration()
{
    auto global = parse_variable("Expected function name.");
    compiler->mark_initialized();
    function(function_type_t::TYPE_FUNCTION);
    define_variable(global);
}

void parser_t::var_declaration()
{
    const auto global = parse_variable("Expected variable name.");

    if (match(TokenType::EQUAL)) {
        expression();
    } else {
        emit(OpCode::NULLOP);
    }
    consume(TokenType::SEMICOLON, "Expected ';' after variable declaration.");
    
    define_variable(global);
}

void parser_t::expression_statement()
{
    expression();
    emit(OpCode::POP);
    consume(TokenType::SEMICOLON, "Expected ';' after expression.");
}

void parser_t::for_statement()
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
    
    int loop_start = CurrentChunk().count();
    
    int exit_jump = -1;
    if (!match(TokenType::SEMICOLON)) {
        expression();
        consume(TokenType::SEMICOLON, "Expected ';' after loop condition.");
        
        // Jump out of the loop if the condition is false.
        exit_jump = emit_jump(OpCode::JUMP_IF_FALSE);
        emit(OpCode::POP);
    }

    if (!match(TokenType::CLOSE_PAREN)) {
        const int body_jump = emit_jump(OpCode::JUMP);
        
        int increment_start = CurrentChunk().count();
        expression();
        emit(OpCode::POP);
        consume(TokenType::CLOSE_PAREN, "Expected ')' after for clauses.");
        
        emit_loop(loop_start);
        loop_start = increment_start;
        patch_jump(body_jump);
    }
    
    statement();
    
    emit_loop(loop_start);
    
    if (exit_jump != -1) {
        patch_jump(exit_jump);
        emit(OpCode::POP); // Condition.
    }

    compiler->end_scope();
}

void parser_t::if_statement()
{
    consume(TokenType::OPEN_PAREN, "Expected '(' after 'if'.");
    expression();
    consume(TokenType::CLOSE_PAREN, "Expected ')' after condition.");
    
    const int then_jump = emit_jump(OpCode::JUMP_IF_FALSE);
    emit(OpCode::POP);
    statement();
    const int else_jump = emit_jump(OpCode::JUMP);
    
    patch_jump(then_jump);
    emit(OpCode::POP);
    if (match(TokenType::ELSE)) { statement(); }
    patch_jump(else_jump);
}

void parser_t::declaration()
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

void parser_t::statement()
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

void parser_t::print_statement()
{
    expression();
    consume(TokenType::SEMICOLON, "Expected ';' after value.");
    emit(OpCode::PRINT);
}

void parser_t::return_statement()
{
    if (compiler->type == function_type_t::TYPE_SCRIPT) {
        error("Cannot return from top-level code.");
    }
    
    if (match(TokenType::SEMICOLON)) {
        emit_return();
    } else {
        if (compiler->type == function_type_t::TYPE_INITIALIZER) {
            error("Cannot return a value from an initializer.");
        }
        
        expression();
        consume(TokenType::SEMICOLON, "Expected ';' after return value.");
        emit(OpCode::RETURN);
    }
}

void parser_t::while_statement()
{
    const int loop_start = CurrentChunk().count();
    
    consume(TokenType::OPEN_PAREN, "Expected '(' after 'while'.");
    expression();
    consume(TokenType::CLOSE_PAREN, "Expected ')' after condition.");
    
    const int exit_jump = emit_jump(OpCode::JUMP_IF_FALSE);
    
    emit(OpCode::POP);
    statement();
    
    emit_loop(loop_start);
    
    patch_jump(exit_jump);
    emit(OpCode::POP);
}

void parser_t::sync()
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

void parser_t::error_at(const Token& token, const std::string& message)
{
    if (panic_mode) return;
    
    panic_mode = true;
    
    fmt::print(stderr, "[line {} ] Error", token.get_line());
    if (token.get_type() == TokenType::_EOF) {
        fmt::print(stderr, " at end");
    } else if (token.get_type() == TokenType::ERROR) {
        // Nothing.
    } else {
        fmt::print(stderr, " at '{}'", token.get_text());
    }
    
    fmt::print(stderr, ": {}\n", message);
    had_error = true;
}
