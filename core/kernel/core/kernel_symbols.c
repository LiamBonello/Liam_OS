#include "kernel_symbols.h"

uint32_t kernel_get_start_address(void) {
    return (uint32_t)&__kernel_start;
}

uint32_t kernel_get_end_address(void) {
    return (uint32_t)&__kernel_end;
}