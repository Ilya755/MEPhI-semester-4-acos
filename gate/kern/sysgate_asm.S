.global _syscall_enter

end_syscall:
    movq $5, %rax
    popq %r11
    popq %rcx
    sysretq

call_sys_work:
    movq %rsi, %rdi
    call sys_work
    jmp end_syscall

call_sys_retire:
    call sys_retire
    jmp end_syscall

_syscall_enter:
    pushq %rcx
    pushq %r11
    cmp $1, %edi
    je call_sys_work
    jmp call_sys_retire