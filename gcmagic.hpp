// Great platform support
#if defined(__x86_64__) && defined(__linux__)

#define GC_PUSH_ALL_REGS(STACK_BOTTOM) \
    asm("pushq %%rbp;" \
        "pushq %%rbx;" \
        "pushq %%r12;" \
        "pushq %%r13;" \
        "pushq %%r14;" \
        "pushq %%r15;" \
        "movq %%rsp, %0;" \
    : "=r"(STACK_BOTTOM) :: "sp")

#define GC_POP_ALL_REGS() \
    asm("popq %%r15;" \
        "popq %%r14;" \
        "popq %%r13;" \
        "popq %%r12;" \
        "popq %%rbx;" \
        "popq %%rbp;" \
    ::: "sp")

#else

#define GC_PUSH_ALL_REGS(STACK_BOTTOM) \
fprintf(stderr, "Out of memory and GC not supported on this platform.\n"); \
exit(1);

#define GC_POP_ALL_REGS()

#endif
