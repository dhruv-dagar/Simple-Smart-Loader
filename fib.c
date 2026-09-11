// Minimal 32-bit ELF test program for the loader.
int fib(int n) {
    if (n <= 1) return n;
    return fib(n - 1) + fib(n - 2);
}

int _start() {
    return fib(10);
}
