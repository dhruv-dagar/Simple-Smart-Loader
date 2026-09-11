# Simple Smart Loader — Architecture

## Overview

Simple Smart Loader is a user-space ELF32 loader for Linux. Instead of loading all executable segments before execution, it relies on page faults to load individual pages on demand.

## Execution flow

```text
launch.c -> validate ELF -> loader.c -> parse ELF32 -> install SIGSEGV
                                                |
                                                v
                                         jump to e_entry
                                                |
                                                v
                                         _start() executes
                                                |
                                         unmapped page access
                                                |
                                                v
                                         SIGSEGV handler
                                                |
                                   locate PT_LOAD + align page
                                                |
                                                v
                                             mmap()
                                                |
                                   copy file data / zero BSS
                                                |
                                                v
                                           mprotect()
                                                |
                                                v
                                      resume execution
```

## Page-fault handling

1. Read the fault address from `siginfo_t`.
2. Align it to a 4096-byte page boundary.
3. Locate the loadable ELF segment containing the address.
4. Map one page with `mmap(..., MAP_FIXED, ...)`.
5. Copy file-backed bytes from the ELF image.
6. Zero-fill BSS bytes where `p_memsz > p_filesz`.
7. Apply `PF_R`, `PF_W`, and `PF_X` using `mprotect()`.
8. Return from the handler and resume execution.

## Memory accounting

The loader tracks page faults, page allocations, allocated virtual memory, bytes copied from the executable image, and approximate internal fragmentation.

## OS concepts demonstrated

- ELF executable format parsing
- Virtual address spaces
- Page alignment
- Demand paging
- Page-fault-driven memory population
- File-backed memory
- BSS initialization
- Memory protection
- Low-level process startup

## Design trade-offs

The implementation intentionally keeps the complete ELF file in a userspace buffer so page population is easy to observe. A production kernel loader would coordinate virtual memory, file mappings, permissions, process state, and fault handling inside the kernel.

The educational SIGSEGV handler also performs operations such as allocation and formatted output that are not generally appropriate for production signal handlers.
