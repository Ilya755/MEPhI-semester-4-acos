        .global sum
        .global sum_struct
        .global sum_array
        .global sum_args

// input:
//    rdi: первое int64_t число
//    rsi: второе int64_t число
//
// output:
//    rax: сумма чисел
//
sum:
    // your code here
    xor %rax, %rax
    add %rdi, %rax
    add %rsi, %rax
    ret

// input:
//    rdi: указатель на структуру
//
// struct {
//     int64_t a;
//     int64_t b;
//     int64_t sum;
// };
//
// output:
//    в поле sum нужно записать сумму a и b
//
sum_struct:
    xor %rax, %rax
    add (%rdi), %rax
    add 8(%rdi), %rax
    mov %rax, 16(%rdi)
    ret

// input:
//    rdi: указатель на массив int64_t
//    rsi: количество элементов в массиве
//
// output:
//    rax: сумма элементов массива
//
sum_array:
    xor %rax, %rax
    xor %rcx, %rcx
    call .for_loop

.for_loop:
    cmp %rsi, %rcx
    je .end_loop
    add (%rdi), %rax
    add $8, %rdi
    inc %rcx
    call .for_loop

.end_loop:
    ret

// input:
//    rdi: количество элементов на стеке (сразу после адреса возврата из функции)
//
// output:
//    rax: сумма элементов со стека
//
sum_args:
    xor %rax, %rax
    xor %rcx, %rcx
    lea 8(%rsp), %rsi
    jmp .for_loop_

.for_loop_:
    cmp %rdi, %rcx
    je .end_loop_
    add (%rsi), %rax
    add $8, %rsi
    inc %rcx
    jmp .for_loop_

.end_loop_:
    ret