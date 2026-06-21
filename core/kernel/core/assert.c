#include "assert.h"
#include "log.h"
#include "panic.h"
#include "../drivers/vga.h"

static kernel_assert_stats_t assert_stats;

void kernel_assert_initialize(void)
{
    assert_stats.initialized = 1;
    assert_stats.passed_assertions = 0;
    assert_stats.failed_assertions = 0;
    assert_stats.last_failure_line = 0;
    assert_stats.last_failure_file = 0;
    assert_stats.last_failure_expression = 0;
    assert_stats.last_failure_message = 0;

    log_success("Kernel assertions initialized");
}

void kernel_assert_record_pass(void)
{
    if (!assert_stats.initialized)
    {
        return;
    }

    assert_stats.passed_assertions++;
}

void kernel_assert_fail(
    const char* expression,
    const char* file,
    uint32_t line,
    const char* message
)
{
    assert_stats.failed_assertions++;
    assert_stats.last_failure_line = line;
    assert_stats.last_failure_file = file;
    assert_stats.last_failure_expression = expression;
    assert_stats.last_failure_message = message;

    vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
    vga_write_line("");
    vga_write_line("KERNEL ASSERTION FAILED");

    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);

    vga_write("  Expression: ");
    vga_write_line(expression ? expression : "(null)");

    vga_write("  File: ");
    vga_write_line(file ? file : "(null)");

    vga_write("  Line: ");
    vga_write_u32(line);
    vga_write_line("");

    if (message != 0)
    {
        vga_write("  Message: ");
        vga_write_line(message);
    }

    kernel_panic("Kernel assertion failed");
}

const kernel_assert_stats_t* kernel_assert_get_stats(void)
{
    return &assert_stats;
}

void kernel_assert_print_stats(void)
{
    const kernel_assert_stats_t* stats = kernel_assert_get_stats();

    vga_write_line("Kernel assertion statistics:");

    vga_write("  Initialized: ");
    vga_write_line(stats->initialized ? "yes" : "no");

    vga_write("  Passed assertions: ");
    vga_write_u32(stats->passed_assertions);
    vga_write_line("");

    vga_write("  Failed assertions: ");
    vga_write_u32(stats->failed_assertions);
    vga_write_line("");

    vga_write("  Last failure file: ");
    vga_write_line(stats->last_failure_file ? stats->last_failure_file : "none");

    vga_write("  Last failure line: ");
    vga_write_u32(stats->last_failure_line);
    vga_write_line("");

    vga_write("  Last failure expression: ");
    vga_write_line(
        stats->last_failure_expression ?
        stats->last_failure_expression :
        "none"
    );

    vga_write("  Last failure message: ");
    vga_write_line(
        stats->last_failure_message ?
        stats->last_failure_message :
        "none"
    );
}