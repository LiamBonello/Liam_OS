#ifndef LIAM_OS_X86_64_CONSOLE_H
#define LIAM_OS_X86_64_CONSOLE_H

#include "types.h"

void x86_64_console_init(void);
void x86_64_console_write_at(const char *message, usize row, usize column);
void x86_64_console_write_u32(usize row, usize column, const char *label, u32 value);
void x86_64_console_write_hex32(usize row, usize column, const char *label, u32 value);
void x86_64_console_write_hex64(usize row, usize column, const char *label, u64 value);

void x86_64_serial_init(void);
void x86_64_serial_write(const char *message);
void x86_64_serial_write_line(const char *message);
void x86_64_serial_write_u32(const char *label, u32 value);
void x86_64_serial_write_hex32(const char *label, u32 value);
void x86_64_serial_write_hex64(const char *label, u64 value);

#endif
