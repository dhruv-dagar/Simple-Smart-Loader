# Simple Smart Loader

A user-space ELF32 loader implemented in C for Linux. The project demonstrates lazy, demand-paged loading of executable segments using `SIGSEGV`, `mmap`, and `mprotect`.

## Highlights

- Parses ELF32 headers and program headers
- Uses page faults to lazily load executable segments
- Maps memory one 4 KiB page at a time
- Handles file-backed data and zero-filled BSS pages
- Applies ELF segment permissions with `mprotect`
- Tracks page faults, page allocations, memory usage, and internal fragmentation
- Includes minimal ELF32 test programs for validation

## Build

This project targets a 32-bit Linux userspace environment and requires multilib support.

```bash
make
```

## Run

```bash
./launch ./fib
./launch ./helloworld
./launch ./sum
```

## Architecture

`launch.c` validates the ELF input and invokes the loader. `loader.c` parses the ELF image and installs a `SIGSEGV` handler. When execution touches an unmapped page, the handler identifies the corresponding loadable segment, maps a page, copies the required file-backed bytes, zero-fills the remaining portion when needed, and applies the segment's protection flags.

See [`ARCHITECTURE.md`](ARCHITECTURE.md) for the detailed execution flow and design trade-offs.

## Educational focus

This project is a user-space demonstration of demand paging and ELF loading. It demonstrates virtual memory, page faults, executable formats, memory mapping, and protection rather than replacing the Linux kernel's ELF loader.

## Project structure

```text
Simple-Smart-Loader/
├── README.md
├── ARCHITECTURE.md
├── PORTFOLIO.md
├── Makefile
├── .gitignore
├── loader.c
├── loader.h
├── launch.c
├── fib.c
├── helloworld.c
└── sum.c
```

## Limitations

- Supports ELF32 little-endian executables.
- Designed for Linux environments with 32-bit compilation support.
- Uses an educational SIGSEGV-based implementation rather than kernel-level page-fault handling.
- Keeps the executable image in a userspace buffer to simplify page population.

## Author

Dhruv Dagar
