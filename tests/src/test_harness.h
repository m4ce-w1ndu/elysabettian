#ifndef ELY_TEST_HARNESS_H
#define ELY_TEST_HARNESS_H

#include <stdio.h>
#include <stdbool.h>
#include <stddef.h>
#include <math.h>

/**
 * Global test execution context tracking runs and failures.
 */
typedef struct ElyTestContext {
    size_t tests_run;
    size_t tests_failed;
    size_t failed_assertions;
} ElyTestContext;

extern ElyTestContext g_ely_test_ctx;

// Prototipi delle suite di test
void run_core_tests(void);

/**
 * Checks condition and records failure with file, line, and expression.
 */
#define TEST_ASSERT(cond) \
    do { \
        if (!(cond)) { \
            fprintf(stderr, "  [FAIL] %s:%d: Assertion failed: %s\n", __FILE__, __LINE__, #cond); \
            g_ely_test_ctx.failed_assertions++; \
        } \
    } while (0)

/**
 * Checks condition with a custom explanatory message.
 */
#define TEST_ASSERT_MSG(cond, msg) \
    do { \
        if (!(cond)) { \
            fprintf(stderr, "  [FAIL] %s:%d: %s (%s)\n", __FILE__, __LINE__, (msg), #cond); \
            g_ely_test_ctx.failed_assertions++; \
        } \
    } while (0)

/**
 * Checks equality between two integers/values.
 */
#define TEST_ASSERT_EQ(a, b) \
    do { \
        if ((a) != (b)) { \
            fprintf(stderr, "  [FAIL] %s:%d: Expected %s == %s\n", __FILE__, __LINE__, #a, #b); \
            g_ely_test_ctx.failed_assertions++; \
        } \
    } while (0)

/**
 * Checks floating-point equality with epsilon tolerance.
 */
#define TEST_ASSERT_FLOAT_EQ(a, b, eps) \
    do { \
        if (fabs((double)(a) - (double)(b)) > (double)(eps)) { \
            fprintf(stderr, "  [FAIL] %s:%d: Float mismatch: %g != %g (eps=%g)\n", \
                    __FILE__, __LINE__, (double)(a), (double)(b), (double)(eps)); \
            g_ely_test_ctx.failed_assertions++; \
        } \
    } while (0)

/**
 * Executes a test function and prints structured result.
 */
#define RUN_TEST(fn) \
    do { \
        printf("[ RUN      ] %s\n", #fn); \
        size_t before = g_ely_test_ctx.failed_assertions; \
        g_ely_test_ctx.tests_run++; \
        fn(); \
        if (g_ely_test_ctx.failed_assertions == before) { \
            printf("[       OK ] %s\n", #fn); \
        } else { \
            printf("[  FAILED  ] %s\n", #fn); \
            g_ely_test_ctx.tests_failed++; \
        } \
    } while (0)

#endif
