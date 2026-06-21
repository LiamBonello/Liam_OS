#include "user_program.h"
#include "process.h"
#include "string.h"
#include "log.h"
#include "../arch/i386/ring3.h"

typedef struct
{
    const char* name;
    const char* description;
} builtin_user_program_t;

static const builtin_user_program_t builtin_programs[] = {
    {
        "hello",
        "Prints a message from ring 3 through syscall_write"
    }
};

static user_program_info_t user_program_info;

void user_program_initialize(void)
{
    user_program_info.initialized = 1;
    user_program_info.available_programs =
        sizeof(builtin_programs) / sizeof(builtin_programs[0]);
    user_program_info.runs = 0;
    user_program_info.successful_runs = 0;
    user_program_info.failed_runs = 0;
    user_program_info.last_status = KERNEL_OK;
    user_program_info.last_program_name[0] = '\0';
}

uint32_t user_program_count(void)
{
    return sizeof(builtin_programs) / sizeof(builtin_programs[0]);
}

const char* user_program_name_at(uint32_t index)
{
    if (index >= user_program_count())
    {
        return 0;
    }

    return builtin_programs[index].name;
}

static int user_program_exists(const char* name)
{
    for (uint32_t i = 0; i < user_program_count(); i++)
    {
        if (string_equals(name, builtin_programs[i].name))
        {
            return 1;
        }
    }

    return 0;
}

kernel_status_t user_program_run(const char* name)
{
    if (name == 0 || string_equals(name, ""))
    {
        return KERNEL_ERROR_INVALID_ARGUMENT;
    }

    user_program_info.runs++;
    string_copy(user_program_info.last_program_name, name, 32);

    if (!user_program_exists(name))
    {
        user_program_info.failed_runs++;
        user_program_info.last_status = KERNEL_ERROR_NOT_FOUND;
        return user_program_info.last_status;
    }

    kernel_status_t status = ring3_initialize();

    if (status != KERNEL_OK)
    {
        user_program_info.failed_runs++;
        user_program_info.last_status = status;
        return status;
    }

    const ring3_info_t* ring3 = ring3_get_info();

    process_t* process = 0;

    status = process_create(
        name,
        ring3->user_code_virtual,
        ring3->user_stack_top,
        &process
    );

    if (status != KERNEL_OK)
    {
        user_program_info.failed_runs++;
        user_program_info.last_status = status;
        return status;
    }

    status = process_run(process);

    if (status == KERNEL_OK)
    {
        user_program_info.successful_runs++;
    }
    else
    {
        user_program_info.failed_runs++;
    }

    user_program_info.last_status = status;
    return status;
}

const user_program_info_t* user_program_get_info(void)
{
    return &user_program_info;
}