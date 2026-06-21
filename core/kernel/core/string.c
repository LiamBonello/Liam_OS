#include "string.h"

uint32_t string_length(const char* value) {
    uint32_t length = 0;

    while (value[length] != '\0') {
        length++;
    }

    return length;
}

int string_equals(const char* left, const char* right) {
    uint32_t index = 0;

    while (left[index] != '\0' && right[index] != '\0') {
        if (left[index] != right[index]) {
            return 0;
        }

        index++;
    }

    return left[index] == '\0' && right[index] == '\0';
}

int string_starts_with(const char* value, const char* prefix) {
    uint32_t index = 0;

    while (prefix[index] != '\0') {
        if (value[index] != prefix[index]) {
            return 0;
        }

        index++;
    }

    return 1;
}

void string_copy(char* destination, const char* source, uint32_t max_length) {
    if (max_length == 0) {
        return;
    }

    uint32_t index = 0;

    while (index < max_length - 1 && source[index] != '\0') {
        destination[index] = source[index];
        index++;
    }

    destination[index] = '\0';
}

void string_clear(char* buffer, uint32_t length) {
    for (uint32_t i = 0; i < length; i++) {
        buffer[i] = '\0';
    }
}

void string_trim_left(const char** value) {
    while (**value == ' ' || **value == '\t') {
        (*value)++;
    }
}

uint32_t string_copy_until_space(char* destination, const char* source, uint32_t max_length) {
    if (max_length == 0) {
        return 0;
    }

    uint32_t index = 0;

    while (
        index < max_length - 1 &&
        source[index] != '\0' &&
        source[index] != ' ' &&
        source[index] != '\t'
    ) {
        destination[index] = source[index];
        index++;
    }

    destination[index] = '\0';
    return index;
}

uint32_t string_skip_spaces(const char* value, uint32_t start_index) {
    uint32_t index = start_index;

    while (value[index] == ' ' || value[index] == '\t') {
        index++;
    }

    return index;
}

static int string_hex_digit_value(char character, uint32_t* value) {
    if (character >= '0' && character <= '9') {
        *value = (uint32_t)(character - '0');
        return 1;
    }

    if (character >= 'a' && character <= 'f') {
        *value = (uint32_t)(character - 'a') + 10;
        return 1;
    }

    if (character >= 'A' && character <= 'F') {
        *value = (uint32_t)(character - 'A') + 10;
        return 1;
    }

    return 0;
}

int string_to_uint32(const char* value, uint32_t* result) {
    uint32_t index = 0;
    uint32_t parsed = 0;
    int found_digit = 0;

    while (value[index] == ' ' || value[index] == '\t') {
        index++;
    }

    while (value[index] >= '0' && value[index] <= '9') {
        found_digit = 1;
        parsed = (parsed * 10) + (uint32_t)(value[index] - '0');
        index++;
    }

    if (!found_digit) {
        return 0;
    }

    while (value[index] == ' ' || value[index] == '\t') {
        index++;
    }

    if (value[index] != '\0') {
        return 0;
    }

    *result = parsed;
    return 1;
}

int string_to_uint32_auto(const char* value, uint32_t* result) {
    uint32_t index = 0;
    uint32_t parsed = 0;
    int found_digit = 0;

    while (value[index] == ' ' || value[index] == '\t') {
        index++;
    }

    if (
        value[index] == '0' &&
        (value[index + 1] == 'x' || value[index + 1] == 'X')
    ) {
        index += 2;

        while (value[index] != '\0') {
            uint32_t digit = 0;

            if (!string_hex_digit_value(value[index], &digit)) {
                break;
            }

            found_digit = 1;
            parsed = (parsed << 4) | digit;
            index++;
        }

        while (value[index] == ' ' || value[index] == '\t') {
            index++;
        }

        if (!found_digit || value[index] != '\0') {
            return 0;
        }

        *result = parsed;
        return 1;
    }

    return string_to_uint32(value, result);
}