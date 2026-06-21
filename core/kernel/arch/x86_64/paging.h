#ifndef LIAM_OS_X86_64_PAGING_H
#define LIAM_OS_X86_64_PAGING_H

#include "types.h"

#define X86_64_BOOTSTRAP_PAGING_ENTRIES 512U

struct x86_64_paging_state {
    u64 cr3;
    u64 pml4_table;
    u64 pdpt_table;
    u64 pd_table;
    u64 identity_map_bytes;
    u32 pml4_present;
    u32 pdpt_present;
    u32 huge_pages_present;
    u64 first_huge_page;
    u64 last_huge_page;
};

void x86_64_paging_state_init(struct x86_64_paging_state *state);

#endif
