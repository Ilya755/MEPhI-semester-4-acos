#include "fiber.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

static const int64_t STACK_SIZE = 1024 * 1024;

typedef struct context {
    void* regs[8];
} context;

typedef struct Fiber {
    struct Fiber* next;
    struct Fiber* prev;
    void (*func)(void*);
    void* data;
    void* stack;
    context* ctx;
    int64_t done;
    int64_t repeat;
} Fiber;

static Fiber* fibers = NULL;

extern void save_context(context* ctx);
extern void get_context(context* ctx);

int64_t first = 0;

void start_func() {
    fibers->func(fibers->data);

    Fiber* tmp = fibers;
    Fiber* nxt = fibers->next;
    tmp->prev->next = tmp->next;
    tmp->next->prev = tmp->prev;
    free(tmp->ctx);
    free(tmp->stack);
    free(tmp);
    fibers = nxt;
    fibers->done = 1;
    get_context(fibers->ctx);
}

void create_new_fiber() {
    Fiber* fiber = (Fiber*)malloc(sizeof(Fiber));
    if (!fiber) {
        printf("error: failed to allocate fiber\n");
        exit(1);
    }
    fiber->stack = aligned_alloc(16, STACK_SIZE);
    if (!fiber->stack) {
        printf("error: failed to allocate fiber stack\n");
        free(fiber);
        exit(1);
    }
    fiber->ctx = (context*)malloc(sizeof(context));
    if (!fiber->ctx) {
        printf("error: failed to allocate fiber context\n");
        free(fiber->stack);
        free(fiber);
        exit(1);
    }
    for (size_t i = 0; i < sizeof(context) / sizeof(void*); ++i) {
        fiber->ctx->regs[i] = NULL;
    }
    fiber->ctx->regs[0] = (void*)((char*)fiber->stack + STACK_SIZE - 8);
    fiber->ctx->regs[1] = (void*)start_func;
    fiber->done = 0;
    fiber->repeat = 1;
    if (fibers == NULL) {
        fibers = fiber;
        fibers->next = fibers;
        fibers->prev = fibers;
    } else {
        fibers->prev->next = fiber;
        fiber->next = fibers;
        fiber->prev = fibers->prev;
        fibers->prev = fiber;
    }
}

void FiberSpawn(void (*func)(void*), void* data) {
    create_new_fiber();
    fibers->prev->func = func;
    fibers->prev->data = data;
}

void FiberYield() {
    if (fibers == NULL) {
        return;
    }
    if (first == 0 && fibers->next == fibers) {
        first = 1;
        create_new_fiber();
        fibers->prev->func = NULL;
        fibers->prev->data = NULL;
        fibers->prev->repeat = 0;
        save_context(fibers->prev->ctx);
    } else {
        save_context(fibers->ctx);
        fibers->repeat = 0;
        if (!fibers->done) fibers = fibers->next;
    }
    if (fibers->done == 1) {
        fibers->done = 0;
        return;
    }
    if (fibers->repeat == 0) {
        fibers->done = 1;
    }
    get_context(fibers->ctx);
}

int FiberTryJoin() {
    if (first == 1 && fibers->next == fibers) {
        return 1;
    }
    return 0;
}