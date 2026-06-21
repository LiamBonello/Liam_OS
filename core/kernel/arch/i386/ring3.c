#include "ring3.h"
#include "gdt.h"
#include "tss.h"
#include "../../memory/pmm.h"
#include "../../memory/vmm.h"
#include "../../core/syscall.h"

#define RING3_KERNEL_SYSCALL_STACK_SIZE 4096

static ring3_info_t ring3_info;
static uint8_t ring3_kernel_syscall_stack[RING3_KERNEL_SYSCALL_STACK_SIZE] __attribute__((aligned(16)));

extern void ring3_enter_user_mode(uint32_t user_eip, uint32_t user_esp);
extern void ring3_resume_kernel_from_user(void);

static void ring3_zero_page(uint8_t* page)
{
    for (uint32_t i = 0; i < RING3_USER_PAGE_SIZE; i++)
    {
        page[i] = 0;
    }
}

static kernel_status_t ring3_prepare_pages(void)
{
    void* code_page = pmm_alloc_page();

    if (code_page == 0)
    {
        return KERNEL_ERROR_OUT_OF_MEMORY;
    }

    void* stack_page = pmm_alloc_page();

    if (stack_page == 0)
    {
        pmm_free_page(code_page);
        return KERNEL_ERROR_OUT_OF_MEMORY;
    }

    ring3_zero_page((uint8_t*)code_page);
    ring3_zero_page((uint8_t*)stack_page);

    if (!vmm_map_page(
            RING3_USER_CODE_VIRTUAL,
            (uint32_t)code_page,
            VMM_PAGE_WRITABLE | VMM_PAGE_USER))
    {
        pmm_free_page(stack_page);
        pmm_free_page(code_page);
        return KERNEL_ERROR_OUT_OF_MEMORY;
    }

    if (!vmm_map_page(
            RING3_USER_STACK_VIRTUAL,
            (uint32_t)stack_page,
            VMM_PAGE_WRITABLE | VMM_PAGE_USER))
    {
        vmm_unmap_page(RING3_USER_CODE_VIRTUAL);
        pmm_free_page(stack_page);
        pmm_free_page(code_page);
        return KERNEL_ERROR_OUT_OF_MEMORY;
    }

    if (!vmm_validate_mapping(RING3_USER_CODE_VIRTUAL, VMM_PAGE_USER | VMM_PAGE_WRITABLE))
    {
        vmm_unmap_page(RING3_USER_STACK_VIRTUAL);
        vmm_unmap_page(RING3_USER_CODE_VIRTUAL);
        pmm_free_page(stack_page);
        pmm_free_page(code_page);
        return KERNEL_ERROR_CORRUPTION_DETECTED;
    }

    if (!vmm_validate_mapping(RING3_USER_STACK_VIRTUAL, VMM_PAGE_USER | VMM_PAGE_WRITABLE))
    {
        vmm_unmap_page(RING3_USER_STACK_VIRTUAL);
        vmm_unmap_page(RING3_USER_CODE_VIRTUAL);
        pmm_free_page(stack_page);
        pmm_free_page(code_page);
        return KERNEL_ERROR_CORRUPTION_DETECTED;
    }

    ring3_info.code_physical = (uint32_t)code_page;
    ring3_info.stack_physical = (uint32_t)stack_page;
    ring3_info.loaded_image_size = 0;
    ring3_info.user_code_virtual = RING3_USER_CODE_VIRTUAL;
    ring3_info.user_stack_virtual = RING3_USER_STACK_VIRTUAL;
    ring3_info.user_stack_top = RING3_USER_STACK_VIRTUAL + RING3_USER_PAGE_SIZE - 16;

    return KERNEL_OK;
}

kernel_status_t ring3_initialize(void)
{
    if (ring3_info.initialized)
    {
        return ring3_info.last_status;
    }

    ring3_info.initialized = 1;
    ring3_info.ready = 0;
    ring3_info.in_progress = 0;
    ring3_info.completed = 0;
    ring3_info.runs = 0;
    ring3_info.successful_runs = 0;
    ring3_info.failed_runs = 0;
    ring3_info.syscall_count = 0;
    ring3_info.last_syscall_number = 0;
    ring3_info.load_attempts = 0;
    ring3_info.successful_loads = 0;
    ring3_info.failed_loads = 0;
    ring3_info.user_code_virtual = 0;
    ring3_info.user_stack_virtual = 0;
    ring3_info.user_stack_top = 0;
    ring3_info.code_physical = 0;
    ring3_info.stack_physical = 0;
    ring3_info.kernel_syscall_stack_top =
        (uint32_t)&ring3_kernel_syscall_stack[RING3_KERNEL_SYSCALL_STACK_SIZE];
    ring3_info.previous_tss_esp0 = 0;
    ring3_info.loaded_image_size = 0;
    ring3_info.last_status = ring3_prepare_pages();

    if (ring3_info.last_status == KERNEL_OK)
    {
        ring3_info.ready = 1;
    }

    return ring3_info.last_status;
}

kernel_status_t ring3_load_user_image(const uint8_t* image, uint32_t image_size, ring3_loaded_image_t* out_loaded)
{
    if (image == 0 || out_loaded == 0)
    {
        return KERNEL_ERROR_INVALID_ARGUMENT;
    }

    kernel_status_t status = ring3_initialize();

    if (status != KERNEL_OK)
    {
        return status;
    }

    ring3_info.load_attempts++;

    if (!ring3_info.ready || ring3_info.code_physical == 0 || ring3_info.stack_physical == 0)
    {
        ring3_info.failed_loads++;
        ring3_info.last_status = KERNEL_ERROR_INVALID_STATE;
        return ring3_info.last_status;
    }

    if (image_size == 0 || image_size > RING3_USER_PAGE_SIZE)
    {
        ring3_info.failed_loads++;
        ring3_info.last_status = KERNEL_ERROR_UNSUPPORTED;
        return ring3_info.last_status;
    }

    uint8_t* code_page = (uint8_t*)ring3_info.code_physical;
    ring3_zero_page(code_page);

    for (uint32_t i = 0; i < image_size; i++)
    {
        code_page[i] = image[i];
    }

    ring3_info.loaded_image_size = image_size;
    ring3_info.successful_loads++;
    ring3_info.last_status = KERNEL_OK;

    out_loaded->entry = ring3_info.user_code_virtual;
    out_loaded->user_stack_top = ring3_info.user_stack_top;
    out_loaded->image_size = image_size;

    return KERNEL_OK;
}

kernel_status_t ring3_run_user_image(uint32_t user_eip, uint32_t user_esp)
{
    kernel_status_t status = ring3_initialize();

    if (status != KERNEL_OK)
    {
        ring3_info.failed_runs++;
        ring3_info.last_status = status;
        return status;
    }

    if (!ring3_info.ready)
    {
        ring3_info.failed_runs++;
        ring3_info.last_status = KERNEL_ERROR_INVALID_STATE;
        return ring3_info.last_status;
    }

    if (user_eip == 0 || user_esp == 0)
    {
        ring3_info.failed_runs++;
        ring3_info.last_status = KERNEL_ERROR_INVALID_ARGUMENT;
        return ring3_info.last_status;
    }

    ring3_info.runs++;
    ring3_info.in_progress = 1;
    ring3_info.completed = 0;
    ring3_info.last_syscall_number = 0;
    ring3_info.previous_tss_esp0 = tss_get_info()->esp0;

    tss_set_kernel_stack(ring3_info.kernel_syscall_stack_top);
    ring3_enter_user_mode(user_eip, user_esp);

    if (!ring3_info.completed)
    {
        ring3_info.in_progress = 0;
        ring3_info.failed_runs++;
        ring3_info.last_status = KERNEL_ERROR_INVALID_STATE;
        return ring3_info.last_status;
    }

    ring3_info.successful_runs++;
    ring3_info.last_status = KERNEL_OK;
    return KERNEL_OK;
}

const ring3_info_t* ring3_get_info(void)
{
    return &ring3_info;
}

uint32_t ring3_syscall_entry(uint32_t syscall_number, uint32_t arg1, uint32_t arg2, uint32_t arg3)
{
    ring3_info.syscall_count++;
    ring3_info.last_syscall_number = syscall_number;

    uint32_t result = syscall_dispatch(syscall_number, arg1, arg2, arg3);

    if (ring3_info.in_progress && syscall_number == LIAM_SYSCALL_EXIT && result == KERNEL_OK)
    {
        ring3_info.completed = 1;
        ring3_info.in_progress = 0;
        tss_set_kernel_stack(ring3_info.previous_tss_esp0);
        ring3_resume_kernel_from_user();
    }

    if (syscall_number != LIAM_SYSCALL_EXIT)
    {
        return result;
    }

    ring3_info.in_progress = 0;
    ring3_info.failed_runs++;
    ring3_info.last_status = KERNEL_ERROR_UNSUPPORTED;
    tss_set_kernel_stack(ring3_info.previous_tss_esp0);
    ring3_resume_kernel_from_user();

    return result;
}