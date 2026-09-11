# Portfolio Notes

## Resume bullet

Implemented a user-space ELF32 loader in C on Linux using SIGSEGV-driven demand paging; parsed ELF program headers, lazily mapped 4 KiB pages with mmap, populated file-backed/BSS regions, enforced segment permissions with mprotect, and tracked page-fault and memory-allocation statistics.

## Core OS concepts

- ELF executable loading
- Virtual memory and address spaces
- Demand paging
- Page faults and signal handling
- Memory mapping with mmap
- Segment permissions with mprotect
- BSS initialization
- Internal fragmentation and memory accounting

## Interview talking points

1. Why demand paging reduces unnecessary upfront mapping.
2. How an ELF program header describes a loadable segment.
3. How a fault address is aligned to a page boundary.
4. The difference between `p_filesz` and `p_memsz`.
5. Why BSS must be zero initialized.
6. Why segment permissions are applied after page population.
7. Why a production implementation would move fault handling into the kernel and avoid non-async-signal-safe operations in a signal handler.
