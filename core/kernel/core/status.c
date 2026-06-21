#include "status.h"

int kernel_status_is_success(kernel_status_t status)
{
    return status == KERNEL_OK;
}

int kernel_status_is_error(kernel_status_t status)
{
    return status < KERNEL_OK;
}

const char* kernel_status_to_string(kernel_status_t status)
{
    switch (status)
    {
    case KERNEL_OK:
        return "ok";
    case KERNEL_ERROR_UNKNOWN:
        return "unknown error";
    case KERNEL_ERROR_INVALID_ARGUMENT:
        return "invalid argument";
    case KERNEL_ERROR_NOT_FOUND:
        return "not found";
    case KERNEL_ERROR_OUT_OF_MEMORY:
        return "out of memory";
    case KERNEL_ERROR_ALREADY_EXISTS:
        return "already exists";
    case KERNEL_ERROR_NOT_INITIALIZED:
        return "not initialized";
    case KERNEL_ERROR_BUSY:
        return "busy";
    case KERNEL_ERROR_PERMISSION_DENIED:
        return "permission denied";
    case KERNEL_ERROR_UNSUPPORTED:
        return "unsupported";
    case KERNEL_ERROR_INVALID_STATE:
        return "invalid state";
    case KERNEL_ERROR_CORRUPTION_DETECTED:
        return "corruption detected";
    case KERNEL_ERROR_TIMEOUT:
        return "timeout";
    default:
        return "unrecognized status";
    }
}