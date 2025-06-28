#include "malloc.h"

void* malloc(size_t size){
    void* adr = mmap(NULL, size + sizeof(size_t), PROT_READ | PROT_WRITE,
                        MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (adr == MAP_FAILED) {
        return NULL;
    } else {
        *((size_t*)adr) = size + sizeof(size_t);
    }
    return (void*)(adr + sizeof(size_t));
}

void free(void* ptr) {
    if (ptr == NULL) {
        return;
    } else {
        size_t size = *((size_t*)((ptr - sizeof(size_t))));
        if (munmap(ptr - sizeof(size_t), size) == -1) {
            // munmap failed
            exit(1);
        }
    }
}

void* realloc(void* ptr, size_t size) {
    if (ptr == NULL) {
        return malloc(size);
    }
    if (size == 0) {
        free(ptr);
        return NULL;
    }
    size_t old_size = *((size_t*)((char*)ptr - sizeof(size_t)));
    void* new_adr = mremap((char*)ptr - sizeof(size_t),
                            old_size, size + sizeof(size_t),
                            MREMAP_MAYMOVE, NULL);
    if (new_adr == MAP_FAILED) {
        return NULL;
    } else {
        *((size_t*)new_adr) = size + sizeof(size_t);
        return (void*)((char*)new_adr + sizeof(size_t));
    }
}
