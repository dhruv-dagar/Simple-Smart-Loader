CC := gcc
CFLAGS := -Wall -Wextra -m32
PIC_CFLAGS := -Wall -Wextra -m32 -fPIC

LOADER_LIB := lib_simpleloader.so
LAUNCH := launch
TESTS := fib helloworld sum

.PHONY: all clean tests

all: $(LOADER_LIB) $(LAUNCH) $(TESTS)

$(LOADER_LIB): loader.c loader.h
	$(CC) $(PIC_CFLAGS) -shared -o $@ loader.c

$(LAUNCH): launch.c loader.h $(LOADER_LIB)
	$(CC) $(CFLAGS) -I. -Wl,-rpath,'$$ORIGIN' -o $@ launch.c -L. -l_simpleloader -ldl

fib: fib.c
	$(CC) $(CFLAGS) -nostdlib -o $@ $<

helloworld: helloworld.c
	$(CC) $(CFLAGS) -nostdlib -o $@ $<

sum: sum.c
	$(CC) $(CFLAGS) -nostdlib -o $@ $<

tests: $(TESTS)
	@echo "Built test ELF32 binaries: $(TESTS)"

clean:
	rm -f $(LOADER_LIB) $(LAUNCH) $(TESTS)
