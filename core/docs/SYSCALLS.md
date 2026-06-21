# Liam_OS Core Syscalls

Current i386 syscall entry uses `int 0x80`.

| Number | Name | Purpose |
|---:|---|---|
| 1 | `exit(code)` | Exit the current user process. |
| 2 | `write(buffer, length)` | Write bytes to the console/output path. |
| 3 | `get_ticks()` | Return timer tick count. |
| 4 | `yield()` | Yield execution. |
| 5 | `get_pid()` | Return current process ID. |

This ABI is early and should be treated as unstable until userspace and process isolation mature.
