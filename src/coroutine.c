#include "liteco.h"
#include <stddef.h>

static int liteco_callback(void *const);

extern int liteco_internal_context_swap(liteco_internal_context_t *const from, liteco_internal_context_t *const to);
extern int liteco_internal_context_make(liteco_internal_context_t *const ctx, void *stack, size_t st_size, int (*fn) (void *const), void *const args);

int liteco_create(liteco_coroutine_t *const co,
                  void *const stack, size_t st_size,
                  int (*fn) (liteco_coroutine_t *const, void *const), void *const args,
                  int (*release) (void *const, const size_t)) {
    if (co == NULL || stack == NULL || fn == NULL) {
        return LITECO_PARAMETER_UNEXCEPTION;
    }
    co->fn = fn;
    co->args = args;
    co->status = LITECO_STARTING;
    co->link = (liteco_internal_context_t **) ((((unsigned long) (stack + st_size) - 8) & 0xfffffffffffffff0));
    co->sche = NULL;
    co->ref_count = 1;
    co->release = release;
    co->st_size = st_size;
    co->stack = stack;
    pthread_mutex_init(&co->mutex, NULL);
    liteco_internal_context_make(&co->context, stack, st_size, liteco_callback, co);
    *co->link = NULL;

    return LITECO_SUCCESS;
}

int liteco_resume(liteco_coroutine_t *const co) {
    liteco_internal_context_t this_context;
    if (co == NULL) {
        return LITECO_PARAMETER_UNEXCEPTION;
    }
    pthread_mutex_lock(&co->mutex);
    if (co->status == LITECO_TERMINATE) {
        pthread_mutex_unlock(&co->mutex);
        return LITECO_SUCCESS;
    }
    if (co->status == LITECO_RUNNING) {
        pthread_mutex_unlock(&co->mutex);
        return LITECO_SUCCESS;
    }
    co->status = LITECO_RUNNING;
    *co->link = &this_context;
    pthread_mutex_unlock(&co->mutex);

    liteco_internal_context_swap(&this_context, &co->context);

    return LITECO_SUCCESS;
}

int liteco_yield(liteco_coroutine_t *const co) {
    if (co == NULL) {
        return LITECO_PARAMETER_UNEXCEPTION;
    }

    pthread_mutex_lock(&co->mutex);
    if (co->status == LITECO_RUNNING) {
        co->status = LITECO_READYING;
    }
    pthread_mutex_unlock(&co->mutex);

    liteco_internal_context_swap(&co->context, *co->link);
    return LITECO_SUCCESS;
}

int liteco_release(liteco_coroutine_t *const co) {
    if (co == NULL) {
        return LITECO_PARAMETER_UNEXCEPTION;
    }

    pthread_mutex_lock(&co->mutex);
    co->status = LITECO_TERMINATE;
    co->ref_count--;
    if (co->ref_count == 0) {
        co->release(co->stack, co->st_size);
    }
    pthread_mutex_unlock(&co->mutex);

    return LITECO_SUCCESS;
}

static int liteco_callback(void *const co_) {
    liteco_coroutine_t *const co = co_;
    if (co_ == NULL) {
        return LITECO_PARAMETER_UNEXCEPTION;
    }
    co->fn(co, co->args);
    pthread_mutex_lock(&co->mutex);
    co->status = LITECO_TERMINATE;
    pthread_mutex_unlock(&co->mutex);
    return LITECO_SUCCESS;
}
