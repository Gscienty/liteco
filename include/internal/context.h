#ifndef __LITECO_INTERNAL_CONTEXT_H__
#define __LITECO_INTERNAL_CONTEXT_H__

#include <sys/types.h>

struct liteco_internal_stack {
    void *stack_pointer;
    u_int64_t stack_size;
} __attribute__((packed));

struct liteco_internal_context {
    struct liteco_internal_context *link;
    u_int8_t regs[256];
    struct liteco_internal_stack stack;
} __attribute__((packed));

int liteco_internal_context_current(struct liteco_internal_context *const ctx);
int liteco_internal_context_swap(struct liteco_internal_context *const from, struct liteco_internal_context *const to);
int liteco_internal_context_make(struct liteco_internal_context *const ctx, int (*fn) (void *const), void *const args);

#endif
