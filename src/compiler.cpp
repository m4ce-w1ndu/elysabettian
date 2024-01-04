#include "runtime/compiler.h"

#include <array>

Compiler::Compiler(Parser* parser, FunctionType type, std::unique_ptr<Compiler> enclosing)
    : parser(parser), type(type), function{default_function}, enclosing(std::move(enclosing))
{
    locals.emplace_back(type == FunctionType::Function ? "" : "this", 0);
    if (type != FunctionType::Script) {
        function->name = parser->previous.get_text();
    }
}

void Compiler::add_local(const std::string& name)
{
    if (locals.size() == UINT8_COUNT) {
        parser->error("Too many local variables in function.");
        return;
    }
    locals.emplace_back(name, -1);
}

void Compiler::declare_variable(const std::string& name)
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

void Compiler::mark_initialized()
{
    if (scope_depth == 0) return;
    locals.back().depth = scope_depth;
}

int Compiler::resolve_local(const std::string_view& name)
{
    for (long i = static_cast<long>(locals.size() - 1); i >=0; i--) {
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

    if (int local = enclosing->resolve_local(name); local != -1) {
        enclosing->locals[local].is_captured = true;
        return add_upvalue(static_cast<uint8_t>(local), true);
    }

    if (int upvalue = enclosing->resolve_upvalue(name); upvalue != -1) {
        return add_upvalue(static_cast<uint8_t>(upvalue), false);
    }
    
    return -1;
}

int Compiler::add_upvalue(uint8_t index, bool is_local)
{
    for (long i = 0; i < static_cast<long>(upvalues.size()); i++) {
        if (upvalues[i].index == index && upvalues[i].is_local == is_local) {
            return static_cast<int>(i);
        }
    }
    
    if (upvalues.size() == UINT8_COUNT) {
        parser->error("Too many closure variables in function.");
        return 0;
    }

    upvalues.emplace_back(UpvalueVar(index, is_local));
    int upvalue_count = static_cast<int>(upvalues.size());
    function->upvalue_count = upvalue_count;
    return upvalue_count - 1;
}

void Compiler::begin_scope()
{
    scope_depth++;
}

void Compiler::end_scope()
{
    scope_depth--;
    while (!locals.empty() && locals.back().depth > scope_depth) {
        if (locals.back().is_captured) {
            parser->emit(Opcode::CloseUpvalue);
        } else {
            parser->emit(Opcode::Pop);
        }
        locals.pop_back();
    }
}

bool Compiler::is_local() const
{
    return scope_depth > 0;
}

ClassCompiler::ClassCompiler(std::unique_ptr<ClassCompiler> enclosing)
    : enclosing(std::move(enclosing)), has_superclass{superclass_default} {};

Parser::Parser(const std::string& source) :
    previous(Token(TokenType::Eof, source, 0)),
    current(Token(TokenType::Eof, source, 0)),
    scanner(Tokenizer(source)),
    class_compiler{nullptr},
    had_error{false}, panic_mode{false}
{
    compiler = std::make_unique<Compiler>(this, FunctionType::Script, nullptr);
    advance();

    build_parse_rules();
}

std::optional<Func> Parser::compile()
{
    while (!match(TokenType::Eof)) {
        declaration();
    }
    Func function = end_compiler();
    
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
        if (current.get_type() != TokenType::Error) break;
        
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

bool Parser::check(TokenType type) const
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
    current_chunk().write(byte, previous.get_line());
}

void Parser::emit(Opcode op)
{
    current_chunk().write(op, previous.get_line());
}

void Parser::emit(Opcode op, uint8_t byte)
{
    emit(op);
    emit(byte);
}

void Parser::emit(Opcode op1, Opcode op2)
{
    emit(op1);
    emit(op2);
}

void Parser::emit_loop(int loop_start)
{
    emit(Opcode::Loop);
    
    int offset = current_chunk().count() - loop_start + 2;
    if (offset > UINT16_MAX) { error("Loop body too large."); }
    
    emit((offset >> 8) & 0xff);
    emit(offset & 0xff);
}

int Parser::emit_jump(Opcode op)
{
    emit(op);
    emit(0xff);
    emit(0xff);
    return current_chunk().count() - 2;
}

void Parser::emit_return()
{
    if (compiler->type == FunctionType::Initializer) {
        emit(Opcode::GetLocal, 0);
    } else {
        emit(Opcode::Nop);
    }
    emit(Opcode::Return);
}

uint8_t Parser::make_constant(const Value& value)
{
    size_t constant = current_chunk().add_constant(value);
    if (constant > UINT8_MAX) {
        error("Too many constants in one chunk.");
        return 0;
    }
    
    return (uint8_t)constant;
}

void Parser::emit_constant(const Value& value)
{
    emit(Opcode::Constant, make_constant(value));
}

void Parser::patch_jump(int offset)
{
    // -2 to adjust for the bytecode for the jump offset itself.
    int jump = current_chunk().count() - offset - 2;
    
    if (jump > UINT16_MAX) {
        error("Too much code to jump over.");
    }
    
    current_chunk().set_code(offset, (jump >> 8) & 0xff);
    current_chunk().set_code(offset + 1, jump & 0xff);
}

Func Parser::end_compiler()
{
    emit_return();
    
    Func function = compiler->function;
    
#ifdef DEBUG_PRINT_CODE
    if (!had_error) {
        auto name = function->get_name().empty() ? "<main>" : function->get_name();
        current_chunk().disassemble(name);
    }
#endif

    return function;
}

void Parser::array([[maybe_unused]] bool can_assign)
{
    size_t count = 0;

    if (!check(TokenType::CloseSquare)) {
        do {
            if (check(TokenType::CloseSquare)) break;
            parse_precedence(PrecedenceType::Or);

            if (count == UINT8_COUNT)
                error("List literals do not allow more than 255 items.");

            count++;
        } while (match(TokenType::Comma));
    }

    consume(TokenType::CloseSquare, "Expected ']' after list literal.");

    emit(Opcode::ArrBuild);
    emit(static_cast<uint8_t>(count));
}

void Parser::array_idx([[maybe_unused]] bool can_assign)
{
    parse_precedence(PrecedenceType::Or);
    consume(TokenType::CloseSquare, "Expected ']' after array index.");

    if (can_assign && match(TokenType::Equal)) {
        expression();
        emit(Opcode::ArrStore);
    }
    else {
        emit(Opcode::ArrIndex);
    }
}

void Parser::binary([[maybe_unused]] bool can_assign)
{
    // Remember the operator.
    TokenType operator_type = previous.get_type();
    
    // Compile the right operand.
    ParseRule rule = get_rule(operator_type);
    parse_precedence(PrecedenceType(static_cast<int>(rule.precedence) + 1));
    
    // Emit the operator instruction.
    switch (operator_type) {
        case TokenType::ExclEqual:      emit(Opcode::Equal, Opcode::Not); break;
        case TokenType::EqualEqual:     emit(Opcode::Equal); break;
        case TokenType::Greater:        emit(Opcode::Greater); break;
        case TokenType::GreaterEqual:   emit(Opcode::Less, Opcode::Not); break;
        case TokenType::Less:           emit(Opcode::Less); break;
        case TokenType::LessEqual:      emit(Opcode::Greater, Opcode::Not); break;
        case TokenType::Plus:           emit(Opcode::Add); break;
        case TokenType::Minus:          emit(Opcode::Subtract); break;
        case TokenType::Star:           emit(Opcode::Multiply); break;
        case TokenType::Slash:          emit(Opcode::Divide); break;
        case TokenType::BwOr:           emit(Opcode::BwOr); break;
        case TokenType::BwAnd:          emit(Opcode::BwAnd); break;
        case TokenType::BwXor:          emit(Opcode::BwXor); break;
        default:
            return;
    }
}

void Parser::call([[maybe_unused]] bool can_assign)
{
    uint8_t arg_count = args_list();
    emit(Opcode::Call, arg_count);
}

void Parser::dot([[maybe_unused]] bool can_assign)
{
    consume(TokenType::Identifier, "Expected property name after '.'.");
    int name = identifier_constant(std::string(previous.get_text()));
    
    if (can_assign && match(TokenType::Equal)) {
        expression();
        emit(Opcode::SetProperty, static_cast<uint8_t>(name));
    } else if (match(TokenType::OpenParen)) {
        uint8_t arg_count = args_list();
        emit(Opcode::Invoke, static_cast<uint8_t>(name));
        emit(arg_count);
    } else {
        emit(Opcode::GetProperty, static_cast<uint8_t>(name));
    }
}

void Parser::literal([[maybe_unused]] bool can_assign)
{
    switch (previous.get_type()) {
        case TokenType::False:  emit(Opcode::False); break;
        case TokenType::Null:   emit(Opcode::Nop); break;
        case TokenType::True:   emit(Opcode::True); break;
        default:                return; // Unreachable.
    }
}

void Parser::grouping([[maybe_unused]] bool can_assign)
{
    expression();
    consume(TokenType::CloseParen, "Expected ')' after expression.");
}

void Parser::number([[maybe_unused]] bool can_assign)
{
    Value value = std::stod(std::string(previous.get_text()));
    emit_constant(value);
}

void Parser::or_([[maybe_unused]] bool can_assign)
{
    int else_jump = emit_jump(Opcode::JumpIfFalse);
    int end_jump = emit_jump(Opcode::Jump);
    
    patch_jump(else_jump);
    emit(Opcode::Pop);
    
    parse_precedence(PrecedenceType::Or);
    patch_jump(end_jump);
}

void Parser::string_([[maybe_unused]] bool can_assign)
{
    std::string_view str = previous.get_text();
    str.remove_prefix(1);
    str.remove_suffix(1);
    emit_constant(std::string(str));
}

void Parser::named_variable(const std::string& name, bool can_assign)
{
    Opcode get_op;
    Opcode set_op;
    int arg = compiler->resolve_local(name);
    if (arg != -1) {
        get_op = Opcode::GetLocal;
        set_op = Opcode::SetLocal;
    } else if ((arg = compiler->resolve_upvalue(name)) != -1) {
        get_op = Opcode::GetUpvalue;
        set_op = Opcode::SetUpvalue;
    } else {
        arg = identifier_constant(name);
        get_op = Opcode::GetGlobal;
        set_op = Opcode::SetGlobal;
    }
    
    if (can_assign && match(TokenType::Equal)) {
        expression();
        emit(set_op, (uint8_t)arg);
    } else {
        emit(get_op, (uint8_t)arg);
    }
}

void Parser::variable([[maybe_unused]] bool can_assign)
{
    named_variable(std::string(previous.get_text()), can_assign);
}

void Parser::super([[maybe_unused]] bool can_assign)
{
    if (class_compiler == nullptr) {
        error("'super' cannot be used outside of a class.");
    } else if (!class_compiler->has_superclass) {
        error("'super' cannot be called in a class without superclass.");
    }
    
    consume(TokenType::Dot, "Expected '.' after 'super'.");
    consume(TokenType::Identifier, "Expected superclass method name.");
    int name = identifier_constant(std::string(previous.get_text()));
    
    named_variable("this", false);
    named_variable("super", false);
    emit(Opcode::GetSuper, static_cast<uint8_t>(name));
}

void Parser::this_([[maybe_unused]] bool can_assign)
{
    if (class_compiler == nullptr) {
        error("'this' cannot be outside of a class.");
        return;
    }
    variable(false);
}

void Parser::and_([[maybe_unused]] bool can_assign)
{
    int end_jump = emit_jump(Opcode::JumpIfFalse);
    
    emit(Opcode::Pop);
    parse_precedence(PrecedenceType::And);
    
    patch_jump(end_jump);
}

void Parser::unary([[maybe_unused]] bool can_assign)
{
    TokenType operator_type = previous.get_type();
    
    // Compile the operand.
    parse_precedence(PrecedenceType::Unary);
    
    // Emit the operator instruciton.
    switch (operator_type) {
        case TokenType::Excl: emit(Opcode::Not); break;
        case TokenType::Minus: emit(Opcode::Negate); break;
        case TokenType::BwNot: emit(Opcode::BwNot); break;
            
        default:
            return; // Unreachable.
    }
}

ParseRule& Parser::get_rule(TokenType type)
{
    return rules[static_cast<int>(type)];
}

void Parser::build_parse_rules()
{
    std::function<void(bool)> grouping = [this](bool can_assign) { this->grouping(can_assign); };
    std::function<void(bool)> unary = [this](bool can_assign) { this->unary(can_assign); };
    std::function<void(bool)> binary = [this](bool can_assign) { this->binary(can_assign); };
    std::function<void(bool)> call = [this](bool can_assign) { this->call(can_assign); };
    std::function<void(bool)> dot = [this](bool can_assign) { this->dot(can_assign); };
    std::function<void(bool)> number = [this](bool can_assign) { this->number(can_assign); };
    std::function<void(bool)> string = [this](bool can_assign) { this->string_(can_assign); };
    std::function<void(bool)> literal = [this](bool can_assign) { this->literal(can_assign); };
    std::function<void(bool)> variable = [this](bool can_assign) { this->variable(can_assign); };
    std::function<void(bool)> super_ = [this](bool can_assign) { this->super(can_assign); };
    std::function<void(bool)> this_ = [this](bool can_assign) { this->this_(can_assign); };
    std::function<void(bool)> and_ = [this](bool can_assign) { this->and_(can_assign); };
    std::function<void(bool)> or_ = [this](bool can_assign) { this->or_(can_assign); };
    std::function<void(bool)> array = [this](bool can_assign) { this->array(can_assign); };
    std::function<void(bool)> array_idx = [this](bool can_assign) { this->array_idx(can_assign); };

    rules = { {
        { grouping,    call,       PrecedenceType::Call },       // TOKEN_LEFT_PAREN
        { nullptr,     nullptr,    PrecedenceType::None },       // TOKEN_RIGHT_PAREN
        { nullptr,     nullptr,    PrecedenceType::None },       // TOKEN_LEFT_BRACE
        { nullptr,     nullptr,    PrecedenceType::None },       // TOKEN_RIGHT_BRACE
        { nullptr,     nullptr,    PrecedenceType::None },       // TOKEN_COMMA
        { nullptr,     dot,        PrecedenceType::Call },       // TOKEN_DOT
        { unary,       binary,     PrecedenceType::Term },       // TOKEN_MINUS
        { nullptr,     binary,     PrecedenceType::Term },       // TOKEN_PLUS
        { nullptr,     nullptr,    PrecedenceType::None },       // TOKEN_SEMICOLON
        { nullptr,     binary,     PrecedenceType::Factor },     // TOKEN_SLASH
        { nullptr,     binary,     PrecedenceType::Factor },     // TOKEN_STAR
        { unary,       nullptr,    PrecedenceType::None },       // TOKEN_BANG
        { nullptr,     binary,     PrecedenceType::Equality },   // TOKEN_BANG_EQUAL
        { nullptr,     nullptr,    PrecedenceType::None },       // TOKEN_EQUAL
        { nullptr,     binary,     PrecedenceType::Equality },   // TOKEN_EQUAL_EQUAL
        { nullptr,     binary,     PrecedenceType::Comparison }, // TOKEN_GREATER
        { nullptr,     binary,     PrecedenceType::Comparison }, // TOKEN_GREATER_EQUAL
        { nullptr,     binary,     PrecedenceType::Comparison }, // TOKEN_LESS
        { nullptr,     binary,     PrecedenceType::Comparison }, // TOKEN_LESS_EQUAL
        { variable,    nullptr,    PrecedenceType::None },       // TOKEN_IDENTIFIER
        { string,      nullptr,    PrecedenceType::None },       // TOKEN_STRING
        { number,      nullptr,    PrecedenceType::None },       // TOKEN_NUMBER
        { nullptr,     and_,       PrecedenceType::And },        // TOKEN_AND
        { nullptr,     nullptr,    PrecedenceType::None },       // TOKEN_CLASS
        { nullptr,     nullptr,    PrecedenceType::None },       // TOKEN_ELSE
        { literal,     nullptr,    PrecedenceType::None },       // TOKEN_FALSE
        { nullptr,     nullptr,    PrecedenceType::None },       // TOKEN_FUN
        { nullptr,     nullptr,    PrecedenceType::None },       // TOKEN_FOR
        { nullptr,     nullptr,    PrecedenceType::None },       // TOKEN_IF
        { literal,     nullptr,    PrecedenceType::None },       // TOKEN_NIL
        { nullptr,     or_,        PrecedenceType::Or },         // TOKEN_OR
        { nullptr,     nullptr,    PrecedenceType::None },       // TOKEN_PRINT
        { nullptr,     nullptr,    PrecedenceType::None },       // TOKEN_RETURN
        { super_,      nullptr,    PrecedenceType::None },       // TOKEN_SUPER
        { this_,       nullptr,    PrecedenceType::None },       // TOKEN_THIS
        { literal,     nullptr,    PrecedenceType::None },       // TOKEN_TRUE
        { nullptr,     nullptr,    PrecedenceType::None },       // TOKEN_VAR
        { nullptr,     nullptr,    PrecedenceType::None },       // TOKEN_WHILE
        { nullptr,     nullptr,    PrecedenceType::None },       // TOKEN_ERROR
        { nullptr,     nullptr,    PrecedenceType::None },       // TOKEN_EOF
        { nullptr,     binary,     PrecedenceType::Term },       // TOKEN_BW_AND
        { nullptr,     binary,     PrecedenceType::Term },       // TOKEN_BW_OR
        { nullptr,     binary,     PrecedenceType::Term },       // TOKEN_BW_XOR
        { unary,       nullptr,    PrecedenceType::Unary},       // TOKEN_BW_NOT
        { array,       array_idx,  PrecedenceType::Or   },       // TOKEN_OPEN_SQUARE
        { unary,       nullptr,    PrecedenceType::None },       // TOKEN_CLOSE_SQUARE
    } };
}

void Parser::parse_precedence(PrecedenceType precedence)
{
    advance();
    ParseFn prefix_rule = get_rule(previous.get_type()).prefix;
    if (prefix_rule == nullptr) {
        error("Expected expression.");
        return;
    }
    
    bool can_assign = precedence <= PrecedenceType::Assignment;
    prefix_rule(can_assign);
    
    while (precedence <= get_rule(current.get_type()).precedence) {
        advance();
        ParseFn infix_rule = get_rule(previous.get_type()).infix;
        infix_rule(can_assign);
    }
    
    if (can_assign && match(TokenType::Equal)) {
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
    consume(TokenType::Identifier, errorMessage);
    
    compiler->declare_variable(std::string(previous.get_text()));
    if (compiler->is_local()) return 0;
    
    return static_cast<uint8_t>(identifier_constant(std::string(previous.get_text())));
}

void Parser::define_variable(uint8_t global)
{
    if (compiler->is_local()) {
        compiler->mark_initialized();
        return;
    }
    
    emit(Opcode::DefineGlobal, global);
}

uint8_t Parser::args_list()
{
    uint8_t arg_count = 0;
    if (!check(TokenType::CloseParen)) {
        do {
            expression();
            if (arg_count == 255) {
                error("A function cannot have more than 255 arguments.");
            }
            arg_count++;
        } while (match(TokenType::Comma));
    }
    consume(TokenType::CloseParen, "Expected ')' after arguments.");
    return arg_count;
}

void Parser::expression()
{
    parse_precedence(PrecedenceType::Assignment);
}

void Parser::block()
{
    while (!check(TokenType::CloseCurly) && !check(TokenType::Eof)) {
        declaration();
    }
    
    consume(TokenType::CloseCurly, "Expected '}' after block.");
}

void Parser::function(FunctionType type)
{
    compiler = std::make_unique<Compiler>(this, type, std::move(compiler));
    compiler->begin_scope();

    consume(TokenType::OpenParen, "Expected '(' after function name.");
    if (!check(TokenType::CloseParen)) {
        do {
            compiler->function->arity++;
            if (compiler->function->arity > 255)
                error_at_current("A function cannot have more than 255 parameters.");
            uint8_t constant = parse_variable("Expected parameter name.");
            define_variable(constant);
        } while (match(TokenType::Comma));
    }
    consume(TokenType::CloseParen, "Expected ')' after parameters.");
    consume(TokenType::OpenCurly, "Expected '{' before function body.");
    block();
    
    Func function = end_compiler();
    std::unique_ptr<Compiler> new_compiler = std::move(compiler);
    compiler = std::move(new_compiler->enclosing);

    emit(Opcode::Closure, make_constant(function));
    
    for (const UpvalueVar& upvalue : new_compiler->upvalues) {
        emit(upvalue.is_local ? 1 : 0);
        emit(upvalue.index);
    }
}

void Parser::method()
{
    consume(TokenType::Identifier, "Expected method name.");
    const int constant = identifier_constant(std::string(previous.get_text()));
    const FunctionType type = previous.get_text() == "init" ? FunctionType::Initializer : FunctionType::Method;
    function(type);
    emit(Opcode::Method, static_cast<uint8_t>(constant));
}

void Parser::class_declaration()
{
    consume(TokenType::Identifier, "Expected class name.");
    const std::string class_name = std::string(previous.get_text());
    const int name_constant = identifier_constant(class_name);
    compiler->declare_variable(std::string(previous.get_text()));
    
    emit(Opcode::Class, static_cast<uint8_t>(name_constant));
    define_variable(static_cast<uint8_t>(name_constant));
    
    class_compiler = std::make_unique<ClassCompiler>(std::move(class_compiler));
    
    if (match(TokenType::Less)) {
        consume(TokenType::Identifier, "Expected superclass name.");
        variable(false);
        
        if (class_name == previous.get_text()) {
            error("A class cannot inherit from itself.");
        }
        
        compiler->begin_scope();
        compiler->add_local("super");
        define_variable(0);
        
        named_variable(class_name, false);
        emit(Opcode::Inherit);
        class_compiler->has_superclass = true;
    }
    
    named_variable(class_name, false);
    consume(TokenType::OpenCurly, "Expected '{' before class body.");
    while (!check(TokenType::CloseCurly) && !check(TokenType::Eof)) {
        method();
    }
    consume(TokenType::CloseCurly, "Expected '}' after class body.");
    emit(Opcode::Pop);
    
    if (class_compiler->has_superclass) {
        compiler->end_scope();
    }
    
    class_compiler = std::move(class_compiler->enclosing);
}

void Parser::func_declaration()
{
    uint8_t global = parse_variable("Expected function name.");
    compiler->mark_initialized();
    function(FunctionType::Function);
    define_variable(global);
}

void Parser::var_declaration()
{
    const uint8_t global = parse_variable("Expected variable name.");

    if (match(TokenType::Equal)) {
        expression();
    } else {
        emit(Opcode::Nop);
    }
    consume(TokenType::Semicolon, "Expected ';' after variable declaration.");
    
    define_variable(global);
}

void Parser::expression_statement()
{
    expression();
    emit(Opcode::Pop);
    consume(TokenType::Semicolon, "Expected ';' after expression.");
}

void Parser::for_statement()
{
    compiler->begin_scope();
    
    consume(TokenType::OpenParen, "Expected '(' after 'for'.");
    if (match(TokenType::Var)) {
        var_declaration();
    } else if (match(TokenType::Semicolon)) {
        // no initializer.
    } else {
        expression_statement();
    }
    
    int loop_start = current_chunk().count();
    
    int exit_jump = -1;
    if (!match(TokenType::Semicolon)) {
        expression();
        consume(TokenType::Semicolon, "Expected ';' after loop condition.");
        
        // Jump out of the loop if the condition is false.
        exit_jump = emit_jump(Opcode::JumpIfFalse);
        emit(Opcode::Pop);
    }

    if (!match(TokenType::CloseParen)) {
        const int body_jump = emit_jump(Opcode::Jump);
        
        int increment_start = current_chunk().count();
        expression();
        emit(Opcode::Pop);
        consume(TokenType::CloseParen, "Expected ')' after for clauses.");
        
        emit_loop(loop_start);
        loop_start = increment_start;
        patch_jump(body_jump);
    }
    
    statement();
    
    emit_loop(loop_start);
    
    if (exit_jump != -1) {
        patch_jump(exit_jump);
        emit(Opcode::Pop); // Condition.
    }

    compiler->end_scope();
}

void Parser::if_statement()
{
    consume(TokenType::OpenParen, "Expected '(' after 'if'.");
    expression();
    consume(TokenType::CloseParen, "Expected ')' after condition.");
    
    const int then_jump = emit_jump(Opcode::JumpIfFalse);
    emit(Opcode::Pop);
    statement();
    const int else_jump = emit_jump(Opcode::Jump);
    
    patch_jump(then_jump);
    emit(Opcode::Pop);
    if (match(TokenType::Else)) { statement(); }
    patch_jump(else_jump);
}

void Parser::declaration()
{
    if (match(TokenType::Class)) {
        class_declaration();
    } else if (match(TokenType::Func)) {
        func_declaration();
    } else if (match(TokenType::Var)) {
        var_declaration();
    } else {
        statement();
    }
    
    if (panic_mode) sync();
}

void Parser::statement()
{
    if (match(TokenType::Print)) {
        print_statement();
    } else if (match(TokenType::For)) {
        for_statement();
    } else if (match(TokenType::If)) {
        if_statement();
    } else if (match(TokenType::Return)) {
        return_statement();
    } else if (match(TokenType::While)) {
        while_statement();
    } else if (match(TokenType::OpenCurly)) {
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
    consume(TokenType::Semicolon, "Expected ';' after value.");
    emit(Opcode::Print);
}

void Parser::return_statement()
{
    if (compiler->type == FunctionType::Script) {
        error("Cannot return from top-level code.");
    }
    
    if (match(TokenType::Semicolon)) {
        emit_return();
    } else {
        if (compiler->type == FunctionType::Initializer) {
            error("Cannot return a value from an initializer.");
        }
        
        expression();
        consume(TokenType::Semicolon, "Expected ';' after return value.");
        emit(Opcode::Return);
    }
}

void Parser::while_statement()
{
    const int loop_start = current_chunk().count();
    
    consume(TokenType::OpenParen, "Expected '(' after 'while'.");
    expression();
    consume(TokenType::CloseParen, "Expected ')' after condition.");
    
    const int exit_jump = emit_jump(Opcode::JumpIfFalse);
    
    emit(Opcode::Pop);
    statement();
    
    emit_loop(loop_start);
    
    patch_jump(exit_jump);
    emit(Opcode::Pop);
}

void Parser::sync()
{
    panic_mode = false;
    
    while (current.get_type() != TokenType::Eof) {
        if (previous.get_type() == TokenType::Semicolon) return;
        
        switch (current.get_type()) {
            case TokenType::Class:
            case TokenType::Func:
            case TokenType::If:
            case TokenType::While:
            case TokenType::Print:
            case TokenType::Return:
                return;
                
            default: /* do nothing */ ;;
        }
        
        advance();
    }
}

void Parser::error_at(const Token& token, const std::string& message)
{
    if (panic_mode) return;
    
    panic_mode = true;
    
    fmt::print(stderr, "[line {} ] Error", token.get_line());
    if (token.get_type() == TokenType::Eof) {
        fmt::print(stderr, " at end");
    } else if (token.get_type() == TokenType::Error) {
        // Nothing.
        ;;
    } else {
        fmt::print(stderr, " at '{}'", token.get_text());
    }
    
    fmt::print(stderr, ": {}\n", message);
    had_error = true;
}
