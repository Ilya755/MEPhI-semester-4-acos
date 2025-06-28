#include <kern/sysgate.h>
#include <kern/syscall.h>

void _start_user();
void _syscall_enter();

long read_msr(int msr) {
    long ans;
    asm volatile (
            "rdmsr\n"
            "shl $32, %%rdx\n"
            "or %%rdx, %%rax"
            : "=a" (ans)
            : "c" (msr)
            : "rdx", "memory"
            );
    return ans;
}

void write_msr(int msr, long value) {
    asm volatile (
            "wrmsr\n"
            :
            : "c" (msr), "a"(value), "d"(value >> 32)
            : "memory"
            );
}

void sysgate() {
    write_msr(IA32_EFER, read_msr(IA32_EFER) | 1);
    write_msr(IA32_LSTAR, (long)&_syscall_enter);
    asm volatile(
            "mov $_start_user, %%rcx\n"
            "sysretq\n"
            :
            :
            : "rcx"
            );
}
