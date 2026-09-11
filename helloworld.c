// helloworld.c — minimal 32-bit ELF test for SimpleSmartLoader
// Uses raw syscalls (no glibc), writes "Hello, World!\n" to stdout
// and returns to the loader for stats printing.

static const char msg[] = "Hello, World!\n";

int _start() {
    // write(1, msg, sizeof(msg)-1)
    __asm__ volatile (
        "movl $4, %%eax\n\t"
        "movl $1, %%ebx\n\t"
        "movl %0, %%ecx\n\t"
        "movl %1, %%edx\n\t"
        "int $0x80\n\t"
        :
        : "r"(msg), "r"(sizeof(msg) - 1)
        : "%eax", "%ebx", "%ecx", "%edx"
    );

    return 0x7F;
}
