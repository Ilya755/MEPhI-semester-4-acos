#pragma once

#include <stdint.h>
#include "atomic.h"
#include "spinlock.h"

#include <stdatomic.h>

struct SeqLock {
    struct SpinLock lock;
    int64_t count;
};

inline void SeqLock_Init(struct SeqLock* lock) {
    SpinLock_Init(&lock->lock);
    lock->count = 0;
}

inline int64_t SeqLock_ReadLock(struct SeqLock* lock) {
    return AtomicLoad(&lock->count);
}

inline int SeqLock_ReadUnlock(struct SeqLock* lock, int64_t value) {
    int64_t temp = AtomicLoad(&lock->count);
    return (temp == value && temp % 2 == 0);
}

inline void SeqLock_WriteLock(struct SeqLock* lock) {
    SpinLock_Lock(&lock->lock);
    AtomicAdd(&lock->count, 1);
    __sync_synchronize();
}

inline void SeqLock_WriteUnlock(struct SeqLock* lock) {
    AtomicAdd(&lock->count, 1);
    SpinLock_Unlock(&lock->lock);
    __sync_synchronize();
}
