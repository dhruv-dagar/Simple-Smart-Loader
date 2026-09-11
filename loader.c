// Simple Smart Loader - ELF32 demand-paged userspace loader
#include "loader.h"
#include <stdint.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/param.h>

#define PAGE_SIZE 4096

Elf32_Ehdr *ehdr = NULL;
Elf32_Phdr *phdr = NULL;
int fd = -1;
void *file_buf = NULL;
off_t file_size = 0;

int no_page_fault = 0;
int no_page_allocation = 0;
size_t total_mem_allocated = 0;
size_t total_mem_used = 0;

void **allocated_pages = NULL;
size_t allocated_pages_count = 0;
size_t allocated_pages_capacity = 0;

static void add_allocated_page(void *addr) {
    if (allocated_pages_count == allocated_pages_capacity) {
        size_t newcap = allocated_pages_capacity ? allocated_pages_capacity * 2 : 64;
        void **tmp = realloc(allocated_pages, newcap * sizeof(void *));
        if (!tmp) return;
        allocated_pages = tmp;
        allocated_pages_capacity = newcap;
    }
    allocated_pages[allocated_pages_count++] = addr;
}

static int is_page_allocated(void *addr) {
    for (size_t i = 0; i < allocated_pages_count; ++i)
        if (allocated_pages[i] == addr) return 1;
    return 0;
}

void loader_cleanup();

static int read_entire_file(void) {
    if (fd < 0) return 0;
    file_size = lseek(fd, 0, SEEK_END);
    if (file_size == -1) return 0;
    if (lseek(fd, 0, SEEK_SET) == -1) return 0;
    file_buf = malloc((size_t)file_size);
    if (!file_buf) return 0;

    ssize_t total = 0;
    while (total < file_size) {
        ssize_t r = read(fd, (char *)file_buf + total, file_size - total);
        if (r < 0) {
            if (errno == EINTR) continue;
            free(file_buf); file_buf = NULL; return 0;
        }
        if (r == 0) break;
        total += r;
    }
    return total == file_size;
}

void segfault_handler(int signo, siginfo_t *info, void *context) {
    (void)signo; (void)context;
    no_page_fault++;

    void *fault_addr = info->si_addr;
    uintptr_t fault_page_addr_u = (uintptr_t)fault_addr & ~(PAGE_SIZE - 1);
    void *fault_page_addr = (void *)fault_page_addr_u;

    if (is_page_allocated(fault_page_addr)) {
        fprintf(stderr, "Repeated page fault at already-allocated page %p - aborting\n", fault_page_addr);
        loader_cleanup();
        _exit(1);
    }

    Elf32_Phdr *target_phdr = NULL;
    for (int i = 0; i < ehdr->e_phnum; ++i) {
        if (phdr[i].p_type != PT_LOAD) continue;
        uintptr_t seg_vaddr = (uintptr_t)phdr[i].p_vaddr;
        uintptr_t seg_vend = seg_vaddr + phdr[i].p_memsz;
        if ((uintptr_t)fault_addr >= seg_vaddr && (uintptr_t)fault_addr < seg_vend) {
            target_phdr = &phdr[i];
            break;
        }
    }

    if (!target_phdr) {
        fprintf(stderr, "Segfault at %p not in any PT_LOAD segment\n", fault_addr);
        loader_cleanup(); _exit(1);
    }

    uintptr_t seg_base = (uintptr_t)target_phdr->p_vaddr;
    uintptr_t page_offset_in_seg = fault_page_addr_u - seg_base;
    if (page_offset_in_seg >= (uintptr_t)target_phdr->p_memsz) {
        fprintf(stderr, "Fault page beyond segment memsz\n");
        loader_cleanup(); _exit(1);
    }

    void *mapped = mmap(fault_page_addr, PAGE_SIZE,
                        PROT_READ | PROT_WRITE | PROT_EXEC,
                        MAP_ANONYMOUS | MAP_PRIVATE | MAP_FIXED, -1, 0);
    if (mapped == MAP_FAILED) {
        perror("mmap failed in segfault handler");
        loader_cleanup(); _exit(1);
    }

    add_allocated_page(fault_page_addr);
    no_page_allocation++;
    total_mem_allocated += PAGE_SIZE;

    uintptr_t file_backed_bytes = 0;
    if (page_offset_in_seg < (uintptr_t)target_phdr->p_filesz) {
        uintptr_t remaining = (uintptr_t)target_phdr->p_filesz - page_offset_in_seg;
        file_backed_bytes = remaining >= PAGE_SIZE ? PAGE_SIZE : remaining;
    }

    if (file_backed_bytes > 0) {
        size_t copy_src_offset = (size_t)target_phdr->p_offset + (size_t)page_offset_in_seg;
        if (copy_src_offset + file_backed_bytes <= (size_t)file_size) {
            memcpy(mapped, (char *)file_buf + copy_src_offset, file_backed_bytes);
            total_mem_used += file_backed_bytes;
        } else {
            size_t safe_bytes = copy_src_offset < (size_t)file_size ? (size_t)file_size - copy_src_offset : 0;
            if (safe_bytes > 0) {
                memcpy(mapped, (char *)file_buf + copy_src_offset, safe_bytes);
                total_mem_used += safe_bytes;
            }
            if (file_backed_bytes > safe_bytes)
                memset((char *)mapped + safe_bytes, 0, file_backed_bytes - safe_bytes);
        }
    }

    if (file_backed_bytes < PAGE_SIZE) {
        uintptr_t seg_remaining = (uintptr_t)target_phdr->p_memsz - page_offset_in_seg;
        uintptr_t bytes_in_seg_page = seg_remaining < PAGE_SIZE ? seg_remaining : PAGE_SIZE;
        if (bytes_in_seg_page > file_backed_bytes)
            memset((char *)mapped + file_backed_bytes, 0,
                   (size_t)(bytes_in_seg_page - file_backed_bytes));
    }

    int prot = 0;
    if (target_phdr->p_flags & PF_R) prot |= PROT_READ;
    if (target_phdr->p_flags & PF_W) prot |= PROT_WRITE;
    if (target_phdr->p_flags & PF_X) prot |= PROT_EXEC;
    if (mprotect(fault_page_addr, PAGE_SIZE, prot) == -1)
        perror("mprotect warning");
}

void loader_cleanup() {
    if (file_buf) { free(file_buf); file_buf = NULL; }
    if (fd != -1) { close(fd); fd = -1; }
    if (allocated_pages) {
        free(allocated_pages);
        allocated_pages = NULL;
        allocated_pages_count = 0;
        allocated_pages_capacity = 0;
    }
}

void load_and_run_elf(const char *exe) {
    fd = open(exe, O_RDONLY);
    if (fd == -1) { perror("open ELF"); return; }

    if (!read_entire_file()) {
        fprintf(stderr, "Failed to read ELF file into memory\n");
        loader_cleanup(); return;
    }

    if ((size_t)file_size < sizeof(Elf32_Ehdr)) {
        fprintf(stderr, "File too small to be ELF\n");
        loader_cleanup(); return;
    }
    ehdr = (Elf32_Ehdr *)file_buf;

    if (!(ehdr->e_ident[EI_MAG0] == ELFMAG0 &&
          ehdr->e_ident[EI_MAG1] == ELFMAG1 &&
          ehdr->e_ident[EI_MAG2] == ELFMAG2 &&
          ehdr->e_ident[EI_MAG3] == ELFMAG3)) {
        fprintf(stderr, "Not a valid ELF file\n");
        loader_cleanup(); return;
    }

    if (ehdr->e_ident[EI_CLASS] != ELFCLASS32 || ehdr->e_ident[EI_DATA] != ELFDATA2LSB) {
        fprintf(stderr, "Unsupported ELF format\n");
        loader_cleanup(); return;
    }

    phdr = (Elf32_Phdr *)((char *)file_buf + ehdr->e_phoff);

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_flags = SA_SIGINFO | SA_NODEFER;
    sa.sa_sigaction = segfault_handler;
    if (sigaction(SIGSEGV, &sa, NULL) == -1) {
        perror("sigaction"); loader_cleanup(); return;
    }

    int (*_start)() = (int (*)())(intptr_t)ehdr->e_entry;
    int result = _start();

    printf("User _start return value = %d\n", result);
    printf("No of page faults: %d\n", no_page_fault);
    printf("Total memory allocated: %zu\n", total_mem_allocated);
    printf("Total memory used (bytes copied from file): %zu\n", total_mem_used);
    printf("Internal fragmentation: %.2f KB\n", (double)(total_mem_allocated - total_mem_used) / 1024.0);
    printf("Total page allocation: %d\n", no_page_allocation);

    loader_cleanup();
}
