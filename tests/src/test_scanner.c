#include "test_harness.h"
#include <vtx/compiler/scanner.h>
#include <string.h>

static void test_scanner_basic_program(void) {
    const char* source = "var foo = 42; if (foo == 42) return true;";
    VtxScanner scanner;
    vtx_scanner_init(&scanner, source);

    VtxToken t1 = vtx_scanner_scan_token(&scanner);
    TEST_ASSERT_EQ(t1.token_type, VTX_TT_VAR);
    TEST_ASSERT_EQ(t1.length, 3);
    TEST_ASSERT(strncmp(t1.lexeme, "var", t1.length) == 0);

    VtxToken t2 = vtx_scanner_scan_token(&scanner);
    TEST_ASSERT_EQ(t2.token_type, VTX_TT_IDENTIFIER);
    TEST_ASSERT_EQ(t2.length, 3);
    TEST_ASSERT(strncmp(t2.lexeme, "foo", t2.length) == 0);

    VtxToken t3 = vtx_scanner_scan_token(&scanner);
    TEST_ASSERT_EQ(t3.token_type, VTX_TT_ASSIGN);

    VtxToken t4 = vtx_scanner_scan_token(&scanner);
    TEST_ASSERT_EQ(t4.token_type, VTX_TT_NUMBER);
    TEST_ASSERT_EQ(t4.length, 2);
    TEST_ASSERT(strncmp(t4.lexeme, "42", t4.length) == 0);

    VtxToken t5 = vtx_scanner_scan_token(&scanner);
    TEST_ASSERT_EQ(t5.token_type, VTX_TT_SEMICOLON);

    VtxToken t6 = vtx_scanner_scan_token(&scanner);
    TEST_ASSERT_EQ(t6.token_type, VTX_TT_IF);

    VtxToken t7 = vtx_scanner_scan_token(&scanner);
    TEST_ASSERT_EQ(t7.token_type, VTX_TT_OPENPAREN);

    VtxToken t8 = vtx_scanner_scan_token(&scanner);
    TEST_ASSERT_EQ(t8.token_type, VTX_TT_IDENTIFIER);

    VtxToken t9 = vtx_scanner_scan_token(&scanner);
    TEST_ASSERT_EQ(t9.token_type, VTX_TT_EQUALITY);

    VtxToken t10 = vtx_scanner_scan_token(&scanner);
    TEST_ASSERT_EQ(t10.token_type, VTX_TT_NUMBER);

    VtxToken t11 = vtx_scanner_scan_token(&scanner);
    TEST_ASSERT_EQ(t11.token_type, VTX_TT_CLOSEPAREN);

    VtxToken t12 = vtx_scanner_scan_token(&scanner);
    TEST_ASSERT_EQ(t12.token_type, VTX_TT_RETURN);

    VtxToken t13 = vtx_scanner_scan_token(&scanner);
    TEST_ASSERT_EQ(t13.token_type, VTX_TT_TRUE);

    VtxToken t14 = vtx_scanner_scan_token(&scanner);
    TEST_ASSERT_EQ(t14.token_type, VTX_TT_SEMICOLON);

    VtxToken t15 = vtx_scanner_scan_token(&scanner);
    TEST_ASSERT_EQ(t15.token_type, VTX_TT_EOF);
}

static void test_scanner_operators(void) {
    const char* source = "+ ++ += - -- -= * *= / /= == != < <= << > >= >> && || ~ ^";
    VtxScanner scanner;
    vtx_scanner_init(&scanner, source);

    TEST_ASSERT_EQ(vtx_scanner_scan_token(&scanner).token_type, VTX_TT_PLUS);
    TEST_ASSERT_EQ(vtx_scanner_scan_token(&scanner).token_type, VTX_TT_PLUSPLUS);
    TEST_ASSERT_EQ(vtx_scanner_scan_token(&scanner).token_type, VTX_TT_PLUSASSIGN);

    TEST_ASSERT_EQ(vtx_scanner_scan_token(&scanner).token_type, VTX_TT_MINUS);
    TEST_ASSERT_EQ(vtx_scanner_scan_token(&scanner).token_type, VTX_TT_MINUSMINUS);
    TEST_ASSERT_EQ(vtx_scanner_scan_token(&scanner).token_type, VTX_TT_MINUSASSIGN);

    TEST_ASSERT_EQ(vtx_scanner_scan_token(&scanner).token_type, VTX_TT_MULTIPLY);
    TEST_ASSERT_EQ(vtx_scanner_scan_token(&scanner).token_type, VTX_TT_MULTIPLYASSIGN);

    TEST_ASSERT_EQ(vtx_scanner_scan_token(&scanner).token_type, VTX_TT_DIVIDE);
    TEST_ASSERT_EQ(vtx_scanner_scan_token(&scanner).token_type, VTX_TT_DIVIDEASSIGN);

    TEST_ASSERT_EQ(vtx_scanner_scan_token(&scanner).token_type, VTX_TT_EQUALITY);
    TEST_ASSERT_EQ(vtx_scanner_scan_token(&scanner).token_type, VTX_TT_NONEQUALITY);

    TEST_ASSERT_EQ(vtx_scanner_scan_token(&scanner).token_type, VTX_TT_LESSTHAN);
    TEST_ASSERT_EQ(vtx_scanner_scan_token(&scanner).token_type, VTX_TT_LESSTHANEQUAL);
    TEST_ASSERT_EQ(vtx_scanner_scan_token(&scanner).token_type, VTX_TT_SHIFTLEFT);

    TEST_ASSERT_EQ(vtx_scanner_scan_token(&scanner).token_type, VTX_TT_GREATERTHAN);
    TEST_ASSERT_EQ(vtx_scanner_scan_token(&scanner).token_type, VTX_TT_GREATERTHANEQUAL);
    TEST_ASSERT_EQ(vtx_scanner_scan_token(&scanner).token_type, VTX_TT_SHIFTRIGHT);

    TEST_ASSERT_EQ(vtx_scanner_scan_token(&scanner).token_type, VTX_TT_AND);
    TEST_ASSERT_EQ(vtx_scanner_scan_token(&scanner).token_type, VTX_TT_OR);
    TEST_ASSERT_EQ(vtx_scanner_scan_token(&scanner).token_type, VTX_TT_BWNOT);
    TEST_ASSERT_EQ(vtx_scanner_scan_token(&scanner).token_type, VTX_TT_BWXOR);

    TEST_ASSERT_EQ(vtx_scanner_scan_token(&scanner).token_type, VTX_TT_EOF);
}

static void test_scanner_strings_and_numbers(void) {
    const char* source = "\"hello world\" 'single' 123 45.678";
    VtxScanner scanner;
    vtx_scanner_init(&scanner, source);

    VtxToken str1 = vtx_scanner_scan_token(&scanner);
    TEST_ASSERT_EQ(str1.token_type, VTX_TT_STRING);
    TEST_ASSERT_EQ(str1.length, 13); // includes enclosing quotes

    VtxToken str2 = vtx_scanner_scan_token(&scanner);
    TEST_ASSERT_EQ(str2.token_type, VTX_TT_STRING);
    TEST_ASSERT_EQ(str2.length, 8);

    VtxToken num1 = vtx_scanner_scan_token(&scanner);
    TEST_ASSERT_EQ(num1.token_type, VTX_TT_NUMBER);
    TEST_ASSERT_EQ(num1.length, 3);
    TEST_ASSERT(strncmp(num1.lexeme, "123", 3) == 0);

    VtxToken num2 = vtx_scanner_scan_token(&scanner);
    TEST_ASSERT_EQ(num2.token_type, VTX_TT_NUMBER);
    TEST_ASSERT_EQ(num2.length, 6);
    TEST_ASSERT(strncmp(num2.lexeme, "45.678", 6) == 0);

    TEST_ASSERT_EQ(vtx_scanner_scan_token(&scanner).token_type, VTX_TT_EOF);
}

static void test_scanner_comments_and_lines(void) {
    const char* source = "// this is a comment\n"
                         "var a = 1;\n"
                         "// another comment\n"
                         "var b = 2;";
    VtxScanner scanner;
    vtx_scanner_init(&scanner, source);

    VtxToken t1 = vtx_scanner_scan_token(&scanner);
    TEST_ASSERT_EQ(t1.token_type, VTX_TT_VAR);
    TEST_ASSERT_EQ(t1.line, 2);

    TEST_ASSERT_EQ(vtx_scanner_scan_token(&scanner).token_type, VTX_TT_IDENTIFIER); // a
    TEST_ASSERT_EQ(vtx_scanner_scan_token(&scanner).token_type, VTX_TT_ASSIGN);     // =
    TEST_ASSERT_EQ(vtx_scanner_scan_token(&scanner).token_type, VTX_TT_NUMBER);     // 1
    TEST_ASSERT_EQ(vtx_scanner_scan_token(&scanner).token_type, VTX_TT_SEMICOLON);  // ;

    VtxToken t_var_b = vtx_scanner_scan_token(&scanner); // 'var' on line 4
    TEST_ASSERT_EQ(t_var_b.token_type, VTX_TT_VAR);
    TEST_ASSERT_EQ(t_var_b.line, 4);
}

void run_scanner_tests(void) {
    RUN_TEST(test_scanner_basic_program);
    RUN_TEST(test_scanner_operators);
    RUN_TEST(test_scanner_strings_and_numbers);
    RUN_TEST(test_scanner_comments_and_lines);
}
