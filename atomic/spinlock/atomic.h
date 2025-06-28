#pragma once

#include <stdint.h>

inline void AtomicAdd(int64_t* atomic, int64_t value) {
    asm volatile (
            "lock add %1, %0"
            : "+m"(*atomic)
            : "r"(value)
            );
}

inline void AtomicSub(int64_t* atomic, int64_t value) {
    asm volatile (
            "lock sub %1, %0"
            : "+m"(*atomic)
            : "r"(value)
            );
}

inline int64_t AtomicXchg(int64_t* atomic, int64_t value) {
    asm volatile (
            "xchg %1, %0"
            : "+m"(*atomic), "+r"(value)
            );
    return value;
}

inline int64_t AtomicCas(int64_t* atomic, int64_t* expected, int64_t value) {
    int8_t flag;
    asm volatile (
            "lock cmpxchg %3, %0\n"
            "setz %1\n"
            : "+m"(*atomic), "=q"(flag), "+a"(*expected)
            : "r"(value)
            : "memory"
            );
    return flag;
}