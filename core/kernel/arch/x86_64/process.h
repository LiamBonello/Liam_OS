#ifndef LIAM_OS_X86_64_PROCESS_H
#define LIAM_OS_X86_64_PROCESS_H

#include "paging_builder.h"
#include "types.h"

#define X86_64_PROCESS_MAX_PROCESSES 64U
#define X86_64_PROCESS_NAME_LEN 24U
#define X86_64_PROCESS_IMAGE_PATH_LEN 64U
#define X86_64_PROCESS_KERNEL_STACK_BYTES 4096ULL
#define X86_64_PROCESS_ADDRESS_SPACE_PAGES 8U
#define X86_64_PROCESS_CHILD_STATUS_QUEUE 16U

typedef void (*x86_64_process_entry_t)(void *arg);

enum x86_64_process_state_code {
    X86_64_PROCESS_UNUSED = 0,
    X86_64_PROCESS_READY = 1,
    X86_64_PROCESS_RUNNING = 2,
    X86_64_PROCESS_EXITED = 3,
    X86_64_PROCESS_FAILED = 4
};

enum x86_64_process_mode_code {
    X86_64_PROCESS_MODE_NONE = 0,
    X86_64_PROCESS_MODE_KERNEL = 1,
    X86_64_PROCESS_MODE_USER = 2
};

struct x86_64_process {
    u32 pid;
    u32 state;
    u32 mode;
    u32 image_bytes;
    char name[X86_64_PROCESS_NAME_LEN];
    char image_path[X86_64_PROCESS_IMAGE_PATH_LEN];
    x86_64_process_entry_t entry;
    void *arg;
    u64 kernel_stack_page;
    u64 kernel_stack_base;
    u64 kernel_stack_top;
    u64 address_space_id;
    u64 cr3;
    u64 user_pdpt_page;
    u64 user_code_pd_page;
    u64 user_stack_pd_page;
    u64 user_code_pt_page;
    u64 user_stack_pt_page;
    u64 user_code_page;
    u64 user_stack_page;
    u64 user_code_virtual;
    u64 user_stack_virtual;
    u64 user_entry;
    u32 address_space_owned;
    u32 user_page_tables_ready;
    u32 kernel_mappings_ready;
    u32 exit_code;
};

struct x86_64_user_schedule_state {
    u32 initialized;
    u32 candidate_found;
    u32 selected_pid;
    u32 selected_state;
    u32 selected_mode;
    u32 entry_valid;
    u32 stack_valid;
    u32 cr3_valid;
    u32 code_page_valid;
    u32 stack_page_valid;
    u32 page_tables_valid;
    u32 kernel_mappings_valid;
    u32 transition_ready;
    u32 scheduler_ok;
    u64 address_space_id;
    u64 cr3;
    u64 user_entry;
    u64 user_rsp;
    u64 user_rflags;
    u64 user_code_page;
    u64 user_stack_page;
};

struct x86_64_process_smoke_state {
    u32 initialized;
    u32 table_capacity;
    u32 created_processes;
    u32 ready_before_run;
    u32 run_attempts;
    u32 run_count;
    u32 exited_processes;
    u32 failed_processes;
    u32 stack_allocations;
    u32 stack_alignment_ok;
    u32 stack_switches;
    u32 stack_execution_ok;
    u32 address_space_allocations;
    u32 address_space_pages_allocated;
    u32 address_spaces_distinct;
    u32 address_space_ok;
    u32 kernel_mapping_source_ready;
    u32 kernel_mappings_ready;
    u32 user_page_tables_ready;
    u32 user_page_table_entries_ok;
    u32 user_processes_created;
    u32 user_processes_exited;
    u32 user_processes_reaped;
    u32 reap_failures;
    u32 completed_child_records;
    u32 completed_child_waits;
    u32 wait_misses;
    u32 completed_child_queue_depth;
    u32 completed_child_queue_high_watermark;
    u32 completed_child_drops;
    u32 user_image_bytes;
    u32 user_image_copied;
    u32 user_process_ready;
    u32 user_scheduler_ready;
    u32 last_created_pid;
    u32 last_user_pid;
    u32 last_exited_user_pid;
    u32 last_reaped_pid;
    u32 last_completed_parent_pid;
    u32 last_completed_child_pid;
    u32 last_completed_exit_code;
    u32 last_wait_parent_pid;
    u32 last_wait_child_pid;
    u32 last_wait_exit_code;
    u32 last_scheduled_user_pid;
    u32 last_run_pid;
    u32 worker_a_count;
    u32 worker_b_count;
    u32 userland_foundation_ok;
    u32 syscall_dispatcher_ok;
    u32 user_context_ok;
    u32 smoke_ok;
    u64 first_stack_base;
    u64 first_stack_top;
    u64 second_stack_base;
    u64 second_stack_top;
    u64 first_address_space_id;
    u64 second_address_space_id;
    u64 first_cr3;
    u64 second_cr3;
    u64 first_user_code_page;
    u64 second_user_code_page;
    u64 first_user_stack_page;
    u64 second_user_stack_page;
    u64 last_user_entry;
    u64 last_user_cr3;
    u64 last_user_pdpt_page;
    u64 last_user_code_pd_page;
    u64 last_user_stack_pd_page;
    u64 last_user_code_pt_page;
    u64 last_user_stack_pt_page;
    u64 last_user_code_page;
    u64 last_user_stack_page;
    u64 scheduled_user_rsp;
    u64 scheduled_user_rflags;
    u64 worker_a_stack_sample;
    u64 worker_b_stack_sample;
};

void x86_64_process_set_paging_context(const struct x86_64_paging_builder_state *paging_builder);
void x86_64_process_initialize(struct x86_64_process_smoke_state *state);
u32 x86_64_process_create(const char *name,
                          x86_64_process_entry_t entry,
                          void *arg);
u32 x86_64_process_create_user_image(const char *path,
                                      const u8 *loaded_code_page,
                                      u64 code_bytes,
                                      u64 entry);
u32 x86_64_process_mark_user_exited(u32 pid, u32 exit_code);
u32 x86_64_process_reap_user(u32 pid);
u32 x86_64_process_reap_exited_user_processes(void);
u32 x86_64_process_record_child_exit(u32 parent_pid, u32 child_pid, u32 exit_code);
u32 x86_64_process_wait_child(u32 parent_pid, u32 *child_pid_out, u32 *exit_code_out);
u64 x86_64_process_snapshot(char *buffer, u64 size);
u32 x86_64_process_prepare_next_user(struct x86_64_user_schedule_state *state);
u32 x86_64_process_run_next_ready(void);
u32 x86_64_process_run_all_ready(u32 max_runs);
void x86_64_process_run_smoke(struct x86_64_process_smoke_state *state);
const struct x86_64_process_smoke_state *x86_64_process_get_smoke_state(void);

#endif
