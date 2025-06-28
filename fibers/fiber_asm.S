.global save_context
.global get_context

save_context:
    lea 8(%rsp), %rax
    mov %rax, 0(%rdi)
    mov 0(%rsp), %rax
    mov %rax, 8(%rdi)
    mov %rbx, 16(%rdi)
    mov %rbp, 24(%rdi)
    mov %r12, 32(%rdi)
    mov %r13, 40(%rdi)
    mov %r14, 48(%rdi)
    mov %r15, 56(%rdi)
    xor %rax, %rax
    ret

get_context:
    mov 0(%rdi), %rsp
    mov 8(%rdi), %rax
    mov 16(%rdi), %rbx
    mov 24(%rdi), %rbp
    mov 32(%rdi), %r12
    mov 40(%rdi), %r13
    mov 48(%rdi), %r14
    mov 56(%rdi), %r15
    push %rax
    ret