#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <errno.h>
#include <sys/mman.h>
#include <string.h>

extern size_t PAGE_SIZE;
extern double* SQRTS;

extern const int MAX_SQRTS;

double* prev_page = NULL;

void CalculateSqrts(double* sqrt_pos, int start, int n);

void HandleSigsegv(int sig, siginfo_t* siginfo, void* ctx) {
    if (prev_page != NULL) {
        if (munmap(prev_page, PAGE_SIZE) == -1) {
            fprintf(stderr, "Couldn't munmap() previous page: %s\n", strerror(errno));
            exit(1);
        }
    }
    double* fault_adr = (double*)siginfo->si_addr;
    if (fault_adr < SQRTS || fault_adr >= SQRTS + MAX_SQRTS) {
        fprintf(stderr, "Segmentation fault at invalid address: %p\n", fault_adr);
        exit(1);
    }
    fault_adr = (double*)((size_t)fault_adr & ~(PAGE_SIZE - 1));
    if (mmap(fault_adr, PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_FIXED |
                MAP_PRIVATE | MAP_ANONYMOUS, -1, 0) == MAP_FAILED) {
        fprintf(stderr, "Couldn't mmap() region for page: %s\n", strerror(errno));
        exit(1);
    }
    int pos = (int)(((void*)(PAGE_SIZE * ((size_t)fault_adr / PAGE_SIZE))
                - (void*)SQRTS) / sizeof(double));
    CalculateSqrts(fault_adr, pos, (int)(PAGE_SIZE / sizeof(double)));
    prev_page = fault_adr;
}
