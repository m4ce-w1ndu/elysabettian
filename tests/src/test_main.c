#include "test_harness.h"
#include <stdlib.h>

ElyTestContext g_ely_test_ctx = {
    .tests_run = 0,
    .tests_failed = 0,
    .failed_assertions = 0
};

// Test suite declaration
void run_core_tests(void);

int main(void) {
    printf("========================================\n");
    printf(" Running Elysabettian Test Suite (C11)\n");
    printf("========================================\n");

    run_core_tests();

    printf("========================================\n");
    printf(" Results: %zu run, %zu failed, %zu assertion failures\n",
           g_ely_test_ctx.tests_run,
           g_ely_test_ctx.tests_failed,
           g_ely_test_ctx.failed_assertions);
    printf("========================================\n");

    return g_ely_test_ctx.tests_failed == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
