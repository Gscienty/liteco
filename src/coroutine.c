#include "liteco.h"
#include <stddef.h>

static int liteco_callback(void *const);

extern int liteco_internal_context_swap(liteco_internal_context_t *const from, liteco_internal_context_t *const to);
extern int liteco_internal_context_make(liteco_internal_context_t *const ctx, void *stack, size_t st_size, int (*fn) (void *const), void *const args);

int liteco_status(const liteco_coroutine_t *const co) {
    if (co == NULL) {
        return LITECO_UNKNOW;
    }
    return co->status;
}

int liteco_create(liteco_coroutine_t *const co, void *const stack, size_t st_size, int (*fn) (liteco_coroutine_t *const, void *const), void *const args) {
    if (co == NULL || stack == NULL || fn == NULL) {
        return LITECO_PARAMETER_UNEXCEPTION;
    }
    co->fn = fn;
    co->args = args;
    co->status = LITECO_STARTING;
    co->swap_link = (liteco_internal_context_t **) ((((unsigned long) (stack + st_size) - 8) & 0xfffffffffffffff0));
    liteco_internal_context_make(&co->context, stack, st_size, liteco_callback, co);

    return LITECO_SUCCESS;
}

int liteco_resume(liteco_coroutine_t *const co) {
    liteco_internal_context_t this_context;
    if (co == NULL) {
        return LITECO_PARAMETER_UNEXCEPTION;
    }
    switch (co->status) {
    case LITECO_TERMINATE:
        return LITECO_SUCCESS;
    }
    *co->swap_link = &this_context;
    liteco_internal_context_swap(&this_context, &co->context);

    return LITECO_SUCCESS;
}

int liteco_yield(liteco_coroutine_t *const co) {
    if (co == NULL) {
        return LITECO_PARAMETER_UNEXCEPTION;
    }
    co->status = LITECO_READYING;
    liteco_internal_context_swap(&co->context, *co->swap_link);

    return LITECO_SUCCESS;
}

static int liteco_callback(void *const co_) {
    liteco_coroutine_t *const co = co_;
    if (co_ == NULL) {
        return LITECO_PARAMETER_UNEXCEPTION;
    }

    co->fn(co, co->args);

    co->status = LITECO_TERMINATE;

    return LITECO_SUCCESS;
}
