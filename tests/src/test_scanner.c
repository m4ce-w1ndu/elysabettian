#include "test_harness.h"
#include <ely/compiler/scanner.h>
#include <string.h>

static void test_scanner_basic_program(void) {
    const char* source = "var foo = 42; if (foo == 42) return true;";
    ElyScanner scanner;
    ely_scanner_init(&scanner, source);

    ElyToken t1 = ely_scanner_scan_token(&scanner);
    TEST_ASSERT_EQ(t1.token_type, ELY_TT_VAR);
    TEST_ASSERT_EQ(t1.length, 3);
    TEST_ASSERT(strncmp(t1.lexeme, "var", t1.length) == 0);

    ElyToken t2 = ely_scanner_scan_token(&scanner);
    TEST_ASSERT_EQ(t2.token_type, ELY_TT_IDENTIFIER);
    TEST_ASSERT_EQ(t2.length, 3);
    TEST_ASSERT(strncmp(t2.lexeme, "foo", t2.length) == 0);

    ElyToken t3 = ely_scanner_scan_token(&scanner);
    TEST_ASSERT_EQ(t3.token_type, ELY_TT_ASSIGN);

    ElyToken t4 = ely_scanner_scan_token(&scanner);
    TEST_ASSERT_EQ(t4.token_type, ELY_TT_NUMBER);
    TEST_ASSERT_EQ(t4.length, 2);
    TEST_ASSERT(strncmp(t4.lexeme, "42", t4.length) == 0);

    ElyToken t5 = ely_scanner_scan_token(&scanner);
    TEST_ASSERT_EQ(t5.token_type, ELY_TT_SEMICOLON);

    ElyToken t6 = ely_scanner_scan_token(&scanner);
    TEST_ASSERT_EQ(t6.token_type, ELY_TT_IF);

    ElyToken t7 = ely_scanner_scan_token(&scanner);
    TEST_ASSERT_EQ(t7.token_type, ELY_TT_OPENPAREN);

    ElyToken t8 = ely_scanner_scan_token(&scanner);
    TEST_ASSERT_EQ(t8.token_type, ELY_TT_IDENTIFIER);

    ElyToken t9 = ely_scanner_scan_token(&scanner);
    TEST_ASSERT_EQ(t9.token_type, ELY_TT_EQUALITY);

    ElyToken t10 = ely_scanner_scan_token(&scanner);
    TEST_ASSERT_EQ(t10.token_type, ELY_TT_NUMBER);

    ElyToken t11 = ely_scanner_scan_token(&scanner);
    TEST_ASSERT_EQ(t11.token_type, ELY_TT_CLOSEPAREN);

    ElyToken t12 = ely_scanner_scan_token(&scanner);
    TEST_ASSERT_EQ(t12.token_type, ELY_TT_RETURN);

    ElyToken t13 = ely_scanner_scan_token(&scanner);
    TEST_ASSERT_EQ(t13.token_type, ELY_TT_TRUE);

    ElyToken t14 = ely_scanner_scan_token(&scanner);
    TEST_ASSERT_EQ(t14.token_type, ELY_TT_SEMICOLON);

    ElyToken t15 = ely_scanner_scan_token(&scanner);
    TEST_ASSERT_EQ(t15.token_type, ELY_TT_EOF);
}

static void test_scanner_operators(void) {
    const char* source = "+ ++ += - -- -= * *= / /= == != < <= << > >= >> && || ~ ^";
    ElyScanner scanner;
    ely_scanner_init(&scanner, source);

    TEST_ASSERT_EQ(ely_scanner_scan_token(&scanner).token_type, ELY_TT_PLUS);
    TEST_ASSERT_EQ(ely_scanner_scan_token(&scanner).token_type, ELY_TT_PLUSPLUS);
    TEST_ASSERT_EQ(ely_scanner_scan_token(&scanner).token_type, ELY_TT_PLUSASSIGN);

    TEST_ASSERT_EQ(ely_scanner_scan_token(&scanner).token_type, ELY_TT_MINUS);
    TEST_ASSERT_EQ(ely_scanner_scan_token(&scanner).token_type, ELY_TT_MINUSMINUS);
    TEST_ASSERT_EQ(ely_scanner_scan_token(&scanner).token_type, ELY_TT_MINUSASSIGN);

    TEST_ASSERT_EQ(ely_scanner_scan_token(&scanner).token_type, ELY_TT_MULTIPLY);
    TEST_ASSERT_EQ(ely_scanner_scan_token(&scanner).token_type, ELY_TT_MULTIPLYASSIGN);

    TEST_ASSERT_EQ(ely_scanner_scan_token(&scanner).token_type, ELY_TT_DIVIDE);
    TEST_ASSERT_EQ(ely_scanner_scan_token(&scanner).token_type, ELY_TT_DIVIDEASSIGN);

    TEST_ASSERT_EQ(ely_scanner_scan_token(&scanner).token_type, ELY_TT_EQUALITY);
    TEST_ASSERT_EQ(ely_scanner_scan_token(&scanner).token_type, ELY_TT_NONEQUALITY);

    TEST_ASSERT_EQ(ely_scanner_scan_token(&scanner).token_type, ELY_TT_LESSTHAN);
    TEST_ASSERT_EQ(ely_scanner_scan_token(&scanner).token_type, ELY_TT_LESSTHANEQUAL);
    TEST_ASSERT_EQ(ely_scanner_scan_token(&scanner).token_type, ELY_TT_SHIFTLEFT);

    TEST_ASSERT_EQ(ely_scanner_scan_token(&scanner).token_type, ELY_TT_GREATERTHAN);
    TEST_ASSERT_EQ(ely_scanner_scan_token(&scanner).token_type, ELY_TT_GREATERTHANEQUAL);
    TEST_ASSERT_EQ(ely_scanner_scan_token(&scanner).token_type, ELY_TT_SHIFTRIGHT);

    TEST_ASSERT_EQ(ely_scanner_scan_token(&scanner).token_type, ELY_TT_AND);
    TEST_ASSERT_EQ(ely_scanner_scan_token(&scanner).token_type, ELY_TT_OR);
    TEST_ASSERT_EQ(ely_scanner_scan_token(&scanner).token_type, ELY_TT_BWNOT);
    TEST_ASSERT_EQ(ely_scanner_scan_token(&scanner).token_type, ELY_TT_BWXOR);

    TEST_ASSERT_EQ(ely_scanner_scan_token(&scanner).token_type, ELY_TT_EOF);
}

static void test_scanner_strings_and_numbers(void) {
    const char* source = "\"hello world\" 'single' 123 45.678";
    ElyScanner scanner;
    ely_scanner_init(&scanner, source);

    ElyToken str1 = ely_scanner_scan_token(&scanner);
    TEST_ASSERT_EQ(str1.token_type, ELY_TT_STRING);
    TEST_ASSERT_EQ(str1.length, 13); // includes enclosing quotes

    ElyToken str2 = ely_scanner_scan_token(&scanner);
    TEST_ASSERT_EQ(str2.token_type, ELY_TT_STRING);
    TEST_ASSERT_EQ(str2.length, 8);

    ElyToken num1 = ely_scanner_scan_token(&scanner);
    TEST_ASSERT_EQ(num1.token_type, ELY_TT_NUMBER);
    TEST_ASSERT_EQ(num1.length, 3);
    TEST_ASSERT(strncmp(num1.lexeme, "123", 3) == 0);

    ElyToken num2 = ely_scanner_scan_token(&scanner);
    TEST_ASSERT_EQ(num2.token_type, ELY_TT_NUMBER);
    TEST_ASSERT_EQ(num2.length, 6);
    TEST_ASSERT(strncmp(num2.lexeme, "45.678", 6) == 0);

    TEST_ASSERT_EQ(ely_scanner_scan_token(&scanner).token_type, ELY_TT_EOF);
}

static void test_scanner_comments_and_lines(void) {
    const char* source = "// this is a comment\n"
                         "var a = 1;\n"
                         "// another comment\n"
                         "var b = 2;";
    ElyScanner scanner;
    ely_scanner_init(&scanner, source);

    ElyToken t1 = ely_scanner_scan_token(&scanner);
    TEST_ASSERT_EQ(t1.token_type, ELY_TT_VAR);
    TEST_ASSERT_EQ(t1.line, 2);

    TEST_ASSERT_EQ(ely_scanner_scan_token(&scanner).token_type, ELY_TT_IDENTIFIER); // a
    TEST_ASSERT_EQ(ely_scanner_scan_token(&scanner).token_type, ELY_TT_ASSIGN);     // =
    TEST_ASSERT_EQ(ely_scanner_scan_token(&scanner).token_type, ELY_TT_NUMBER);     // 1
    TEST_ASSERT_EQ(ely_scanner_scan_token(&scanner).token_type, ELY_TT_SEMICOLON);  // ;

    ElyToken t_var_b = ely_scanner_scan_token(&scanner); // 'var' on line 4
    TEST_ASSERT_EQ(t_var_b.token_type, ELY_TT_VAR);
    TEST_ASSERT_EQ(t_var_b.line, 4);
}

void run_scanner_tests(void) {
    RUN_TEST(test_scanner_basic_program);
    RUN_TEST(test_scanner_operators);
    RUN_TEST(test_scanner_strings_and_numbers);
    RUN_TEST(test_scanner_comments_and_lines);
}
