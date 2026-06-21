#ifndef LIAM_OS_LOG_H
#define LIAM_OS_LOG_H

#include "types.h"


void log_info(const char* message);
void log_success(const char* message);
void log_warning(const char* message);
void log_error(const char* message);
void log_debug(const char* message);

void log_info_hex_u32(const char* label, uint32_t value);
void log_info_u32(const char* label, uint32_t value);

void log_debug_hex_u32(const char* label, uint32_t value);
void log_debug_u32(const char* label, uint32_t value);

#endif