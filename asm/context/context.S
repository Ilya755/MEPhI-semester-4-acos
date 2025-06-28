        .global savectx
        .global jumpctx

savectx:
    mov 0(%rsp), %rcx
    mov %rcx, 0(%rdi)
    mov %rsp, 8(%rdi)
    mov %rbp, 16(%rdi)
    xor %rax, %rax
    ret

jumpctx:
    mov 8(%rdi), %rsp
    mov 0(%rdi), %rcx
    mov %rcx, 0(%rsp)
    mov 16(%rdi), %rbp
    mov $1, %rax
    ret