#include "syscall.h"

int open(const char* pathname, int flags) {
    int fd;
    asm ( "mov $2, %%eax\n"
          "mov %1, %%rdi\n"
          "mov %2, %%esi\n"
          "syscall\n"
          "mov %%eax, %0"
          : "=r"(fd)
          : "r"(pathname), "r"(flags)
          : "eax", "rdi", "esi"
    );
    return fd;
}

int close(int fd) {
    int status;
    asm ( "mov $3, %%eax\n"
          "mov %1, %%edi\n"
          "syscall\n"
          "mov %%eax, %0"
          : "=r"(status)
          : "r"(fd)
          : "eax", "edi"
    );
    return status;
}

ssize_t read(int fd, void* buf, size_t count) {
    ssize_t bytes_read;
    asm ( "mov $0, %%rax\n"
          "mov %1, %%edi\n"
          "mov %2, %%rsi\n"
          "mov %3, %%rdx\n"
          "syscall\n"
          "mov %%rax, %0"
          : "=r"(bytes_read)
          : "r"(fd), "r"(buf), "r"(count)
          : "rax", "edi", "rsi", "rdx"
    );
    return bytes_read;
}

ssize_t write(int fd, const void* buf, ssize_t count) {
    ssize_t bytes_written;
    asm ( "mov $1, %%rax\n"
          "mov %1, %%edi\n"
          "mov %2, %%rsi\n"
          "mov %3, %%rdx\n"
          "syscall\n"
          "mov %%rax, %0"
          : "=r"(bytes_written)
          : "r"(fd), "r"(buf), "r"(count)
          : "rax", "edi", "rsi", "rdx"
    );
    return bytes_written;
}

int pipe(int pipefd[2]) {
    int status;
    asm ( "mov $22, %%eax\n"
          "mov %1, %%rdi\n"
          "syscall\n"
          "mov %%eax, %0"
          : "=r"(status)
          : "r"(pipefd)
          : "eax", "rdi"
    );
    return status;
}

int dup(int oldfd) {
    int newfd;
    asm ( "mov $32, %%rax\n"
          "mov %1, %%edi\n"
          "syscall\n"
          "mov %%eax, %0"
          : "=r"(newfd)
          : "r"(oldfd)
          : "eax", "edi"
    );
    return newfd;
}

pid_t fork() {
    pid_t pid;
    asm ( "mov $57, %%eax\n"
          "syscall\n"
          "mov %%eax, %0"
          : "=r"(pid)
          :
          : "eax"
    );
    return pid;
}

pid_t waitpid(pid_t pid, int* wstatus, int options) {
    pid_t status;
    asm ( "mov $61, %%eax\n"
          "mov %1, %%edi\n"
          "mov %2, %%rsi\n"
          "mov %3, %%edx\n"
          "syscall\n"
          "mov %%eax, %0"
          : "=r" (status)
          : "r" (pid), "r" (wstatus), "r" (options)
          : "eax", "edi", "rsi", "edx"
    );
    return status;
}

int execve(const char* filename, char* const argv[], char* const envp[]) {
    int status;
    asm ( "mov $59, %%eax\n"
          "mov %1, %%rdi\n"
          "mov %2, %%rsi\n"
          "mov %3, %%rdx\n"
          "syscall\n"
          "mov %%eax, %0"
          : "=r" (status)
          : "r" (filename), "r" (argv), "r" (envp)
          : "eax", "rdi", "rsi", "rdx"
    );
    return status;
}

void exit(int status) {
    asm ( "mov $60, %%rax\n"
          "mov %0, %%edi\n"
          "syscall"
          :
          : "r"(status)
          : "rax", "edi"
    );
}