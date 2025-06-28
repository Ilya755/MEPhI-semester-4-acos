#pragma once

#include "atomic.h"

struct SpinLock {
    int64_t lock;
};

inline void SpinLock_Init(struct SpinLock* lock) {
    AtomicXchg(&lock->lock, 0);
}

inline void SpinLock_Lock(struct SpinLock* lock) {
    while (AtomicXchg(&lock->lock, 1)) {
        continue;
    }
    __sync_synchronize();
}

inline void SpinLock_Unlock(struct SpinLock* lock) {
    int64_t expected = 1;
    AtomicCas(&lock->lock, &expected, 0);
    __sync_synchronize();
}
