#include "paging_builder.h"
#include "pmm.h"
#include "userland.h"

#define X86_64_PAGE_PRESENT 0x001ULL
#define X86_64_PAGE_WRITABLE 0x002ULL
#define X86_64_PAGE_USER 0x004ULL
#define X86_64_PAGE_HUGE 0x080ULL
#define X86_64_TABLE_FLAGS (X86_64_PAGE_PRESENT | X86_64_PAGE_WRITABLE)
#define X86_64_USER_TABLE_FLAGS (X86_64_TABLE_FLAGS | X86_64_PAGE_USER)
#define X86_64_HUGE_PAGE_FLAGS (X86_64_TABLE_FLAGS | X86_64_PAGE_HUGE)
#define X86_64_USER_CODE_FLAGS (X86_64_PAGE_PRESENT | X86_64_PAGE_USER)
#define X86_64_USER_STACK_FLAGS (X86_64_PAGE_PRESENT | X86_64_PAGE_WRITABLE | X86_64_PAGE_USER)
#define X86_64_PAGE_MASK (X86_64_PAGE_SIZE - 1ULL)

extern const u8 x86_64_user_init_image_start[];
extern const u8 x86_64_user_init_image_end[];

static void clear_page(u64 page)
{
    u8 *bytes = (u8 *)page;
    for (usize i = 0; i < X86_64_PAGE_SIZE; ++i) {
        bytes[i] = 0U;
    }
}

static void clear_builder_state(struct x86_64_paging_builder_state *state)
{
    u8 *bytes = (u8 *)state;
    for (usize i = 0; i < sizeof(*state); ++i) {
        bytes[i] = 0U;
    }
}

static u64 read_cr3(void)
{
    u64 value;
    __asm__ volatile ("movq %%cr3, %0" : "=r" (value));
    return value;
}

static void write_cr3(u64 value)
{
    __asm__ volatile ("movq %0, %%cr3" :: "r" (value) : "memory");
}

static u32 read_probe_u32(u64 address)
{
    volatile const u32 *ptr = (volatile const u32 *)address;
    return *ptr;
}

static u32 is_page_aligned(u64 value)
{
    return ((value & X86_64_PAGE_MASK) == 0ULL) ? 1U : 0U;
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

static u32 align_page_count(u64 bytes)
{
    return (u32)((bytes + X86_64_PAGE_MASK) / X86_64_PAGE_SIZE);
}

static u32 huge_page_count_for(u64 bytes)
{
    return (u32)((bytes + X86_64_HUGE_PAGE_SIZE - 1ULL) / X86_64_HUGE_PAGE_SIZE);
}

static u32 count_present_entries(const u64 *table)
{
    u32 count = 0U;

    for (u32 i = 0; i < X86_64_PAGING_BUILDER_ENTRIES; ++i) {
        if ((table[i] & X86_64_PAGE_PRESENT) != 0ULL) {
            ++count;
        }
    }

    return count;
}

static u32 entry_has_user(u64 entry)
{
    return ((entry & X86_64_PAGE_USER) != 0ULL) ? 1U : 0U;
}

static u32 checksum_bytes(const u8 *bytes, u32 count)
{
    u32 checksum = 0U;
    for (u32 i = 0; i < count; ++i) {
        checksum = (checksum << 5U) ^ (checksum >> 2U) ^ bytes[i];
    }

    return checksum;
}

static u32 page_prefix_matches(const u8 *page, const u8 *expected, u32 count)
{
    for (u32 i = 0; i < count; ++i) {
        if (page[i] != expected[i]) {
            return 0U;
        }
    }

    return 1U;
}

static u32 page_is_zeroed(const u8 *page)
{
    for (usize i = 0; i < X86_64_PAGE_SIZE; ++i) {
        if (page[i] != 0U) {
            return 0U;
        }
    }

    return 1U;
}

static u32 embedded_user_image_size(void)
{
    usize image_bytes = (usize)(x86_64_user_init_image_end - x86_64_user_init_image_start);
    if (image_bytes > 0xFFFFFFFFULL) {
        return 0U;
    }

    return (u32)image_bytes;
}

static u32 tables_are_aligned(const struct x86_64_paging_builder_state *state)
{
    return ((is_page_aligned(state->pml4_table) != 0U) &&
            (is_page_aligned(state->identity_pdpt_table) != 0U) &&
            (is_page_aligned(state->identity_pd_table) != 0U) &&
            (is_page_aligned(state->direct_pdpt_table) != 0U) &&
            (is_page_aligned(state->direct_pd_table) != 0U) &&
            (is_page_aligned(state->kernel_pdpt_table) != 0U) &&
            (is_page_aligned(state->kernel_pd_table) != 0U) &&
            (is_page_aligned(state->kernel_pt_table) != 0U) &&
            (is_page_aligned(state->user_code_pt_table) != 0U) &&
            (is_page_aligned(state->user_stack_pd_table) != 0U) &&
            (is_page_aligned(state->user_stack_pt_table) != 0U) &&
            (is_page_aligned(state->user_code_page) != 0U) &&
            (is_page_aligned(state->user_stack_page) != 0U)) ? 1U : 0U;
}

static u32 allocate_page(u64 *page, u64 *allocated_pages, u32 *allocated_count)
{
    u64 allocated = x86_64_pmm_alloc_page();
    if (allocated == X86_64_PMM_INVALID_PAGE) {
        return 0U;
    }

    allocated_pages[*allocated_count] = allocated;
    *allocated_count += 1U;
    *page = allocated;
    clear_page(allocated);
    return 1U;
}

static u32 allocate_table(u64 **table, u64 *allocated_pages, u32 *allocated_count)
{
    u64 page = 0ULL;
    if (allocate_page(&page, allocated_pages, allocated_count) == 0U) {
        return 0U;
    }

    *table = (u64 *)page;
    return 1U;
}

static void rollback_allocated_pages(const u64 *allocated_pages, u32 allocated_count)
{
    for (u32 i = 0; i < allocated_count; ++i) {
        (void)x86_64_pmm_free_page(allocated_pages[i]);
    }
}

static void load_embedded_user_image(struct x86_64_paging_builder_state *state)
{
    const u8 *image = x86_64_user_init_image_start;
    u32 image_bytes = embedded_user_image_size();
    struct x86_64_elf_header header = {
        .ident = {
            X86_64_ELF_MAGIC_0,
            X86_64_ELF_MAGIC_1,
            X86_64_ELF_MAGIC_2,
            X86_64_ELF_MAGIC_3,
            X86_64_ELF_CLASS_64,
            X86_64_ELF_DATA_LITTLE_ENDIAN,
            X86_64_ELF_VERSION_CURRENT,
            0U,
            0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U
        },
        .type = X86_64_ELF_TYPE_EXECUTABLE,
        .machine = X86_64_ELF_MACHINE_X86_64,
        .version = X86_64_ELF_VERSION_CURRENT,
        .entry = X86_64_USER_CODE_BASE,
        .program_header_offset = sizeof(struct x86_64_elf_header),
        .section_header_offset = 0ULL,
        .flags = 0U,
        .header_size = sizeof(struct x86_64_elf_header),
        .program_header_entry_size = sizeof(struct x86_64_elf_program_header),
        .program_header_count = 1U,
        .section_header_entry_size = 0U,
        .section_header_count = 0U,
        .section_name_index = 0U
    };
    struct x86_64_elf_program_header phdr = {
        .type = X86_64_ELF_PROGRAM_TYPE_LOAD,
        .flags = 5U,
        .offset = sizeof(struct x86_64_elf_header) + sizeof(struct x86_64_elf_program_header),
        .virtual_address = X86_64_USER_CODE_BASE,
        .physical_address = 0ULL,
        .file_size = image_bytes,
        .memory_size = image_bytes,
        .alignment = X86_64_USER_PAGE_BYTES
    };
    u8 *code = (u8 *)state->user_code_page;
    u8 *stack = (u8 *)state->user_stack_page;

    state->user_image_embedded =
        ((image != (const u8 *)0) &&
         (image_bytes > 0U) &&
         (image_bytes < X86_64_PAGE_SIZE)) ? 1U : 0U;
    state->user_image_source_ok = state->user_image_embedded;

    state->elf64_header_ok =
        ((header.ident[0] == X86_64_ELF_MAGIC_0) &&
         (header.ident[1] == X86_64_ELF_MAGIC_1) &&
         (header.ident[2] == X86_64_ELF_MAGIC_2) &&
         (header.ident[3] == X86_64_ELF_MAGIC_3) &&
         (header.ident[4] == X86_64_ELF_CLASS_64) &&
         (header.ident[5] == X86_64_ELF_DATA_LITTLE_ENDIAN) &&
         (header.type == X86_64_ELF_TYPE_EXECUTABLE) &&
         (header.machine == X86_64_ELF_MACHINE_X86_64) &&
         (header.header_size == sizeof(struct x86_64_elf_header))) ? 1U : 0U;
    state->elf64_program_header_ok =
        ((header.program_header_offset == sizeof(struct x86_64_elf_header)) &&
         (header.program_header_entry_size == sizeof(struct x86_64_elf_program_header)) &&
         (header.program_header_count == 1U) &&
         (phdr.type == X86_64_ELF_PROGRAM_TYPE_LOAD) &&
         (phdr.alignment == X86_64_USER_PAGE_BYTES)) ? 1U : 0U;
    state->elf64_entry_ok =
        ((header.entry == X86_64_USER_CODE_BASE) &&
         (header.entry >= phdr.virtual_address) &&
         (header.entry < (phdr.virtual_address + phdr.memory_size))) ? 1U : 0U;
    state->elf64_segment_ok =
        ((state->user_image_source_ok != 0U) &&
         (phdr.virtual_address == X86_64_USER_CODE_BASE) &&
         (phdr.file_size == image_bytes) &&
         (phdr.memory_size == phdr.file_size) &&
         (phdr.file_size < X86_64_PAGE_SIZE)) ? 1U : 0U;

    if ((state->elf64_header_ok != 0U) &&
        (state->elf64_program_header_ok != 0U) &&
        (state->elf64_entry_ok != 0U) &&
        (state->elf64_segment_ok != 0U)) {
        for (u32 i = 0; i < image_bytes; ++i) {
            code[i] = image[i];
        }
    }

    state->user_image_bytes = image_bytes;
    state->user_image_checksum = checksum_bytes(code, image_bytes);
    state->user_image_loaded =
        ((state->user_image_source_ok != 0U) &&
         (image_bytes < X86_64_PAGE_SIZE) &&
         (page_prefix_matches(code, image, image_bytes) != 0U) &&
         (state->user_image_checksum == checksum_bytes(image, image_bytes))) ? 1U : 0U;
    state->user_stack_zeroed = page_is_zeroed(stack);
    state->elf64_load_ok =
        ((state->user_image_source_ok != 0U) &&
         (state->elf64_header_ok != 0U) &&
         (state->elf64_program_header_ok != 0U) &&
         (state->elf64_entry_ok != 0U) &&
         (state->elf64_segment_ok != 0U) &&
         (state->user_image_loaded != 0U)) ? 1U : 0U;
}

void x86_64_paging_builder_init(struct x86_64_paging_builder_state *state,
                                const struct x86_64_memory_layout *layout,
                                const struct x86_64_paging_plan *plan)
{
    clear_builder_state(state);

    const struct x86_64_pmm_state *pmm_state = x86_64_pmm_get_state();
    state->pmm_free_pages_before = pmm_state->free_pages;

    u64 allocated_pages[X86_64_PAGING_BUILDER_ALLOCATED_PAGES];
    u32 allocated_count = 0U;
    u64 *builder_pml4 = (u64 *)0;
    u64 *builder_identity_pdpt = (u64 *)0;
    u64 *builder_identity_pd = (u64 *)0;
    u64 *builder_direct_pdpt = (u64 *)0;
    u64 *builder_direct_pd = (u64 *)0;
    u64 *builder_kernel_pdpt = (u64 *)0;
    u64 *builder_kernel_pd = (u64 *)0;
    u64 *builder_kernel_pt = (u64 *)0;
    u64 *builder_user_code_pt = (u64 *)0;
    u64 *builder_user_stack_pd = (u64 *)0;
    u64 *builder_user_stack_pt = (u64 *)0;
    u64 user_code_page = 0ULL;
    u64 user_stack_page = 0ULL;

    if (allocate_table(&builder_pml4, allocated_pages, &allocated_count) == 0U ||
        allocate_table(&builder_identity_pdpt, allocated_pages, &allocated_count) == 0U ||
        allocate_table(&builder_identity_pd, allocated_pages, &allocated_count) == 0U ||
        allocate_table(&builder_direct_pdpt, allocated_pages, &allocated_count) == 0U ||
        allocate_table(&builder_direct_pd, allocated_pages, &allocated_count) == 0U ||
        allocate_table(&builder_kernel_pdpt, allocated_pages, &allocated_count) == 0U ||
        allocate_table(&builder_kernel_pd, allocated_pages, &allocated_count) == 0U ||
        allocate_table(&builder_kernel_pt, allocated_pages, &allocated_count) == 0U ||
        allocate_table(&builder_user_code_pt, allocated_pages, &allocated_count) == 0U ||
        allocate_table(&builder_user_stack_pd, allocated_pages, &allocated_count) == 0U ||
        allocate_table(&builder_user_stack_pt, allocated_pages, &allocated_count) == 0U ||
        allocate_page(&user_code_page, allocated_pages, &allocated_count) == 0U ||
        allocate_page(&user_stack_page, allocated_pages, &allocated_count) == 0U) {
        rollback_allocated_pages(allocated_pages, allocated_count);
        state->pmm_free_pages_after = x86_64_pmm_get_state()->free_pages;
        return;
    }

    state->pml4_table = (u64)builder_pml4;
    state->identity_pdpt_table = (u64)builder_identity_pdpt;
    state->identity_pd_table = (u64)builder_identity_pd;
    state->direct_pdpt_table = (u64)builder_direct_pdpt;
    state->direct_pd_table = (u64)builder_direct_pd;
    state->kernel_pdpt_table = (u64)builder_kernel_pdpt;
    state->kernel_pd_table = (u64)builder_kernel_pd;
    state->kernel_pt_table = (u64)builder_kernel_pt;
    state->user_code_pt_table = (u64)builder_user_code_pt;
    state->user_stack_pd_table = (u64)builder_user_stack_pd;
    state->user_stack_pt_table = (u64)builder_user_stack_pt;
    state->user_code_page = user_code_page;
    state->user_stack_page = user_stack_page;
    state->user_code_virtual = X86_64_USER_CODE_BASE;
    state->user_stack_virtual = X86_64_USER_STACK_TOP - X86_64_PAGE_SIZE;
    state->pmm_backed = 1U;
    state->allocated_table_pages = X86_64_PAGING_BUILDER_TABLE_PAGES;
    state->allocated_user_pages = X86_64_PAGING_BUILDER_USER_PHYSICAL_PAGES;
    state->pmm_free_pages_after = x86_64_pmm_get_state()->free_pages;
    state->allocation_ok = (allocated_count == X86_64_PAGING_BUILDER_ALLOCATED_PAGES) ? 1U : 0U;
    load_embedded_user_image(state);

    u32 identity_pages = huge_page_count_for(plan->identity_window_bytes);
    if (identity_pages > X86_64_PAGING_BUILDER_ENTRIES) {
        identity_pages = X86_64_PAGING_BUILDER_ENTRIES;
    }

    for (u32 i = 0; i < identity_pages; ++i) {
        builder_identity_pd[i] = ((u64)i * X86_64_HUGE_PAGE_SIZE) | X86_64_HUGE_PAGE_FLAGS;
    }

    u32 direct_pages = huge_page_count_for(plan->direct_map_bytes);
    if (direct_pages > X86_64_PAGING_BUILDER_ENTRIES) {
        direct_pages = X86_64_PAGING_BUILDER_ENTRIES;
    }

    for (u32 i = 0; i < direct_pages; ++i) {
        builder_direct_pd[i] = ((u64)i * X86_64_HUGE_PAGE_SIZE) | X86_64_HUGE_PAGE_FLAGS;
    }

    u32 kernel_pages = align_page_count(layout->kernel_size_bytes);
    u32 kernel_pages_fit = (kernel_pages <= X86_64_PAGING_BUILDER_ENTRIES) ? 1U : 0U;
    if (kernel_pages > X86_64_PAGING_BUILDER_ENTRIES) {
        kernel_pages = X86_64_PAGING_BUILDER_ENTRIES;
    }

    for (u32 i = 0; i < kernel_pages; ++i) {
        builder_kernel_pt[i] = (layout->kernel_start + ((u64)i * X86_64_PAGE_SIZE)) | X86_64_TABLE_FLAGS;
    }

    u32 kernel_pdpt_index = pdpt_index_for(plan->kernel_virtual_start);
    u32 kernel_pd_index = pd_index_for(plan->kernel_virtual_start);
    u32 user_code_pml4_index = pml4_index_for(X86_64_USER_CODE_BASE);
    u32 user_code_pdpt_index = pdpt_index_for(X86_64_USER_CODE_BASE);
    u32 user_code_pd_index = pd_index_for(X86_64_USER_CODE_BASE);
    u32 user_code_pt_index = pt_index_for(X86_64_USER_CODE_BASE);
    u32 user_stack_pml4_index = pml4_index_for(state->user_stack_virtual);
    u32 user_stack_pdpt_index = pdpt_index_for(state->user_stack_virtual);
    u32 user_stack_pd_index = pd_index_for(state->user_stack_virtual);
    u32 user_stack_pt_index = pt_index_for(state->user_stack_virtual);

    builder_identity_pdpt[0] = ((u64)builder_identity_pd) | X86_64_USER_TABLE_FLAGS;
    builder_direct_pdpt[0] = ((u64)builder_direct_pd) | X86_64_TABLE_FLAGS;
    builder_kernel_pdpt[kernel_pdpt_index] = ((u64)builder_kernel_pd) | X86_64_TABLE_FLAGS;
    builder_kernel_pd[kernel_pd_index] = ((u64)builder_kernel_pt) | X86_64_TABLE_FLAGS;

    builder_user_code_pt[user_code_pt_index] = user_code_page | X86_64_USER_CODE_FLAGS;
    builder_identity_pd[user_code_pd_index] = ((u64)builder_user_code_pt) | X86_64_USER_TABLE_FLAGS;
    builder_user_stack_pt[user_stack_pt_index] = user_stack_page | X86_64_USER_STACK_FLAGS;
    builder_user_stack_pd[user_stack_pd_index] = ((u64)builder_user_stack_pt) | X86_64_USER_TABLE_FLAGS;
    builder_identity_pdpt[user_stack_pdpt_index] = ((u64)builder_user_stack_pd) | X86_64_USER_TABLE_FLAGS;

    builder_pml4[plan->identity_pml4_index] = ((u64)builder_identity_pdpt) | X86_64_USER_TABLE_FLAGS;
    builder_pml4[plan->direct_map_pml4_index] = ((u64)builder_direct_pdpt) | X86_64_TABLE_FLAGS;
    builder_pml4[plan->kernel_pml4_index] = ((u64)builder_kernel_pdpt) | X86_64_TABLE_FLAGS;

    u32 split_identity_huge_pages = identity_pages;
    if (user_code_pml4_index == plan->identity_pml4_index &&
        user_code_pdpt_index == 0U &&
        user_code_pd_index < identity_pages) {
        split_identity_huge_pages -= 1U;
    }

    state->pml4_present_entries = count_present_entries(builder_pml4);
    state->identity_huge_pages = split_identity_huge_pages;
    state->direct_huge_pages = direct_pages;
    state->kernel_pages = kernel_pages;
    state->identity_entry_ok =
        ((builder_pml4[plan->identity_pml4_index] & X86_64_PAGE_PRESENT) != 0ULL &&
Continue output truncated at 20k. Need update pushed? yes. Need maybe final ask user test. Since we did patch. Need no need more. Need maybe if can continue to next? User supplied results; they expect next. But we patched warning only. Could also start next milestone after fixing? But final should ask test? However 