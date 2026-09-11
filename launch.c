#include "loader.h"

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <ELF Executable>\n", argv[0]);
        return 1;
    }

    int fd = open(argv[1], O_RDONLY);
    if (fd == -1)
    {
        perror("Error: unable to open ELF file");
        return 1;
    }

    off_t size = lseek(fd, 0, SEEK_END);
    if (size < (off_t)sizeof(Elf32_Ehdr))
    {
        fprintf(stderr, "Error: file is too small to contain an ELF32 header\n");
        close(fd);
        return 1;
    }

    if (lseek(fd, 0, SEEK_SET) == (off_t)-1)
    {
        perror("Error: failed to seek ELF file");
        close(fd);
        return 1;
    }

    Elf32_Ehdr header;
    ssize_t bytes_read = read(fd, &header, sizeof(header));
    close(fd);

    if (bytes_read != (ssize_t)sizeof(header))
    {
        fprintf(stderr, "Error: failed to read ELF header\n");
        return 1;
    }

    if (header.e_ident[EI_MAG0] != ELFMAG0 ||
        header.e_ident[EI_MAG1] != ELFMAG1 ||
        header.e_ident[EI_MAG2] != ELFMAG2 ||
        header.e_ident[EI_MAG3] != ELFMAG3)
    {
        fprintf(stderr, "Error: not a valid ELF file\n");
        return 1;
    }

    if (header.e_ident[EI_CLASS] != ELFCLASS32)
    {
        fprintf(stderr, "Error: only ELF32 executables are supported\n");
        return 1;
    }

    if (header.e_ident[EI_DATA] != ELFDATA2LSB)
    {
        fprintf(stderr, "Error: only little-endian ELF files are supported\n");
        return 1;
    }

    load_and_run_elf(argv[1]);
    loader_cleanup();
    return 0;
}
