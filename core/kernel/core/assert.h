#ifndef LIAM_OS_ASSERT_H
#define LIAM_OS_ASSERT_H

#include "types.h"


typedef struct
{
    uint8_t initialized;
    uint32_t passed_assertions;
    uint32_t failed_assertions;
    uint32_t last_failure_line;
    const char* last_failure_file;
    const char* last_failure_expression;
    const char* last_failure_message;
} kernel_assert_stats_t;

void kernel_assert_initialize(void);

void kernel_assert_record_pass(void);

void kernel_assert_fail(
    const char* expression,
    const char* file,
    uint32_t line,
    const char* message
);

const kernel_assert_stats_t* kernel_assert_get_stats(void);
void kernel_assert_print_stats(void);

#define kernel_assert(expression) \
    do { \
        if (!(expression)) { \
            kernel_assert_fail(#expression, __FILE__, __LINE__, 0); \
        } else { \
            kernel_assert_record_pass(); \
        } \
    } while (0)

#define kernel_assert_message(expression, message) \
    do { \
        if (!(expression)) { \
            kernel_assert_fail(#expression, __FILE__, __LINE__, message); \
        } else { \
            kernel_assert_record_pass(); \
        } \
    } while (0)

#endif