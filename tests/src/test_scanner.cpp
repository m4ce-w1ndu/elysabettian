#include <catch2/catch_test_macros.hpp>
#include <ely/compiler/scanner.hpp>

TEST_CASE("Scanner tokenizes basic keywords and identifiers", "[compiler][scanner]") {
    std::string_view source = "var foo = 42; if (foo == 42) return true;";
    ely::compiler::Scanner scanner(source);

    auto t1 = scanner.scan_token();
    REQUIRE(t1.token_type == ely::compiler::TokenType::Var);
    REQUIRE(t1.lexeme == "var");

    auto t2 = scanner.scan_token();
    REQUIRE(t2.token_type == ely::compiler::TokenType::Identifier);
    REQUIRE(t2.lexeme == "foo");

    auto t3 = scanner.scan_token();
    REQUIRE(t3.token_type == ely::compiler::TokenType::Assign);

    auto t4 = scanner.scan_token();
    REQUIRE(t4.token_type == ely::compiler::TokenType::Number);
    REQUIRE(t4.lexeme == "42");

    auto t5 = scanner.scan_token();
    REQUIRE(t5.token_type == ely::compiler::TokenType::Semicolon);

    auto t6 = scanner.scan_token();
    REQUIRE(t6.token_type == ely::compiler::TokenType::If);

    auto t7 = scanner.scan_token();
    REQUIRE(t7.token_type == ely::compiler::TokenType::OpenParen);

    auto t8 = scanner.scan_token();
    REQUIRE(t8.token_type == ely::compiler::TokenType::Identifier);
    REQUIRE(t8.lexeme == "foo");

    auto t9 = scanner.scan_token();
    REQUIRE(t9.token_type == ely::compiler::TokenType::Equality);

    auto t10 = scanner.scan_token();
    REQUIRE(t10.token_type == ely::compiler::TokenType::Number);

    auto t11 = scanner.scan_token();
    REQUIRE(t11.token_type == ely::compiler::TokenType::CloseParen);

    auto t12 = scanner.scan_token();
    REQUIRE(t12.token_type == ely::compiler::TokenType::Return);

    auto t13 = scanner.scan_token();
    REQUIRE(t13.token_type == ely::compiler::TokenType::True);

    auto t14 = scanner.scan_token();
    REQUIRE(t14.token_type == ely::compiler::TokenType::Semicolon);

    auto t15 = scanner.scan_token();
    REQUIRE(t15.token_type == ely::compiler::TokenType::Eof);
}
