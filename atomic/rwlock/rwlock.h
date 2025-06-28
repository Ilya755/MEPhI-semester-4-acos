#pragma once

#include "atomic.h"
#include "spinlock.h"

struct RwLock {
    int64_t readers_count;
    struct SpinLock readers_lock;
    struct SpinLock writers_lock;
};

inline void RwLock_Init(struct RwLock* lock) {
    lock->readers_count = 0;
    SpinLock_Init(&lock->readers_lock);
    SpinLock_Init(&lock->writers_lock);
}

inline void RwLock_ReadLock(struct RwLock* lock) {
    SpinLock_Lock(&lock->writers_lock);
    AtomicAdd(&lock->readers_count, 1);
    SpinLock_Unlock(&lock->writers_lock);
}

inline void RwLock_ReadUnlock(struct RwLock* lock) {
    SpinLock_Lock(&lock->readers_lock);
    AtomicSub(&lock->readers_count, 1);
    SpinLock_Unlock(&lock->readers_lock);
}

inline void RwLock_WriteLock(struct RwLock* lock) {
    SpinLock_Lock(&lock->writers_lock);
    while (AtomicLoad(&lock->readers_count) != 0) {
        continue;
    }
}

inline void RwLock_WriteUnlock(struct RwLock* lock) {
    SpinLock_Unlock(&lock->writers_lock);
}
