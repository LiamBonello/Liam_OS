#ifndef LIAM_OS_X86_64_HEAP_H
#define LIAM_OS_X86_64_HEAP_H

#include "paging_plan.h"
#include "types.h"

#define X86_64_EARLY_HEAP_PAGES 16U

struct x86_64_heap_state {
    u32 initialized;
    u32 pmm_backed;
    u32 direct_map_ok;
    u32 heap_pages;
    u32 allocations;
    u32 alignment_ok;
    u32 pattern_ok;
    u32 smoke_ok;
    u32 pmm_free_pages_before;
    u32 pmm_free_pages_after;
    u64 heap_bytes;
    u64 used_bytes;
    u64 first_physical_page;
    u64 last_physical_page;
    u64 first_virtual_page;
    u64 last_virtual_page;
    u64 first_allocation;
    u64 second_allocation;
};

void x86_64_heap_init(struct x86_64_heap_state *state,
                      const struct x86_64_paging_plan *plan);
void *x86_64_heap_alloc(usize size, usize alignment);
void x86_64_heap_run_smoke(struct x86_64_heap_state *state);
const struct x86_64_heap_state *x86_64_heap_get_state(void);

#endif
