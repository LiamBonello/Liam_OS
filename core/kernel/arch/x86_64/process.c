#include "process.h"

#include "console.h"
#include "gdt.h"
#include "paging_plan.h"
#include "pmm.h"
#include "syscall_dispatch.h"
#include "user_context.h"
#include "userland.h"

#define X86_64_PROCESS_INVALID_PID 0U
#define X86_64_PROCESS_FIRST_PID 1U
#define X86_64_PROCESS_USER_RFLAGS 0x0000000000000202ULL
#define X86_64_PROCESS_USER_STACK_ALIGNMENT 16ULL
#define X86_64_PROCESS_PAGE_PRESENT 0x001ULL
#define X86_64_PROCESS_PAGE_WRITABLE 0x002ULL
#define X86_64_PROCESS_PAGE_USER 0x004ULL
#define X86_64_PROCESS_USER_TABLE_FLAGS \
    (X86_64_PROCESS_PAGE_PRESENT | X86_64_PROCESS_PAGE_WRITABLE | X86_64_PROCESS_PAGE_USER)
#define X86_64_PROCESS_USER_CODE_FLAGS \
    (X86_64_PROCESS_PAGE_PRESENT | X86_64_PROCESS_PAGE_USER)
#define X86_64_PROCESS_USER_STACK_FLAGS \
    (X86_64_PROCESS_PAGE_PRESENT | X86_64_PROCESS_PAGE_WRITABLE | X86_64_PROCESS_PAGE_USER)

__attribute__((naked)) static void x86_64_process_enter_stack(
    u64 stack_top __attribute__((unused)),
    x86_64_process_entry_t entry __attribute__((unused)),
    void *arg __attribute__((unused)))
{
    __asm__ volatile (
        "push %rbx\n"
        "mov %rsp, %rbx\n"
        "mov %rdi, %rsp\n"
        "and $-16, %rsp\n"
        "mov %rdx, %rdi\n"
        "call *%rsi\n"
        "mov %rbx, %rsp\n"
        "pop %rbx\n"
        "ret\n"
    );
}

struct x86_64_process_paging_context {
    u32 initialized;
    u32 identity_pml4_index;
    u32 direct_map_pml4_index;
    u32 kernel_pml4_index;
    u64 source_pml4_table;
    u64 source_identity_pdpt_table;
    u64 source_identity_pd_table;
};

struct x86_64_child_status {
    u32 parent_pid;
    u32 child_pid;
    u32 exit_code;
};

static struct x86_64_process process_table[X86_64_PROCESS_MAX_PROCESSES];
static struct x86_64_process_smoke_state process_state;
static struct x86_64_process_paging_context process_paging_context;
static u32 next_pid = X86_64_PROCESS_FIRST_PID;
static u64 next_address_space_id = 1ULL;
static struct x86_64_child_status child_status_queue[X86_64_PROCESS_CHILD_STATUS_QUEUE];
static u32 child_status_count;
static u32 worker_a_runs;
static u32 worker_b_runs;
static u64 worker_a_stack_sample;
static u64 worker_b_stack_sample;

static void clear_bytes(void *ptr, usize size)
{
    u8 *bytes = (u8 *)ptr;
    for (usize i = 0; i < size; ++i) {
        bytes[i] = 0U;
    }
}

static void copy_bytes(void *dest, const void *src, usize size)
{
    u8 *out = (u8 *)dest;
    const u8 *in = (const u8 *)src;
    for (usize i = 0; i < size; ++i) {
        out[i] = in[i];
    }
}

static void clear_page(u64 page)
{
    clear_bytes((void *)page, X86_64_USER_PAGE_BYTES);
}

static u64 read_rsp(void)
{
    u64 value;
    __asm__ volatile ("mov %%rsp, %0" : "=r"(value));
    return value;
}

static u32 is_aligned_u64(u64 value, u64 alignment)
{
    return ((value & (alignment - 1ULL)) == 0ULL) ? 1U : 0U;
}

static u32 pml4_index_for(u64 address)
{
    return (u32)((address >> 39) & 0x1FFULL);
}

static u32 pdpt_index_for(u64 address)
{
    return (u32)((address >> 30) & 0x1FFULL);
}

static u32 pd_index_for(u64 address)
{
    return (u32)((address >> 21) & 0x1FFULL);
}

static u32 pt_index_for(u64 address)
{
    return (u32)((address >> 12) & 0x1FFULL);
}

static u32 entry_present(u64 entry)
{
    return ((entry & X86_64_PROCESS_PAGE_PRESENT) != 0ULL) ? 1U : 0U;
}

static u32 entry_user_accessible(u64 entry)
{
    return ((entry & X86_64_PROCESS_PAGE_USER) != 0ULL) ? 1U : 0U;
}

static u32 entry_points_to(u64 entry, u64 page)
{
    return ((entry & ~(X86_64_USER_PAGE_BYTES - 1ULL)) == page) ? 1U : 0U;
}

static u32 is_inside_stack(u64 value, u64 stack_base, u64 stack_top)
{
    return ((value >= stack_base) && (value < stack_top)) ? 1U : 0U;
}

void x86_64_process_set_paging_context(const struct x86_64_paging_builder_state *paging_builder)
{
    clear_bytes(&process_paging_context, sizeof(process_paging_context));

    if (paging_builder == (const struct x86_64_paging_builder_state *)0 ||
        paging_builder->builder_ok == 0U ||
        paging_builder->pml4_table == 0ULL ||
        paging_builder->identity_pdpt_table == 0ULL ||
        paging_builder->identity_pd_table == 0ULL) {
        return;
    }

    process_paging_context.identity_pml4_index = pml4_index_for(0ULL);
    process_paging_context.direct_map_pml4_index = pml4_index_for(X86_64_DIRECT_MAP_BASE);
    process_paging_context.kernel_pml4_index = pml4_index_for(X86_64_KERNEL_VIRTUAL_BASE);
    process_paging_context.source_pml4_table = paging_builder->pml4_table;
    process_paging_context.source_identity_pdpt_table = paging_builder->identity_pdpt_table;
    process_paging_context.source_identity_pd_table = paging_builder->identity_pd_table;
    process_paging_context.initialized = 1U;
}

static u32 validate_kernel_mappings(const struct x86_64_process *process)
{
    if (process == (const struct x86_64_process *)0 ||
        process_paging_context.initialized == 0U ||
        process->cr3 == 0ULL ||
        process->user_pdpt_page == 0ULL ||
        process->user_code_pd_page == 0ULL) {
        return 0U;
    }

    const u64 *process_pml4 = (const u64 *)process->cr3;
    const u64 *source_pml4 = (const u64 *)process_paging_context.source_pml4_table;
    const u64 *process_pdpt = (const u64 *)process->user_pdpt_page;
    const u64 *source_pdpt = (const u64 *)process_paging_context.source_identity_pdpt_table;
    const u64 *process_code_pd = (const u64 *)process->user_code_pd_page;
    const u64 *source_identity_pd = (const u64 *)process_paging_context.source_identity_pd_table;
    u32 identity_index = process_paging_context.identity_pml4_index;
    u32 direct_index = process_paging_context.direct_map_pml4_index;
    u32 kernel_index = process_paging_context.kernel_pml4_index;
    u32 code_pdpt_index = pdpt_index_for(process->user_code_virtual);
    u32 code_pd_index = pd_index_for(process->user_code_virtual);

    return ((entry_present(source_pml4[direct_index]) != 0U) &&
            (entry_present(source_pml4[kernel_index]) != 0U) &&
            (process_pml4[direct_index] == source_pml4[direct_index]) &&
            (process_pml4[kernel_index] == source_pml4[kernel_index]) &&
            (entry_points_to(process_pml4[identity_index], process->user_pdpt_page) != 0U) &&
            (entry_user_accessible(process_pml4[identity_index]) != 0U) &&
            (entry_points_to(process_pdpt[code_pdpt_index], process->user_code_pd_page) != 0U) &&
            (entry_user_accessible(process_pdpt[code_pdpt_index]) != 0U) &&
            (entry_present(source_pdpt[code_pdpt_index]) != 0U) &&
            (entry_present(source_identity_pd[0]) != 0U) &&
            (process_code_pd[0] == source_identity_pd[0]) &&
            (process_code_pd[code_pd_index] != source_identity_pd[code_pd_index])) ? 1U : 0U;
}

static u32 clone_kernel_mappings(struct x86_64_process *process)
{
    if (process == (struct x86_64_process *)0 || process_paging_context.initialized == 0U) {
        return 0U;
    }

    copy_bytes((void *)process->cr3,
               (const void *)process_paging_context.source_pml4_table,
               X86_64_USER_PAGE_BYTES);
    copy_bytes((void *)process->user_pdpt_page,
               (const void *)process_paging_context.source_identity_pdpt_table,
               X86_64_USER_PAGE_BYTES);
    copy_bytes((void *)process->user_code_pd_page,
               (const void *)process_paging_context.source_identity_pd_table,
               X86_64_USER_PAGE_BYTES);

    u64 *pml4 = (u64 *)process->cr3;
    pml4[process_paging_context.identity_pml4_index] =
        process->user_pdpt_page | X86_64_PROCESS_USER_TABLE_FLAGS;

    return 1U;
}

static void copy_limited(char *dest, u32 dest_size, const char *src)
{
    u32 i = 0U;
    if (dest_size == 0U) {
        return;
    }

    if (src != (const char *)0) {
        while (i + 1U < dest_size && src[i] != '\0') {
            dest[i] = src[i];
            i += 1U;
        }
    }

    dest[i] = '\0';
}

static void copy_name(char *dest, const char *src)
{
    copy_limited(dest, X86_64_PROCESS_NAME_LEN, src);
}

static void copy_image_path(char *dest, const char *src)
{
    copy_limited(dest, X86_64_PROCESS_IMAGE_PATH_LEN, src);
}

static u64 append_char(char *buffer, u64 size, u64 offset, char value)
{
    if (offset + 1ULL < size) {
        buffer[offset] = value;
    }

    return offset + 1ULL;
}

static u64 append_string(char *buffer, u64 size, u64 offset, const char *value)
{
    if (value == (const char *)0) {
        return offset;
    }

    for (u64 i = 0ULL; value[i] != '\0'; ++i) {
        offset = append_char(buffer, size, offset, value[i]);
    }

    return offset;
}

static u64 append_u32_decimal(char *buffer, u64 size, u64 offset, u32 value)
{
    char digits[10];
    u32 count = 0U;

    if (value == 0U) {
        return append_char(buffer, size, offset, '0');
    }

    while (value != 0U && count < (u32)sizeof(digits)) {
        digits[count++] = (char)('0' + (value % 10U));
        value /= 10U;
    }

    while (count > 0U) {
        --count;
        offset = append_char(buffer, size, offset, digits[count]);
    }

    return offset;
}

static const char *process_state_name(u32 state)
{
    switch (state) {
    case X86_64_PROCESS_READY:
        return "ready";
    case X86_64_PROCESS_RUNNING:
        return "running";
    case X86_64_PROCESS_EXITED:
        return "exited";
    case X86_64_PROCESS_FAILED:
        return "failed";
    default:
        return "unused";
    }
}

static const char *process_mode_name(u32 mode)
{
    switch (mode) {
    case X86_64_PROCESS_MODE_KERNEL:
        return "kernel";
    case X86_64_PROCESS_MODE_USER:
        return "user";
    default:
        return "none";
    }
}

static struct x86_64_process *find_unused_process(void)
{
    for (u32 i = 0; i < X86_64_PROCESS_MAX_PROCESSES; ++i) {
        if (process_table[i].state == X86_64_PROCESS_UNUSED) {
            return &process_table[i];
        }
    }

    return (struct x86_64_process *)0;
}

static struct x86_64_process *find_process_by_pid(u32 pid)
{
    for (u32 i = 0; i < X86_64_PROCESS_MAX_PROCESSES; ++i) {
        if (process_table[i].state != X86_64_PROCESS_UNUSED && process_table[i].pid == pid) {
            return &process_table[i];
        }
    }

    return (struct x86_64_process *)0;
}

static struct x86_64_process *find_next_ready_process(void)
{
    for (u32 i = 0; i < X86_64_PROCESS_MAX_PROCESSES; ++i) {
        if (process_table[i].state == X86_64_PROCESS_READY &&
            process_table[i].mode == X86_64_PROCESS_MODE_KERNEL &&
            process_table[i].entry != (x86_64_process_entry_t)0) {
            return &process_table[i];
        }
    }

    return (struct x86_64_process *)0;
}

static struct x86_64_process *find_next_ready_user_process(void)
{
    for (u32 i = 0; i < X86_64_PROCESS_MAX_PROCESSES; ++i) {
        if (process_table[i].state == X86_64_PROCESS_READY &&
            process_table[i].mode == X86_64_PROCESS_MODE_USER &&
            process_table[i].user_entry != 0ULL) {
            return &process_table[i];
        }
    }

    return (struct x86_64_process *)0;
}

static u32 count_processes_with_state(u32 state)
{
    u32 count = 0U;
    for (u32 i = 0; i < X86_64_PROCESS_MAX_PROCESSES; ++i) {
        if (process_table[i].state == state) {
            count += 1U;
        }
    }

    return count;
}

static void release_address_space(struct x86_64_process *process)
{
    if (process == (struct x86_64_process *)0) {
        return;
    }

    if (process->cr3 != 0ULL) {
        (void)x86_64_pmm_free_page(process->cr3);
    }
    if (process->user_pdpt_page != 0ULL) {
        (void)x86_64_pmm_free_page(process->user_pdpt_page);
    }
    if (process->user_code_pd_page != 0ULL) {
        (void)x86_64_pmm_free_page(process->user_code_pd_page);
    }
    if (process->user_stack_pd_page != 0ULL) {
        (void)x86_64_pmm_free_page(process->user_stack_pd_page);
    }
    if (process->user_code_pt_page != 0ULL) {
        (void)x86_64_pmm_free_page(process->user_code_pt_page);
    }
    if (process->user_stack_pt_page != 0ULL) {
        (void)x86_64_pmm_free_page(process->user_stack_pt_page);
    }
    if (process->user_code_page != 0ULL) {
        (void)x86_64_pmm_free_page(process->user_code_page);
    }
    if (process->user_stack_page != 0ULL) {
        (void)x86_64_pmm_free_page(process->user_stack_page);
    }

    process->address_space_id = 0ULL;
    process->cr3 = 0ULL;
    process->user_pdpt_page = 0ULL;
    process->user_code_pd_page = 0ULL;
    process->user_stack_pd_page = 0ULL;
    process->user_code_pt_page = 0ULL;
    process->user_stack_pt_page = 0ULL;
    process->user_code_page = 0ULL;
    process->user_stack_page = 0ULL;
    process->user_code_virtual = 0ULL;
    process->user_stack_virtual = 0ULL;
    process->address_space_owned = 0U;
    process->user_page_tables_ready = 0U;
    process->kernel_mappings_ready = 0U;
}

static u32 allocate_kernel_stack(struct x86_64_process *process)
{
    if (process == (struct x86_64_process *)0) {
        return 0U;
    }

    u64 page = x86_64_pmm_alloc_page();
    if (page == X86_64_PMM_INVALID_PAGE) {
        return 0U;
    }

    clear_page(page);
    process->kernel_stack_page = page;
    process->kernel_stack_base = X86_64_DIRECT_MAP_BASE + page;
    process->kernel_stack_top = process->kernel_stack_base + X86_64_PROCESS_KERNEL_STACK_BYTES;

    return ((is_aligned_u64(process->kernel_stack_page, X86_64_PROCESS_KERNEL_STACK_BYTES) != 0U) &&
            (is_aligned_u64(process->kernel_stack_base, X86_64_PROCESS_KERNEL_STACK_BYTES) != 0U) &&
            (is_aligned_u64(process->kernel_stack_top, 16ULL) != 0U)) ? 1U : 0U;
}

static void release_kernel_stack(struct x86_64_process *process)
{
    if (process == (struct x86_64_process *)0) {
        return;
    }

    if (process->kernel_stack_page != 0ULL) {
        (void)x86_64_pmm_free_page(process->kernel_stack_page);
    }

    process->kernel_stack_page = 0ULL;
    process->kernel_stack_base = 0ULL;
    process->kernel_stack_top = 0ULL;
}

static void release_process_resources(struct x86_64_process *process)
{
    release_address_space(process);
    release_kernel_stack(process);
}

static u32 validate_user_page_tables(const struct x86_64_process *process)
{
    const u64 *pml4 = (const u64 *)process->cr3;
    const u64 *pdpt = (const u64 *)process->user_pdpt_page;
    const u64 *code_pd = (const u64 *)process->user_code_pd_page;
    const u64 *stack_pd = (const u64 *)process->user_stack_pd_page;
    const u64 *code_pt = (const u64 *)process->user_code_pt_page;
    const u64 *stack_pt = (const u64 *)process->user_stack_pt_page;
    u32 code_pml4_index = pml4_index_for(process->user_code_virtual);
    u32 code_pdpt_index = pdpt_index_for(process->user_code_virtual);
    u32 code_pd_index = pd_index_for(process->user_code_virtual);
    u32 code_pt_index = pt_index_for(process->user_code_virtual);
    u32 stack_pml4_index = pml4_index_for(process->user_stack_virtual);
    u32 stack_pdpt_index = pdpt_index_for(process->user_stack_virtual);
    u32 stack_pd_index = pd_index_for(process->user_stack_virtual);
    u32 stack_pt_index = pt_index_for(process->user_stack_virtual);

    return ((code_pml4_index == stack_pml4_index) &&
            (entry_present(pml4[code_pml4_index]) != 0U) &&
            (entry_user_accessible(pml4[code_pml4_index]) != 0U) &&
            (entry_points_to(pml4[code_pml4_index], process->user_pdpt_page) != 0U) &&
            (entry_present(pdpt[code_pdpt_index]) != 0U) &&
            (entry_user_accessible(pdpt[code_pdpt_index]) != 0U) &&
            (entry_points_to(pdpt[code_pdpt_index], process->user_code_pd_page) != 0U) &&
            (entry_present(pdpt[stack_pdpt_index]) != 0U) &&
            (entry_user_accessible(pdpt[stack_pdpt_index]) != 0U) &&
            (entry_points_to(pdpt[stack_pdpt_index], process->user_stack_pd_page) != 0U) &&
            (entry_present(code_pd[code_pd_index]) != 0U) &&
            (entry_user_accessible(code_pd[code_pd_index]) != 0U) &&
            (entry_points_to(code_pd[code_pd_index], process->user_code_pt_page) != 0U) &&
            (entry_present(stack_pd[stack_pd_index]) != 0U) &&
            (entry_user_accessible(stack_pd[stack_pd_index]) != 0U) &&
            (entry_points_to(stack_pd[stack_pd_index], process->user_stack_pt_page) != 0U) &&
            (entry_present(code_pt[code_pt_index]) != 0U) &&
            (entry_user_accessible(code_pt[code_pt_index]) != 0U) &&
            (entry_points_to(code_pt[code_pt_index], process->user_code_page) != 0U) &&
            (entry_present(stack_pt[stack_pt_index]) != 0U) &&
            (entry_user_accessible(stack_pt[stack_pt_index]) != 0U) &&
            (entry_points_to(stack_pt[stack_pt_index], process->user_stack_page) != 0U) &&
            (validate_kernel_mappings(process) != 0U)) ? 1U : 0U;
}

static void build_user_page_tables(struct x86_64_process *process)
{
    u64 *pml4 = (u64 *)process->cr3;
    u64 *pdpt = (u64 *)process->user_pdpt_page;
    u64 *code_pd = (u64 *)process->user_code_pd_page;
    u64 *stack_pd = (u64 *)process->user_stack_pd_page;
    u64 *code_pt = (u64 *)process->user_code_pt_page;
    u64 *stack_pt = (u64 *)process->user_stack_pt_page;
    u32 code_pml4_index = pml4_index_for(process->user_code_virtual);
    u32 code_pdpt_index = pdpt_index_for(process->user_code_virtual);
    u32 code_pd_index = pd_index_for(process->user_code_virtual);
    u32 code_pt_index = pt_index_for(process->user_code_virtual);
    u32 stack_pdpt_index = pdpt_index_for(process->user_stack_virtual);
    u32 stack_pd_index = pd_index_for(process->user_stack_virtual);
    u32 stack_pt_index = pt_index_for(process->user_stack_virtual);

    pml4[code_pml4_index] = process->user_pdpt_page | X86_64_PROCESS_USER_TABLE_FLAGS;
    pdpt[code_pdpt_index] = process->user_code_pd_page | X86_64_PROCESS_USER_TABLE_FLAGS;
    pdpt[stack_pdpt_index] = process->user_stack_pd_page | X86_64_PROCESS_USER_TABLE_FLAGS;
    code_pd[code_pd_index] = process->user_code_pt_page | X86_64_PROCESS_USER_TABLE_FLAGS;
    stack_pd[stack_pd_index] = process->user_stack_pt_page | X86_64_PROCESS_USER_TABLE_FLAGS;
    code_pt[code_pt_index] = process->user_code_page | X86_64_PROCESS_USER_CODE_FLAGS;
    stack_pt[stack_pt_index] = process->user_stack_page | X86_64_PROCESS_USER_STACK_FLAGS;
    process->kernel_mappings_ready = validate_kernel_mappings(process);
    process->user_page_tables_ready = validate_user_page_tables(process);
}

static u32 allocate_address_space(struct x86_64_process *process)
{
    process->cr3 = x86_64_pmm_alloc_page();
    process->user_pdpt_page = x86_64_pmm_alloc_page();
    process->user_code_pd_page = x86_64_pmm_alloc_page();
    process->user_stack_pd_page = x86_64_pmm_alloc_page();
    process->user_code_pt_page = x86_64_pmm_alloc_page();
    process->user_stack_pt_page = x86_64_pmm_alloc_page();
    process->user_code_page = x86_64_pmm_alloc_page();
    process->user_stack_page = x86_64_pmm_alloc_page();

    if (process->cr3 == 0ULL ||
        process->user_pdpt_page == 0ULL ||
        process->user_code_pd_page == 0ULL ||
        process->user_stack_pd_page == 0ULL ||
        process->user_code_pt_page == 0ULL ||
        process->user_stack_pt_page == 0ULL ||
        process->user_code_page == 0ULL ||
        process->user_stack_page == 0ULL) {
        release_address_space(process);
        return 0U;
    }

    clear_page(process->cr3);
    clear_page(process->user_pdpt_page);
    clear_page(process->user_code_pd_page);
    clear_page(process->user_stack_pd_page);
    clear_page(process->user_code_pt_page);
    clear_page(process->user_stack_pt_page);
    clear_page(process->user_code_page);
    clear_page(process->user_stack_page);

    process->address_space_id = next_address_space_id;
    next_address_space_id += 1ULL;
    process->user_code_virtual = X86_64_USER_CODE_BASE;
    process->user_stack_virtual = X86_64_USER_STACK_TOP - X86_64_USER_PAGE_BYTES;
    process->address_space_owned = 1U;

    if (clone_kernel_mappings(process) == 0U) {
        release_address_space(process);
        return 0U;
    }

    build_user_page_tables(process);

    return ((process->kernel_mappings_ready != 0U) &&
            (process->user_page_tables_ready != 0U)) ? 1U : 0U;
}

static void record_created_process(struct x86_64_process *process)
{
    process_state.created_processes += 1U;
    process_state.stack_allocations += 1U;
    process_state.address_space_allocations += 1U;
    process_state.address_space_pages_allocated += X86_64_PROCESS_ADDRESS_SPACE_PAGES;
    process_state.last_created_pid = process->pid;
    if (process->kernel_mappings_ready != 0U) {
        process_state.kernel_mappings_ready += 1U;
    }
    if (process->user_page_tables_ready != 0U) {
        process_state.user_page_tables_ready += 1U;
    }

    if (process_state.first_stack_base == 0ULL) {
        process_state.first_stack_base = process->kernel_stack_base;
        process_state.first_stack_top = process->kernel_stack_top;
        process_state.first_address_space_id = process->address_space_id;
        process_state.first_cr3 = process->cr3;
        process_state.first_user_code_page = process->user_code_page;
        process_state.first_user_stack_page = process->user_stack_page;
    } else if (process_state.second_stack_base == 0ULL) {
        process_state.second_stack_base = process->kernel_stack_base;
        process_state.second_stack_top = process->kernel_stack_top;
        process_state.second_address_space_id = process->address_space_id;
        process_state.second_cr3 = process->cr3;
        process_state.second_user_code_page = process->user_code_page;
        process_state.second_user_stack_page = process->user_stack_page;
    }
}

static void refresh_counts(void)
{
    process_state.exited_processes = count_processes_with_state(X86_64_PROCESS_EXITED);
    process_state.failed_processes = count_processes_with_state(X86_64_PROCESS_FAILED);
    process_state.completed_child_queue_depth = child_status_count;
    process_state.worker_a_count = worker_a_runs;
    process_state.worker_b_count = worker_b_runs;
    process_state.worker_a_stack_sample = worker_a_stack_sample;
    process_state.worker_b_stack_sample = worker_b_stack_sample;
}

static void refresh_child_status_high_watermark(void)
{
    process_state.completed_child_queue_depth = child_status_count;
    if (child_status_count > process_state.completed_child_queue_high_watermark) {
        process_state.completed_child_queue_high_watermark = child_status_count;
    }
}

static void remove_child_status_at(u32 index)
{
    if (index >= child_status_count) {
        return;
    }

    for (u32 i = index; (i + 1U) < child_status_count; ++i) {
        child_status_queue[i] = child_status_queue[i + 1U];
    }

    if (child_status_count != 0U) {
        child_status_count -= 1U;
    }
    if (child_status_count < X86_64_PROCESS_CHILD_STATUS_QUEUE) {
        clear_bytes(&child_status_queue[child_status_count],
                    sizeof(child_status_queue[child_status_count]));
    }
    refresh_child_status_high_watermark();
}

void x86_64_process_initialize(struct x86_64_process_smoke_state *state)
{
    clear_bytes(process_table, sizeof(process_table));
    clear_bytes(&process_state, sizeof(process_state));
    clear_bytes(child_status_queue, sizeof(child_status_queue));
    next_pid = X86_64_PROCESS_FIRST_PID;
    next_address_space_id = 1ULL;
    child_status_count = 0U;
    worker_a_runs = 0U;
    worker_b_runs = 0U;
    worker_a_stack_sample = 0ULL;
    worker_b_stack_sample = 0ULL;

    process_state.initialized = 1U;
    process_state.table_capacity = X86_64_PROCESS_MAX_PROCESSES;
    process_state.kernel_mapping_source_ready = process_paging_context.initialized;

    if (state != &process_state) {
        *state = process_state;
    }
}

u32 x86_64_process_create(const char *name,
                          x86_64_process_entry_t entry,
                          void *arg)
{
    if (process_state.initialized == 0U || entry == (x86_64_process_entry_t)0) {
        return X86_64_PROCESS_INVALID_PID;
    }

    struct x86_64_process *process = find_unused_process();
    if (process == (struct x86_64_process *)0) {
        return X86_64_PROCESS_INVALID_PID;
    }

    if (allocate_address_space(process) == 0U) {
        return X86_64_PROCESS_INVALID_PID;
    }

    if (allocate_kernel_stack(process) == 0U) {
        release_kernel_stack(process);
        release_address_space(process);
        return X86_64_PROCESS_INVALID_PID;
    }

    process->pid = next_pid;
    next_pid += 1U;
    process->state = X86_64_PROCESS_READY;
    process->mode = X86_64_PROCESS_MODE_KERNEL;
    copy_name(process->name, name);
    copy_image_path(process->image_path, "");
    process->entry = entry;
    process->arg = arg;
    process->user_entry = 0ULL;
    process->image_bytes = 0U;
    process->exit_code = 0U;

    record_created_process(process);

    process_state.stack_alignment_ok =
        ((process->kernel_stack_base != 0ULL) &&
         (is_aligned_u64(process->kernel_stack_base, 16ULL) != 0U) &&
         (is_aligned_u64(process->kernel_stack_top, 16ULL) != 0U)) ? 1U : 0U;

    return process->pid;
}

u32 x86_64_process_create_user_image(const char *path,
                                      const u8 *loaded_code_page,
                                      u64 code_bytes,
                                      u64 entry)
{
    if (process_state.initialized == 0U ||
        loaded_code_page == (const u8 *)0 ||
        code_bytes == 0ULL ||
        code_bytes > X86_64_USER_PAGE_BYTES ||
        entry < X86_64_USER_CODE_BASE ||
        entry >= (X86_64_USER_CODE_BASE + X86_64_USER_PAGE_BYTES)) {
        return X86_64_PROCESS_INVALID_PID;
    }

    struct x86_64_process *process = find_unused_process();
    if (process == (struct x86_64_process *)0) {
        return X86_64_PROCESS_INVALID_PID;
    }

    if (allocate_address_space(process) == 0U) {
        return X86_64_PROCESS_INVALID_PID;
    }

    if (allocate_kernel_stack(process) == 0U) {
        release_kernel_stack(process);
        release_address_space(process);
        return X86_64_PROCESS_INVALID_PID;
    }

    process->pid = next_pid;
    next_pid += 1U;
    process->state = X86_64_PROCESS_READY;
    process->mode = X86_64_PROCESS_MODE_USER;
    copy_name(process->name, "user-process");
    copy_image_path(process->image_path, path);
    process->entry = (x86_64_process_entry_t)0;
    process->arg = (void *)0;
    process->user_entry = entry;
    process->image_bytes = (u32)code_bytes;
    process->exit_code = 0U;
    copy_bytes((void *)process->user_code_page, loaded_code_page, (usize)code_bytes);

    record_created_process(process);
    process_state.user_processes_created += 1U;
    process_state.user_image_bytes = process->image_bytes;
    process_state.user_image_copied = 1U;
    process_state.user_process_ready = 1U;
    process_state.last_user_pid = process->pid;
    process_state.last_user_entry = process->user_entry;
    process_state.last_user_cr3 = process->cr3;
    process_state.last_user_pdpt_page = process->user_pdpt_page;
    process_state.last_user_code_pd_page = process->user_code_pd_page;
    process_state.last_user_stack_pd_page = process->user_stack_pd_page;
    process_state.last_user_code_pt_page = process->user_code_pt_page;
    process_state.last_user_stack_pt_page = process->user_stack_pt_page;
    process_state.last_user_code_page = process->user_code_page;
    process_state.last_user_stack_page = process->user_stack_page;
    process_state.user_page_table_entries_ok = validate_user_page_tables(process);

    return process->pid;
}

u32 x86_64_process_mark_user_exited(u32 pid, u32 exit_code)
{
    struct x86_64_process *process = find_process_by_pid(pid);
    if (process == (struct x86_64_process *)0 ||
        process->mode != X86_64_PROCESS_MODE_USER ||
        process->state == X86_64_PROCESS_UNUSED) {
        return 0U;
    }

    if (process->state != X86_64_PROCESS_EXITED) {
        process_state.user_processes_exited += 1U;
    }

    process->state = X86_64_PROCESS_EXITED;
    process->exit_code = exit_code;
    process_state.last_exited_user_pid = pid;
    refresh_counts();

    return 1U;
}

u32 x86_64_process_reap_user(u32 pid)
{
    struct x86_64_process *process = find_process_by_pid(pid);
    if (process == (struct x86_64_process *)0 ||
        process->mode != X86_64_PROCESS_MODE_USER ||
        (process->state != X86_64_PROCESS_EXITED &&
         process->state != X86_64_PROCESS_FAILED)) {
        process_state.reap_failures += 1U;
        return 0U;
    }

    release_process_resources(process);
    clear_bytes(process, sizeof(*process));
    process_state.user_processes_reaped += 1U;
    process_state.last_reaped_pid = pid;
    refresh_counts();

    return 1U;
}

u32 x86_64_process_reap_exited_user_processes(void)
{
    u32 reaped = 0U;
    for (u32 i = 0; i < X86_64_PROCESS_MAX_PROCESSES; ++i) {
        u32 pid = process_table[i].pid;
        if (process_table[i].mode == X86_64_PROCESS_MODE_USER &&
            (process_table[i].state == X86_64_PROCESS_EXITED ||
             process_table[i].state == X86_64_PROCESS_FAILED) &&
            x86_64_process_reap_user(pid) != 0U) {
            reaped += 1U;
        }
    }

    return reaped;
}

u32 x86_64_process_record_child_exit(u32 parent_pid, u32 child_pid, u32 exit_code)
{
    if (parent_pid == X86_64_PROCESS_INVALID_PID ||
        child_pid == X86_64_PROCESS_INVALID_PID ||
        parent_pid == child_pid) {
        return 0U;
    }

    if (child_status_count >= X86_64_PROCESS_CHILD_STATUS_QUEUE) {
        remove_child_status_at(0U);
        process_state.completed_child_drops += 1U;
    }

    child_status_queue[child_status_count].parent_pid = parent_pid;
    child_status_queue[child_status_count].child_pid = child_pid;
    child_status_queue[child_status_count].exit_code = exit_code;
    child_status_count += 1U;

    process_state.completed_child_records += 1U;
    process_state.last_completed_parent_pid = parent_pid;
    process_state.last_completed_child_pid = child_pid;
    process_state.last_completed_exit_code = exit_code;
    refresh_child_status_high_watermark();

    return 1U;
}

u32 x86_64_process_wait_child(u32 parent_pid, u32 *child_pid_out, u32 *exit_code_out)
{
    if (parent_pid == X86_64_PROCESS_INVALID_PID ||
        child_pid_out == (u32 *)0 ||
        exit_code_out == (u32 *)0) {
        return 0U;
    }

    for (u32 i = 0U; i < child_status_count; ++i) {
        if (child_status_queue[i].parent_pid != parent_pid) {
            continue;
        }

        *child_pid_out = child_status_queue[i].child_pid;
        *exit_code_out = child_status_queue[i].exit_code;

        process_state.completed_child_waits += 1U;
        process_state.last_wait_parent_pid = parent_pid;
        process_state.last_wait_child_pid = child_status_queue[i].child_pid;
        process_state.last_wait_exit_code = child_status_queue[i].exit_code;

        remove_child_status_at(i);

        return 1U;
    }

    process_state.wait_misses += 1U;
    refresh_child_status_high_watermark();
    return 0U;
}

u64 x86_64_process_snapshot(char *buffer, u64 size)
{
    if (buffer == (char *)0 || size == 0ULL) {
        return 0ULL;
    }

    u64 offset = 0ULL;
    offset = append_string(buffer, size, offset, "PID STATE MODE NAME\n");

    for (u32 i = 0U; i < X86_64_PROCESS_MAX_PROCESSES; ++i) {
        const struct x86_64_process *process = &process_table[i];
        if (process->state == X86_64_PROCESS_UNUSED) {
            continue;
        }

        offset = append_u32_decimal(buffer, size, offset, process->pid);
        offset = append_char(buffer, size, offset, ' ');
        offset = append_string(buffer, size, offset, process_state_name(process->state));
        offset = append_char(buffer, size, offset, ' ');
        offset = append_string(buffer, size, offset, process_mode_name(process->mode));
        offset = append_char(buffer, size, offset, ' ');
        offset = append_string(buffer, size, offset, process->name);
        if (process->image_path[0] != '\0') {
            offset = append_char(buffer, size, offset, ' ');
            offset = append_string(buffer, size, offset, process->image_path);
        }
        offset = append_char(buffer, size, offset, '\n');
    }

    offset = append_string(buffer, size, offset, "created ");
    offset = append_u32_decimal(buffer, size, offset, process_state.created_processes);
    offset = append_string(buffer, size, offset, " user-created ");
    offset = append_u32_decimal(buffer, size, offset, process_state.user_processes_created);
    offset = append_string(buffer, size, offset, " user-exited ");
    offset = append_u32_decimal(buffer, size, offset, process_state.user_processes_exited);
    offset = append_string(buffer, size, offset, " user-reaped ");
    offset = append_u32_decimal(buffer, size, offset, process_state.user_processes_reaped);
    offset = append_string(buffer, size, offset, " wait-ready ");
    offset = append_u32_decimal(buffer, size, offset, child_status_count);
    offset = append_string(buffer, size, offset, " wait-records ");
    offset = append_u32_decimal(buffer, size, offset, process_state.completed_child_records);
    offset = append_string(buffer, size, offset, " waits ");
    offset = append_u32_decimal(buffer, size, offset, process_state.completed_child_waits);
    offset = append_string(buffer, size, offset, " wait-drops ");
    offset = append_u32_decimal(buffer, size, offset, process_state.completed_child_drops);
    offset = append_char(buffer, size, offset, '\n');

    if (offset < size) {
        buffer[offset] = '\0';
    } else {
        buffer[size - 1ULL] = '\0';
    }

    return (offset < size) ? offset : (size - 1ULL);
}

u32 x86_64_process_prepare_next_user(struct x86_64_user_schedule_state *state)
{
    if (state == (struct x86_64_user_schedule_state *)0) {
        return 0U;
    }

    clear_bytes(state, sizeof(*state));
    state->initialized = 1U;

    if (process_state.initialized == 0U) {
        return 0U;
    }

    struct x86_64_process *process = find_next_ready_user_process();
    if (process == (struct x86_64_process *)0) {
        return 0U;
    }

    state->candidate_found = 1U;
    state->selected_pid = process->pid;
    state->selected_state = process->state;
    state->selected_mode = process->mode;
    state->address_space_id = process->address_space_id;
    state->cr3 = process->cr3;
    state->user_entry = process->user_entry;
    state->user_rsp = X86_64_USER_STACK_TOP - X86_64_PROCESS_USER_STACK_ALIGNMENT;
    state->user_rflags = X86_64_PROCESS_USER_RFLAGS;
    state->user_code_page = process->user_code_page;
    state->user_stack_page = process->user_stack_page;
    state->entry_valid =
        ((state->user_entry >= X86_64_USER_CODE_BASE) &&
         (state->user_entry < (X86_64_USER_CODE_BASE + X86_64_USER_PAGE_BYTES))) ? 1U : 0U;
    state->stack_valid =
        ((state->user_rsp >= process->user_stack_virtual) &&
         (state->user_rsp < X86_64_USER_STACK_TOP) &&
         (is_aligned_u64(state->user_rsp, X86_64_PROCESS_USER_STACK_ALIGNMENT) != 0U)) ? 1U : 0U;
    state->cr3_valid =
        ((state->cr3 != 0ULL) &&
         (is_aligned_u64(state->cr3, X86_64_USER_PAGE_BYTES) != 0U)) ? 1U : 0U;
    state->code_page_valid =
        ((state->user_code_page != 0ULL) &&
         (is_aligned_u64(state->user_code_page, X86_64_USER_PAGE_BYTES) != 0U)) ? 1U : 0U;
    state->stack_page_valid =
        ((state->user_stack_page != 0ULL) &&
         (is_aligned_u64(state->user_stack_page, X86_64_USER_PAGE_BYTES) != 0U)) ? 1U : 0U;
    state->page_tables_valid = process->user_page_tables_ready;
    state->kernel_mappings_valid = process->kernel_mappings_ready;
    state->transition_ready =
        ((state->candidate_found != 0U) &&
         (state->entry_valid != 0U) &&
         (state->stack_valid != 0U) &&
         (state->cr3_valid != 0U) &&
         (state->code_page_valid != 0U) &&
         (state->stack_page_valid != 0U) &&
         (state->page_tables_valid != 0U) &&
         (state->kernel_mappings_valid != 0U) &&
         (process->address_space_owned != 0U)) ? 1U : 0U;
    state->scheduler_ok = state->transition_ready;

    process_state.user_scheduler_ready = state->scheduler_ok;
    process_state.last_scheduled_user_pid = state->selected_pid;
    process_state.scheduled_user_rsp = state->user_rsp;
    process_state.scheduled_user_rflags = state->user_rflags;

    return state->scheduler_ok;
}

u32 x86_64_process_run_next_ready(void)
{
    struct x86_64_process *process = find_next_ready_process();
    if (process == (struct x86_64_process *)0) {
        return 0U;
    }

    process_state.run_attempts += 1U;
    process_state.last_run_pid = process->pid;
    process->state = X86_64_PROCESS_RUNNING;

    if (process->entry == (x86_64_process_entry_t)0 ||
        process->kernel_stack_top == 0ULL ||
        process->address_space_owned == 0U ||
        process->cr3 == 0ULL ||
        process->kernel_mappings_ready == 0U) {
        process->state = X86_64_PROCESS_FAILED;
        refresh_counts();
        return 0U;
    }

    x86_64_process_enter_stack(process->kernel_stack_top, process->entry, process->arg);
    process_state.stack_switches += 1U;
    process->state = X86_64_PROCESS_EXITED;
    process->exit_code = 0U;
    process_state.run_count += 1U;
    refresh_counts();

    return 1U;
}

u32 x86_64_process_run_all_ready(u32 max_runs)
{
    u32 runs = 0U;
    while (runs < max_runs) {
        if (x86_64_process_run_next_ready() == 0U) {
            break;
        }
        runs += 1U;
    }

    return runs;
}

static void process_worker_a(void *arg)
{
    (void)arg;
    worker_a_stack_sample = read_rsp();
    worker_a_runs += 1U;
}

static void process_worker_b(void *arg)
{
    (void)arg;
    worker_b_stack_sample = read_rsp();
    worker_b_runs += 1U;
}

void x86_64_process_run_smoke(struct x86_64_process_smoke_state *state)
{
    struct x86_64_userland_foundation_state userland_state;
    struct x86_64_syscall_dispatch_state syscall_dispatch_state;
    struct x86_64_user_context_state user_context_state;
    struct x86_64_user_schedule_state user_schedule_state;

    x86_64_process_initialize(&process_state);

    u32 first_pid = x86_64_process_create("boot-worker-a", process_worker_a, (void *)0);
    u32 second_pid = x86_64_process_create("boot-worker-b", process_worker_b, (void *)0);
    process_state.ready_before_run = count_processes_with_state(X86_64_PROCESS_READY);

    (void)x86_64_process_run_all_ready(X86_64_PROCESS_MAX_PROCESSES);
    refresh_counts();

    struct x86_64_process *first_process = find_process_by_pid(first_pid);
    u64 context_address_space_id = (first_process != (struct x86_64_process *)0) ?
        first_process->address_space_id : (u64)first_pid;
    u64 context_cr3 = (first_process != (struct x86_64_process *)0) ? first_process->cr3 : 0ULL;

    u32 user_pid = X86_64_PROCESS_INVALID_PID;
    if (first_process != (struct x86_64_process *)0) {
        user_pid = x86_64_process_create_user_image("/bin/smoke",
                                                    (const u8 *)first_process->user_code_page,
                                                    16ULL,
                                                    X86_64_USER_CODE_BASE);
    }
    (void)x86_64_process_prepare_next_user(&user_schedule_state);
    refresh_counts();

    x86_64_userland_foundation_init(&userland_state);
    x86_64_syscall_dispatch_run_smoke(&syscall_dispatch_state, first_pid);
    x86_64_user_context_init(&user_context_state, &userland_state,
                             context_address_space_id, context_cr3);
    process_state.userland_foundation_ok = userland_state.foundation_ok;
    process_state.syscall_dispatcher_ok = syscall_dispatch_state.dispatcher_ok;
    process_state.user_context_ok = user_context_state.context_ok;

    u32 worker_a_stack_ok = is_inside_stack(process_state.worker_a_stack_sample,
                                            process_state.first_stack_base,
                                            process_state.first_stack_top);
    u32 worker_b_stack_ok = is_inside_stack(process_state.worker_b_stack_sample,
                                            process_state.second_stack_base,
                                            process_state.second_stack_top);

    process_state.stack_alignment_ok =
        ((process_state.first_stack_base != 0ULL) &&
         (process_state.second_stack_base != 0ULL) &&
         (is_aligned_u64(process_state.first_stack_base, 16ULL) != 0U) &&
         (is_aligned_u64(process_state.first_stack_top, 16ULL) != 0U) &&
         (is_aligned_u64(process_state.second_stack_base, 16ULL) != 0U) &&
         (is_aligned_u64(process_state.second_stack_top, 16ULL) != 0U)) ? 1U : 0U;

    process_state.stack_execution_ok =
        ((process_state.stack_switches == 2U) &&
         (worker_a_stack_ok != 0U) &&
         (worker_b_stack_ok != 0U)) ? 1U : 0U;

    process_state.address_spaces_distinct =
        ((process_state.first_address_space_id != 0ULL) &&
         (process_state.second_address_space_id != 0ULL) &&
         (process_state.first_address_space_id != process_state.second_address_space_id) &&
         (process_state.first_cr3 != 0ULL) &&
         (process_state.second_cr3 != 0ULL) &&
         (process_state.first_cr3 != process_state.second_cr3) &&
         (process_state.first_user_code_page != process_state.second_user_code_page) &&
         (process_state.first_user_stack_page != process_state.second_user_stack_page) &&
         (process_state.last_user_cr3 != 0ULL) &&
         (process_state.last_user_cr3 != process_state.first_cr3) &&
         (process_state.last_user_cr3 != process_state.second_cr3) &&
         (process_state.last_user_pdpt_page != 0ULL) &&
         (process_state.last_user_code_pd_page != 0ULL) &&
         (process_state.last_user_stack_pd_page != 0ULL) &&
         (process_state.last_user_code_pt_page != 0ULL) &&
         (process_state.last_user_stack_pt_page != 0ULL) &&
         (process_state.last_user_code_page != process_state.first_user_code_page) &&
         (process_state.last_user_stack_page != process_state.first_user_stack_page)) ? 1U : 0U;

    process_state.address_space_ok =
        ((process_state.address_space_allocations == process_state.created_processes) &&
         (process_state.address_space_pages_allocated ==
          (process_state.created_processes * X86_64_PROCESS_ADDRESS_SPACE_PAGES)) &&
         (process_state.address_spaces_distinct != 0U) &&
         (process_state.kernel_mapping_source_ready != 0U) &&
         (process_state.kernel_mappings_ready == process_state.created_processes) &&
         (process_state.user_page_tables_ready == process_state.created_processes) &&
         (process_state.user_page_table_entries_ok != 0U) &&
         (is_aligned_u64(process_state.first_cr3, X86_64_USER_PAGE_BYTES) != 0U) &&
         (is_aligned_u64(process_state.second_cr3, X86_64_USER_PAGE_BYTES) != 0U) &&
         (is_aligned_u64(process_state.last_user_cr3, X86_64_USER_PAGE_BYTES) != 0U)) ? 1U : 0U;

    u32 user_reap_ok = 0U;
    u32 wait_ok = 0U;
    u32 second_wait_ok = 0U;
    if (user_pid != X86_64_PROCESS_INVALID_PID) {
        u32 mark_ok = x86_64_process_mark_user_exited(user_pid, 0U);
        u32 record_ok = x86_64_process_record_child_exit(first_pid, user_pid, 0U);
        u32 second_record_ok = x86_64_process_record_child_exit(first_pid, second_pid, 7U);
        u32 waited_pid = 0U;
        u32 waited_exit_code = 0U;
        wait_ok = x86_64_process_wait_child(first_pid, &waited_pid, &waited_exit_code);
        u32 second_waited_pid = 0U;
        u32 second_waited_exit_code = 0U;
        second_wait_ok = x86_64_process_wait_child(first_pid, &second_waited_pid,
                                                   &second_waited_exit_code);
        u32 reap_ok = x86_64_process_reap_user(user_pid);
        user_reap_ok =
            ((mark_ok != 0U) &&
             (record_ok != 0U) &&
             (second_record_ok != 0U) &&
             (wait_ok != 0U) &&
             (waited_pid == user_pid) &&
             (waited_exit_code == 0U) &&
             (second_wait_ok != 0U) &&
             (second_waited_pid == second_pid) &&
             (second_waited_exit_code == 7U) &&
             (reap_ok != 0U)) ? 1U : 0U;
    }

    process_state.smoke_ok =
        ((process_state.initialized != 0U) &&
         (first_pid != X86_64_PROCESS_INVALID_PID) &&
         (second_pid != X86_64_PROCESS_INVALID_PID) &&
         (user_pid != X86_64_PROCESS_INVALID_PID) &&
         (process_state.created_processes == 3U) &&
         (process_state.ready_before_run == 2U) &&
         (process_state.run_count == 2U) &&
         (process_state.exited_processes == 2U) &&
         (process_state.failed_processes == 0U) &&
         (process_state.stack_allocations == 3U) &&
         (process_state.stack_alignment_ok != 0U) &&
         (process_state.stack_execution_ok != 0U) &&
         (process_state.address_space_ok != 0U) &&
         (process_state.user_processes_created == 1U) &&
         (process_state.user_processes_exited == 1U) &&
         (process_state.user_processes_reaped == 1U) &&
         (process_state.completed_child_records == 2U) &&
         (process_state.completed_child_waits == 2U) &&
         (process_state.completed_child_queue_depth == 0U) &&
         (process_state.completed_child_queue_high_watermark == 2U) &&
         (process_state.completed_child_drops == 0U) &&
         (process_state.reap_failures == 0U) &&
         (process_state.last_exited_user_pid == user_pid) &&
         (process_state.last_reaped_pid == user_pid) &&
         (process_state.last_completed_parent_pid == first_pid) &&
         (process_state.last_completed_child_pid == second_pid) &&
         (process_state.last_wait_parent_pid == first_pid) &&
         (process_state.last_wait_child_pid == second_pid) &&
         (user_reap_ok != 0U) &&
         (wait_ok != 0U) &&
         (second_wait_ok != 0U) &&
         (process_state.user_image_bytes == 16U) &&
         (process_state.user_image_copied != 0U) &&
         (process_state.user_process_ready != 0U) &&
         (process_state.user_scheduler_ready != 0U) &&
         (process_state.worker_a_count == 1U) &&
         (process_state.worker_b_count == 1U) &&
         (process_state.userland_foundation_ok != 0U) &&
         (process_state.syscall_dispatcher_ok != 0U) &&
         (process_state.user_context_ok != 0U)) ? 1U : 0U;

    x86_64_console_write_u32(25, 0, "Process smoke ok: ", process_state.smoke_ok);

    x86_64_serial_write_line("x86_64 process scheduler smoke online");
    x86_64_serial_write_u32("Process initialized: ", process_state.initialized);
    x86_64_serial_write_u32("Process table capacity: ", process_state.table_capacity);
    x86_64_serial_write_u32("Process created: ", process_state.created_processes);
    x86_64_serial_write_u32("Process ready before run: ", process_state.ready_before_run);
    x86_64_serial_write_u32("Process run attempts: ", process_state.run_attempts);
    x86_64_serial_write_u32("Process run count: ", process_state.run_count);
    x86_64_serial_write_u32("Process exited: ", process_state.exited_processes);
    x86_64_serial_write_u32("Process failed: ", process_state.failed_processes);
    x86_64_serial_write_u32("Process stack allocations: ", process_state.stack_allocations);
    x86_64_serial_write_u32("Process stack alignment ok: ", process_state.stack_alignment_ok);
    x86_64_serial_write_u32("Process stack switches: ", process_state.stack_switches);
    x86_64_serial_write_u32("Process stack execution ok: ", process_state.stack_execution_ok);
    x86_64_serial_write_u32("Process address spaces: ", process_state.address_space_allocations);
    x86_64_serial_write_u32("Process address-space pages: ", process_state.address_space_pages_allocated);
    x86_64_serial_write_u32("Process address spaces distinct: ", process_state.address_spaces_distinct);
    x86_64_serial_write_u32("Process address space ok: ", process_state.address_space_ok);
    x86_64_serial_write_u32("Process kernel mapping source ready: ", process_state.kernel_mapping_source_ready);
    x86_64_serial_write_u32("Process kernel mappings ready: ", process_state.kernel_mappings_ready);
    x86_64_serial_write_u32("Process page tables ready: ", process_state.user_page_tables_ready);
    x86_64_serial_write_u32("Process page table entries ok: ", process_state.user_page_table_entries_ok);
    x86_64_serial_write_u32("Process user creates: ", process_state.user_processes_created);
    x86_64_serial_write_u32("Process user exited: ", process_state.user_processes_exited);
    x86_64_serial_write_u32("Process user reaped: ", process_state.user_processes_reaped);
    x86_64_serial_write_u32("Process reap failures: ", process_state.reap_failures);
    x86_64_serial_write_u32("Process child records: ", process_state.completed_child_records);
    x86_64_serial_write_u32("Process child waits: ", process_state.completed_child_waits);
    x86_64_serial_write_u32("Process wait misses: ", process_state.wait_misses);
    x86_64_serial_write_u32("Process child queue depth: ", process_state.completed_child_queue_depth);
    x86_64_serial_write_u32("Process child queue high watermark: ", process_state.completed_child_queue_high_watermark);
    x86_64_serial_write_u32("Process child drops: ", process_state.completed_child_drops);
    x86_64_serial_write_u32("Process user image bytes: ", process_state.user_image_bytes);
    x86_64_serial_write_u32("Process user image copied: ", process_state.user_image_copied);
    x86_64_serial_write_u32("Process user ready: ", process_state.user_process_ready);
    x86_64_serial_write_u32("Process user scheduler ready: ", process_state.user_scheduler_ready);
    x86_64_serial_write_u32("Process last created pid: ", process_state.last_created_pid);
    x86_64_serial_write_u32("Process last user pid: ", process_state.last_user_pid);
    x86_64_serial_write_u32("Process last exited user pid: ", process_state.last_exited_user_pid);
    x86_64_serial_write_u32("Process last reaped pid: ", process_state.last_reaped_pid);
    x86_64_serial_write_u32("Process last completed parent pid: ", process_state.last_completed_parent_pid);
    x86_64_serial_write_u32("Process last completed child pid: ", process_state.last_completed_child_pid);
    x86_64_serial_write_u32("Process last completed exit code: ", process_state.last_completed_exit_code);
    x86_64_serial_write_u32("Process last wait parent pid: ", process_state.last_wait_parent_pid);
    x86_64_serial_write_u32("Process last wait child pid: ", process_state.last_wait_child_pid);
    x86_64_serial_write_u32("Process last wait exit code: ", process_state.last_wait_exit_code);
    x86_64_serial_write_u32("Process last scheduled user pid: ", process_state.last_scheduled_user_pid);
    x86_64_serial_write_u32("Process last run pid: ", process_state.last_run_pid);
    x86_64_serial_write_hex64("Process first stack base: 0x", process_state.first_stack_base);
    x86_64_serial_write_hex64("Process first stack top: 0x", process_state.first_stack_top);
    x86_64_serial_write_hex64("Process second stack base: 0x", process_state.second_stack_base);
    x86_64_serial_write_hex64("Process second stack top: 0x", process_state.second_stack_top);
    x86_64_serial_write_hex64("Process first address space id: 0x", process_state.first_address_space_id);
    x86_64_serial_write_hex64("Process second address space id: 0x", process_state.second_address_space_id);
    x86_64_serial_write_hex64("Process first CR3: 0x", process_state.first_cr3);
    x86_64_serial_write_hex64("Process second CR3: 0x", process_state.second_cr3);
    x86_64_serial_write_hex64("Process first user code page: 0x", process_state.first_user_code_page);
    x86_64_serial_write_hex64("Process second user code page: 0x", process_state.second_user_code_page);
    x86_64_serial_write_hex64("Process first user stack page: 0x", process_state.first_user_stack_page);
    x86_64_serial_write_hex64("Process second user stack page: 0x", process_state.second_user_stack_page);
    x86_64_serial_write_hex64("Process last user entry: 0x", process_state.last_user_entry);
    x86_64_serial_write_hex64("Process last user CR3: 0x", process_state.last_user_cr3);
    x86_64_serial_write_hex64("Process last user PDPT: 0x", process_state.last_user_pdpt_page);
    x86_64_serial_write_hex64("Process last user code PD: 0x", process_state.last_user_code_pd_page);
    x86_64_serial_write_hex64("Process last user stack PD: 0x", process_state.last_user_stack_pd_page);
    x86_64_serial_write_hex64("Process last user code PT: 0x", process_state.last_user_code_pt_page);
    x86_64_serial_write_hex64("Process last user stack PT: 0x", process_state.last_user_stack_pt_page);
    x86_64_serial_write_hex64("Process last user code page: 0x", process_state.last_user_code_page);
    x86_64_serial_write_hex64("Process last user stack page: 0x", process_state.last_user_stack_page);
    x86_64_serial_write_hex64("Process scheduled user RSP: 0x", process_state.scheduled_user_rsp);
    x86_64_serial_write_hex64("Process scheduled user RFLAGS: 0x", process_state.scheduled_user_rflags);
    x86_64_serial_write_hex64("Process worker A stack sample: 0x", process_state.worker_a_stack_sample);
    x86_64_serial_write_hex64("Process worker B stack sample: 0x", process_state.worker_b_stack_sample);
    x86_64_serial_write_u32("Process worker A count: ", process_state.worker_a_count);
    x86_64_serial_write_u32("Process worker B count: ", process_state.worker_b_count);
    x86_64_serial_write_u32("Process userland foundation ok: ", process_state.userland_foundation_ok);
    x86_64_serial_write_u32("Process syscall dispatcher ok: ", process_state.syscall_dispatcher_ok);
    x86_64_serial_write_u32("Process user context ok: ", process_state.user_context_ok);
    x86_64_userland_foundation_report(&userland_state);
    x86_64_syscall_dispatch_report(&syscall_dispatch_state);
    x86_64_user_context_report(&user_context_state);
    x86_64_serial_write_u32("Process smoke ok: ", process_state.smoke_ok);

    if (state != &process_state) {
        *state = process_state;
    }
}

const struct x86_64_process_smoke_state *x86_64_process_get_smoke_state(void)
{
    return &process_state;
}
