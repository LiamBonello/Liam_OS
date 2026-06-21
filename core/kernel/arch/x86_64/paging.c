#include "paging.h"

#include "memory_layout.h"

#define X86_64_PAGE_PRESENT 0x001ULL
#define X86_64_PAGE_HUGE 0x080ULL

extern u64 x86_64_boot_pml4_table[];
extern u64 x86_64_boot_pdpt_table[];
extern u64 x86_64_boot_pd_table[];

static u64 read_cr3(void)
{
    u64 value;
    __asm__ volatile ("mov %%cr3, %0" : "=r" (value));
    return value;
}

static u64 huge_page_base(u64 entry)
{
    return entry & ~(X86_64_HUGE_PAGE_SIZE - 1ULL);
}

void x86_64_paging_state_init(struct x86_64_paging_state *state)
{
    state->cr3 = read_cr3();
    state->pml4_table = (u64)x86_64_boot_pml4_table;
    state->pdpt_table = (u64)x86_64_boot_pdpt_table;
    state->pd_table = (u64)x86_64_boot_pd_table;
    state->identity_map_bytes = X86_64_BOOTSTRAP_IDENTITY_MAP_BYTES;
    state->pml4_present = (x86_64_boot_pml4_table[0] & X86_64_PAGE_PRESENT) != 0ULL;
    state->pdpt_present = (x86_64_boot_pdpt_table[0] & X86_64_PAGE_PRESENT) != 0ULL;
    state->huge_pages_present = 0U;
    state->first_huge_page = 0ULL;
    state->last_huge_page = 0ULL;

    for (u32 i = 0; i < X86_64_BOOTSTRAP_PAGING_ENTRIES; ++i) {
        u64 entry = x86_64_boot_pd_table[i];
        if ((entry & (X86_64_PAGE_PRESENT | X86_64_PAGE_HUGE)) !=
            (X86_64_PAGE_PRESENT | X86_64_PAGE_HUGE)) {
            continue;
        }

        u64 base = huge_page_base(entry);
        if (state->huge_pages_present == 0U) {
            state->first_huge_page = base;
        }

        state->last_huge_page = base;
        ++state->huge_pages_present;
    }
}
