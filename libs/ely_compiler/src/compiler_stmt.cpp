#include "ely/compiler/scope.hpp"
#include "ely/compiler/token.hpp"
#include "ely/core/chunk.hpp"
#include "ely/core/opcode.hpp"
#include <ely/compiler/compiler.hpp>
#include <utility>
#include <memory>

namespace ely::compiler {

void Compiler::statement() {
    if (m_parser.match_token(TokenType::Print)) {
        print_statement();
    } else if (m_parser.match_token(TokenType::If)) {
        if_statement();
    } else if (m_parser.match_token(TokenType::While)) {
        while_statement();
    } else if (m_parser.match_token(TokenType::For)) {
        for_statement();
    } else if (m_parser.match_token(TokenType::Return)) {
        return_statement();
    } else if (m_parser.match_token(TokenType::OpenCurly)) {
        begin_scope();
        block();
        end_scope();
    } else {
        expression_statement();
    }
}

void Compiler::print_statement() {
    expression();
    m_parser.consume(TokenType::Semicolon, "Expected ';' after value.");
    emit_opcode(core::Opcode::Print);
}

void Compiler::expression_statement() {
    expression();
    m_parser.consume(TokenType::Semicolon, "Expected ';' after expression.");
    emit_opcode(core::Opcode::Pop);
}

void Compiler::declaration() {
    if (m_parser.match_token(TokenType::Var)) {
        var_declaration();
    } else if (m_parser.match_token(TokenType::Func)) {
        function_declaration();
    } else if (m_parser.match_token(TokenType::Import)) {
        import_statement();
    } else {
        statement();
    }

    if (m_parser.panic_mode()) {
        synchronize();
    }
}

void Compiler::import_statement() {
    // Parentheses are optional to support both function-like and statement-like import system.
    const auto has_paren = m_parser.match_token(TokenType::OpenParen);
    m_parser.consume(
        TokenType::String,
        "Expected module name string after 'import'."
    );

    // Strip quotes from the module name literal
    const auto lexeme = m_parser.previous().lexeme;
    const auto module_name = lexeme.size() >= 2
        ? lexeme.substr(1, lexeme.size() - 2)
        : "";

    const auto string_idx = m_chunk.add_string_constant(std::string(module_name));

    if (has_paren) {
        m_parser.consume(TokenType::CloseParen, "Expected ')' after module name.");
    }

    m_parser.consume(TokenType::Semicolon, "Expected ';' after import statement.");

    emit_opcode(core::Opcode::Import);
    emit_byte(static_cast<uint8_t>(string_idx));
}

void Compiler::var_declaration() {
    const auto global = parse_variable("Expected variable name.");

    if (m_parser.match_token(TokenType::Assign)) {
        expression();
    } else {
        emit_opcode(core::Opcode::Nop);
    }

    m_parser.consume(
        TokenType::Semicolon,
        "Expected ';' after variable declaration."
    );

    define_variable(global);
}

void Compiler::synchronize() {
    m_parser.reset_panic_mode();

    while (m_parser.current().token_type != TokenType::Eof) {
        if (m_parser.previous().token_type == TokenType::Semicolon) return;

        switch (m_parser.current().token_type) {
            case TokenType::Class:
            case TokenType::Func:
            case TokenType::Var:
            case TokenType::For:
            case TokenType::If:
            case TokenType::While:
            case TokenType::Print:
            case TokenType::Return: return;
            default: break;
        }

        m_parser.advance();
    }
}

void Compiler::block() {
    while (m_parser.current().token_type != TokenType::CloseCurly
        && m_parser.current().token_type != TokenType::Eof) {
        declaration();
    }

    m_parser.consume(TokenType::CloseCurly, "Expected '}' after block.");
}

void Compiler::if_statement() {
    m_parser.consume(TokenType::OpenParen, "Expected '(' after 'if'.");
    expression();
    m_parser.consume(TokenType::CloseParen, "Expected ')' after condition.");

    const auto then_jump = emit_jump(core::Opcode::JumpZero);
    emit_opcode(core::Opcode::Pop);
    statement();

    const auto else_jump = emit_jump(core::Opcode::Jump);
    patch_jump(then_jump);
    emit_opcode(core::Opcode::Pop);

    if (m_parser.match_token(TokenType::Else)) {
        statement();
    }

    patch_jump(else_jump);
}

void Compiler::while_statement() {
    const auto loop_start = m_chunk.size();

    m_parser.consume(TokenType::OpenParen, "Expected '(' after 'while'.");
    expression();
    m_parser.consume(TokenType::CloseParen, "Expected ')' after condition.");

    const auto exit_jump = emit_jump(core::Opcode::JumpZero);
    emit_opcode(core::Opcode::Pop);

    statement();
    emit_loop(loop_start);

    patch_jump(exit_jump);
    emit_opcode(core::Opcode::Pop);
}

void Compiler::for_statement() {
    begin_scope();
    m_parser.consume(TokenType::OpenParen, "Expected '(' after 'for'.");

    if (m_parser.match_token(TokenType::Semicolon)) {
        ;
    } else if (m_parser.match_token(TokenType::Var)) {
        var_declaration();
    } else {
        expression_statement();
    }

    auto loop_start = m_chunk.size();
    std::optional<size_t> exit_jump = std::nullopt;
    if (!m_parser.match_token(TokenType::Semicolon)) {
        expression();
        m_parser.consume(TokenType::Semicolon, "Expected ';' after loop condition.");
        exit_jump = emit_jump(core::Opcode::JumpZero);
        emit_opcode(core::Opcode::Pop);
    }

    if (!m_parser.match_token(TokenType::CloseParen)) {
        const auto body_jump = emit_jump(core::Opcode::Jump);
        const auto increment_start = m_chunk.size();
        expression();
        emit_opcode(core::Opcode::Pop);

        m_parser.consume(TokenType::CloseParen, "Expected ')' after 'for' clauses.");
        emit_loop(loop_start);
        loop_start = increment_start;
        patch_jump(body_jump);
    }

    statement();
    emit_loop(loop_start);

    if (exit_jump) {
        patch_jump(*exit_jump);
        emit_opcode(core::Opcode::Pop);
    }

    end_scope();
}

size_t Compiler::function() {
    begin_scope();
    m_parser.consume(TokenType::OpenParen, "Expected '(' after function name.");

    size_t arity = 0;
    if (!m_parser.check(TokenType::CloseParen)) {
        while (true) {
            arity++;
            if (arity > kMaxArgs) {
                m_parser.error(std::format("Cannot have more than {} parameters.", kMaxArgs));
                return arity;
            }

            const auto constant = parse_variable("Expected parameter name.");
            define_variable(constant);

            if (!m_parser.match_token(TokenType::Comma)) break;
        }
    }

    m_parser.consume(TokenType::CloseParen, "Expected ')' after function parameters.");
    m_parser.consume(TokenType::OpenCurly, "Expected '{' after function signature.");

    block();
    end_scope();
    emit_return();

    return arity;
}

void Compiler::function_declaration() {
    const auto global = parse_variable("Expected function name for signature.");
    const auto func_name = m_parser.previous().lexeme;

    if (m_context.scope_depth() > 0) mark_initialized();

    auto prev_chunk = std::exchange(m_chunk, core::Chunk{});
    const auto prev_type = m_context.function_type();
    m_context.set_function_type(FunctionType::Function);
    const auto prev_locals = std::exchange(m_context.locals(), {});
    const auto prev_depth = m_context.scope_depth();
    m_context.set_scope_depth(0);

    m_context.locals().push_back(Local {
        Token {
            TokenType::Identifier,
            "",
            0
        },
        0,
        false
    });

    const auto arity = function();

    auto func_chunk = std::exchange(m_chunk, std::move(prev_chunk));
    m_context.locals() = std::move(prev_locals);
    m_context.set_scope_depth(prev_depth);
    m_context.set_function_type(prev_type);

    auto desc = core::FunctionDescriptor{};
    desc.arity = arity;
    desc.chunk = std::make_unique<core::Chunk>(std::move(func_chunk));
    desc.name = std::string(func_name);

    const auto func_idx = m_chunk.add_function_constant(std::move(desc));
    const auto const_idx = m_chunk.add_constant(core::Value::obj(static_cast<uint64_t>(func_idx)));

    emit_opcode(core::Opcode::Closure);
    emit_byte(static_cast<uint8_t>(const_idx));

    define_variable(global);
}

void Compiler::return_statement() {
    if (m_context.function_type() == FunctionType::Script) {
        m_parser.error("Cannot return from main script.");
    }

    if (m_parser.match_token(TokenType::Semicolon)) {
        emit_return();
    } else {
        expression();
        m_parser.consume(TokenType::Semicolon, "Expected ';' after return value.");
        emit_opcode(core::Opcode::Return);
    }
}

uint8_t Compiler::parse_variable(std::string_view error_message) {
    m_parser.consume(TokenType::Identifier, error_message);
    declare_variable();
    if (m_context.scope_depth() > 0) return 0;
    return identifier_constant(m_parser.previous());
}

void Compiler::define_variable(uint8_t global) {
    if (m_context.scope_depth() > 0) {
        mark_initialized();
        return;
    }

    if (m_context.scope_depth() == 0) {
        emit_opcode(core::Opcode::DefineGlobal);
        emit_byte(global);
    }
}

}
