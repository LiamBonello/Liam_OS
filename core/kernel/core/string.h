#ifndef LIAM_OS_STRING_H
#define LIAM_OS_STRING_H

#include "types.h"


uint32_t string_length(const char* value);
int string_equals(const char* left, const char* right);
int string_starts_with(const char* value, const char* prefix);
void string_copy(char* destination, const char* source, uint32_t max_length);
void string_clear(char* buffer, uint32_t length);

void string_trim_left(const char** value);
uint32_t string_copy_until_space(char* destination, const char* source, uint32_t max_length);
uint32_t string_skip_spaces(const char* value, uint32_t start_index);
int string_to_uint32(const char* value, uint32_t* result);
int string_to_uint32_auto(const char* value, uint32_t* result);

#endif