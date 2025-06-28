#include "backtrace.h"

#include <elf.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>

off_t CreateElfFile(char** addr) {
    int fd = open("/proc/self/exe", O_RDONLY);
    if (fd == -1) {
        perror("open failed");
        return 0;
    }
    struct stat st;
    if (fstat(fd, &st) == -1) {
        perror("fstat failed");
        close(fd);
        return 0;
    }
    *addr = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    close(fd);
    if (*addr == MAP_FAILED) {
        perror("mmap failed");
        return 0;
    }
    return st.st_size;
}

void FreeElfFile(void* addr, size_t size) {
    if (munmap(addr, size) == -1) {
        perror("munmap failed");
    }
}

char* AddrToName(void* addr) {
    char* elf_start;
    off_t size = CreateElfFile(&elf_start);
    if (!elf_start) {
        perror("CreateElfFile failed");
        return NULL;
    }

    Elf64_Ehdr* elf_hdr = (Elf64_Ehdr*)elf_start;
    Elf64_Shdr* sec_hdr = (Elf64_Shdr*)((char*)elf_start + elf_hdr->e_shoff);

    Elf64_Shdr* symtab = NULL;
    Elf64_Shdr* strtab = NULL;
    for (size_t i = 0; i < (size_t) elf_hdr->e_shnum; ++i) {
        if (sec_hdr[i].sh_type == SHT_SYMTAB) {
            symtab = &sec_hdr[i];
            strtab = &sec_hdr[symtab->sh_link];
            break;
        }
    }

    if (!symtab || !strtab) {
        FreeElfFile(elf_start, size);
        return NULL;
    }

    Elf64_Sym* sym = (Elf64_Sym*)((char*)elf_start + symtab->sh_offset);
    const char* strings = (const char*)elf_start + strtab->sh_offset;

    for (size_t i = 0; i < (size_t) symtab->sh_size / sizeof(Elf64_Sym); ++i) {
        if (ELF64_ST_TYPE(sym[i].st_info) == STT_FUNC &&
            sym[i].st_value <= (Elf64_Addr) addr &&
            sym[i].st_value + sym[i].st_size > (Elf64_Addr) addr) {
            return (char*) strings + sym[i].st_name;
        }
    }
    FreeElfFile(elf_start, size);
    return NULL;
}

int Backtrace(void* backtrace[], int limit) {
    long long cnt = 0;
    long long lim = (long long) limit;
    asm volatile (
            "test %1, %1\n"
            "jz end\n"

            "mov %%rbp, %%rax\n"

            "loop:\n"
            "cmp $0, 0(%%rax)\n"
            "je end\n"

            "mov 8(%%rax), %%rcx\n"
            "mov %%rcx, (%1, %0, 8)\n"
            "inc %0\n"
            "cmp %2, %0\n"
            "jge end\n"

            "mov 0(%%rax), %%rax\n"
            "jmp loop\n"

            "end:\n"
            : "+r" (cnt)
            : "r" (backtrace), "r" (lim)
            : "rax", "rcx", "memory"
            );
    return (int) cnt;
}

void PrintBt() {
    void* buffer[256];
    int func_cnt = Backtrace(buffer, 256);

    for (int i = 0; i < func_cnt; ++i) {
        char* func_name = AddrToName(buffer[i]);
        printf("%p: %s\n", buffer[i], func_name);
    }
}
