#ifndef LIAM_OS_STATUS_H
#define LIAM_OS_STATUS_H

typedef int kernel_status_t;

#define KERNEL_OK                         0
#define KERNEL_ERROR_UNKNOWN             -1
#define KERNEL_ERROR_INVALID_ARGUMENT    -2
#define KERNEL_ERROR_NOT_FOUND           -3
#define KERNEL_ERROR_OUT_OF_MEMORY       -4
#define KERNEL_ERROR_ALREADY_EXISTS      -5
#define KERNEL_ERROR_NOT_INITIALIZED     -6
#define KERNEL_ERROR_BUSY                -7
#define KERNEL_ERROR_PERMISSION_DENIED   -8
#define KERNEL_ERROR_UNSUPPORTED         -9
#define KERNEL_ERROR_INVALID_STATE       -10
#define KERNEL_ERROR_CORRUPTION_DETECTED -11
#define KERNEL_ERROR_TIMEOUT             -12

int kernel_status_is_success(kernel_status_t status);
int kernel_status_is_error(kernel_status_t status);
const char* kernel_status_to_string(kernel_status_t status);

#endif