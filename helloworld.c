static const char message[] = "Hello, World!\n";

int _start() {
    __asm__ volatile (
        "movl $4, %%eax\n"
        "movl $1, %%ebx\n"
        "movl $message, %%ecx\n"
        "movl $14, %%edx\n"
        "int $0x80\n"
        :
        : "S"(message)
        : "eax", "ebx", "ecx", "edx"
    );
    return 0x7F;
}
