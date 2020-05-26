#ifndef __LITECO_INTERNAL_CONTEXT_H__
#define __LITECO_INTERNAL_CONTEXT_H__

#include <sys/types.h>

struct liteco_internal_context {
    struct liteco_internal_context *link;
    u_int8_t regs[256];
} __attribute__((packed));

int liteco_internal_context_swap(struct liteco_internal_context *const from, struct liteco_internal_context *const to);
int liteco_internal_context_make(struct liteco_internal_context *const ctx, void *stack, size_t size, int (*fn) (void *const), void *const args);

#endif
