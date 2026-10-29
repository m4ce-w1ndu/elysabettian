#include "ely/compiler/parser.hpp"
#include "ely/core/opcode.hpp"
#include "ely/core/value.hpp"
#include <ely/compiler/compiler.hpp>
#include <charconv>
#include <format>
#include <string_view>
#include <system_error>

namespace ely::compiler {

/**
 * Parses a double value from a string view.
 *
 * @param str String value to parse as double.
 */
static std::optional<double> parse_double(std::string_view str) {
    double value;
    auto [ptr, ec] = std::from_chars(str.data(), str.data() + str.size(), value);

    if (ec != std::errc{}) return std::nullopt;

    return value;
}

constexpr Precedence Compiler::get_precedence(TokenType token_type) noexcept {
    switch (token_type) {
        case TokenType::Multiply:
        case TokenType::Divide: return Precedence::Factor;
        case TokenType::Plus:
        case TokenType::Minus: return Precedence::Term;
        case TokenType::LessThan:
        case TokenType::LessThanEqual:
        case TokenType::GreaterThan:
        case TokenType::GreaterThanEqual: return Precedence::Comparison;
        case TokenType::Equality:
        case TokenType::NonEquality: return Precedence::Equality;
        case TokenType::BwOr:
        case TokenType::BwXor:
        case TokenType::BwAnd:
        case TokenType::ShiftLeft:
        case TokenType::ShiftRight: return Precedence::Term;
        case TokenType::And: return Precedence::And;
        case TokenType::Or: return Precedence::Or;
        case TokenType::OpenParen: return Precedence::Call;
        default: return Precedence::None;
    }
}

void Compiler::expression() {
    parse_precedence(Precedence::Assignment);
}

void Compiler::number() {
    const auto value = parse_double(m_parser.previous().lexeme);
    const auto boxed_value = core::Value::number(
        value
            ? *value
            : 0.0
    );
    emit_constant(boxed_value);
}

void Compiler::grouping() {
    expression();
    m_parser.consume(TokenType::CloseParen, "Expected ')' after expression.");
}

void Compiler::unary() {
    const auto operator_type = m_parser.previous().token_type;
    parse_precedence(Precedence::Unary);

    switch (operator_type) {
        case TokenType::Minus: return emit_opcode(core::Opcode::Negate);
        case TokenType::Bang: return emit_opcode(core::Opcode::Not);
        case TokenType::BwNot: return emit_opcode(core::Opcode::BwNot);
        default: return;
    }
}

void Compiler::prefix_inc_dec() {
    const auto is_inc = m_parser.previous().token_type == TokenType::PlusPlus;
    m_parser.consume(TokenType::Identifier, "Expected variable name after '++' or '--'.");
    const auto name = m_parser.previous();

    // Figure out if the variable is local or global once to avoid multile scope lookups
    const auto slot = resolve_local(name);
    const auto [get_op, set_op, arg] = slot
        ? std::tuple(core::Opcode::GetLocal, core::Opcode::SetLocal, *slot)
        : std::tuple(core::Opcode::GetGlobal, core::Opcode::SetGlobal, identifier_constant(name));

    emit_opcode(get_op);
    emit_byte(arg);

    emit_constant(core::Value::number(1.0));
    emit_opcode(is_inc
        ? core::Opcode::Add
        : core::Opcode::Subtract
    );

    emit_opcode(set_op);
    emit_byte(arg);
}

void Compiler::postfix_inc_dec(core::Opcode get_op, core::Opcode set_op, uint8_t arg) {
    const auto is_inc = m_parser.previous().token_type == TokenType::PlusPlus;

    // Postfix trick: push the current value twice.
    // The first copy stays on the stack as the expression result (old value),
    // while the second copy is modified and saved back into the variable.
    emit_opcode(get_op);
    emit_byte(arg);

    emit_opcode(get_op);
    emit_byte(arg);

    emit_constant(core::Value::number(1.0));
    emit_opcode(is_inc
        ? core::Opcode::Add
        : core::Opcode::Subtract
    );

    // SetLocal / SetGlobal leaves the updated value on top of the stack.
    // Pop that updated value so only the original old value remains on the stack.
    emit_opcode(set_op);
    emit_byte(arg);
    emit_opcode(core::Opcode::Pop);
}

void Compiler::binary() {
    const auto operator_type = m_parser.previous().token_type;
    const auto precedence = get_precedence(operator_type);
    parse_precedence(next(precedence));

    switch (operator_type) {
        case TokenType::Plus: return emit_opcode(core::Opcode::Add);
        case TokenType::Minus: return emit_opcode(core::Opcode::Subtract);
        case TokenType::Multiply: return emit_opcode(core::Opcode::Multiply);
        case TokenType::Divide: return emit_opcode(core::Opcode::Divide);
        case TokenType::Equality: return emit_opcode(core::Opcode::Equal);
        case TokenType::NonEquality: {
            emit_opcode(core::Opcode::Equal);
            emit_opcode(core::Opcode::Not);
            break;
        }
        case TokenType::GreaterThan: return emit_opcode(core::Opcode::Greater);
        case TokenType::LessThan: return emit_opcode(core::Opcode::Less);
        case TokenType::GreaterThanEqual: {
            emit_opcode(core::Opcode::Less);
            emit_opcode(core::Opcode::Not);
            break;
        }
        case TokenType::LessThanEqual: {
            emit_opcode(core::Opcode::Greater);
            emit_opcode(core::Opcode::Not);
            break;
        }
        case TokenType::BwAnd: return emit_opcode(core::Opcode::BwAnd);
        case TokenType::BwOr: return emit_opcode(core::Opcode::BwOr);
        case TokenType::BwXor: return emit_opcode(core::Opcode::BwXor);
        case TokenType::ShiftLeft: return emit_opcode(core::Opcode::ShiftLeft);
        case TokenType::ShiftRight: return emit_opcode(core::Opcode::ShiftRight);
        default: return;
    }
}

void Compiler::literal() {
    switch (m_parser.previous().token_type) {
        case TokenType::True: return emit_opcode(core::Opcode::True);
        case TokenType::False: return emit_opcode(core::Opcode::False);
        case TokenType::Null: return emit_opcode(core::Opcode::Nop);
        default: return;
    }
}

void Compiler::string() {
    const auto lexeme = m_parser.previous().lexeme;
    const auto value = lexeme.substr(1, lexeme.size() - 2);
    const auto string_idx = m_chunk.add_string_constant(std::string(value));

    emit_constant(core::Value::obj(static_cast<uint64_t>(string_idx)));
}

void Compiler::named_variable(Token name, bool can_assign) {
    const auto slot = resolve_local(name);
    const auto [get_op, set_op, arg] = slot
        ? std::tuple(core::Opcode::GetLocal, core::Opcode::SetLocal, *slot)
        : std::tuple(core::Opcode::GetGlobal, core::Opcode::SetGlobal, identifier_constant(name));

    if (can_assign && m_parser.match_token(TokenType::Assign)) {
        expression();
        emit_opcode(set_op);
        emit_byte(arg);
    } else if (can_assign
        && (m_parser.match_token(TokenType::PlusAssign)
            || m_parser.match_token(TokenType::MinusAssign)
            || m_parser.match_token(TokenType::MultiplyAssign)
            || m_parser.match_token(TokenType::DivideAssign))) {
                // Save the operator
                const auto op = m_parser.previous().token_type;

                // Read value
                emit_opcode(get_op);
                emit_byte(arg);

                expression();

                // Emit opcode based on operator type
                switch (op) {
                    case TokenType::PlusAssign: emit_opcode(core::Opcode::Add); break;
                    case TokenType::MinusAssign: emit_opcode(core::Opcode::Subtract); break;
                    case TokenType::MultiplyAssign: emit_opcode(core::Opcode::Multiply); break;
                    case TokenType::DivideAssign: emit_opcode(core::Opcode::Divide); break;
                    default: break;
                }

                // Store result
                emit_opcode(set_op);
                emit_byte(arg);
    } else if (m_parser.match_token(TokenType::PlusPlus) || m_parser.match_token(TokenType::MinusMinus)) {
        postfix_inc_dec(get_op, set_op, arg);
    } else {
        emit_opcode(get_op);
        emit_byte(arg);
    }
}

void Compiler::variable(bool can_assign) {
    named_variable(m_parser.previous(), can_assign);
}

void Compiler::and_op() {
    const auto end_jump = emit_jump(core::Opcode::JumpZero);
    emit_opcode(core::Opcode::Pop);
    parse_precedence(Precedence::And);
    patch_jump(end_jump);
}

void Compiler::or_op() {
    const auto else_jump = emit_jump(core::Opcode::JumpZero);
    const auto end_jump = emit_jump(core::Opcode::Jump);
    patch_jump(else_jump);
    emit_opcode(core::Opcode::Pop);
    parse_precedence(Precedence::Or);
    patch_jump(end_jump);
}

uint8_t Compiler::argument_list() {
    uint8_t arg_count = 0;

    if (!m_parser.check(TokenType::CloseParen)) {
        while (true) {
            expression();

            if (arg_count == kMaxArgs) {
                m_parser.error(std::format("Cannot have more than {} arguments.", kMaxArgs));
            }

            arg_count++;

            if (!m_parser.match_token(TokenType::Comma)) break;
        }
    }

    m_parser.consume(TokenType::CloseParen, "Expected ')' after arguments.");
    return arg_count;
}

void Compiler::call() {
    const auto arg_count = argument_list();
    emit_opcode(core::Opcode::Call);
    emit_byte(arg_count);
}

void Compiler::parse_precedence(Precedence precedence) {
    m_parser.advance();

    const auto can_assign = precedence <= Precedence::Assignment;
    switch (m_parser.previous().token_type) {
        case TokenType::OpenParen: grouping(); break;
        case TokenType::Minus:
        case TokenType::Bang:
        case TokenType::BwNot: unary(); break;
        case TokenType::PlusPlus:
        case TokenType::MinusMinus: prefix_inc_dec(); break;
        case TokenType::False:
        case TokenType::True:
        case TokenType::Null: literal(); break;
        case TokenType::Number: number(); break;
        case TokenType::String: string(); break;
        case TokenType::Identifier: variable(can_assign); break;
        default: {
            m_parser.error("Expected expression.");
            return;
        }
    }

    while (precedence <= get_precedence(m_parser.current().token_type)) {
        m_parser.advance();
        switch (m_parser.previous().token_type) {
            case TokenType::And: and_op(); break;
            case TokenType::Or: or_op(); break;
            case TokenType::OpenParen: call(); break;
            default: binary(); break;
        }
    }

    if (can_assign
        && (m_parser.match_token(TokenType::Assign)
            || m_parser.match_token(TokenType::PlusAssign)
            || m_parser.match_token(TokenType::MinusAssign)
            || m_parser.match_token(TokenType::MultiplyAssign)
            || m_parser.match_token(TokenType::DivideAssign))) {
        m_parser.error("Invalid assignment target.");
    }
}

uint8_t Compiler::identifier_constant(Token name) {
    const auto string_idx = m_chunk.add_string_constant(std::string(name.lexeme));
    return static_cast<uint8_t>(string_idx);
}

}
