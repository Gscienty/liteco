#include "liteco.h"
#include <stddef.h>
#include <sys/types.h>
#include <sys/time.h>

__thread liteco_coroutine_t *__CURR_CO__ = NULL;

static int liteco_callback(void *const);

extern int liteco_internal_context_swap(liteco_internal_context_t *const from, liteco_internal_context_t *const to);
extern int liteco_internal_context_make(liteco_internal_context_t *const ctx, void *stack, size_t st_size, int (*fn) (void *const), void *const args);
extern int liteco_internal_atomic_cas(int *const ptr, const int old, const int nex);
extern int liteco_internal_p_yield(const int n);

int liteco_create(liteco_coroutine_t *const co,
                  void *const stack, size_t st_size,
                  int (*fn) (liteco_coroutine_t *const, void *const), void *const args,
                  int (*finished_fn) (liteco_coroutine_t *const)) {
    if (co == NULL || stack == NULL || fn == NULL) {
        return LITECO_PARAMETER_UNEXCEPTION;
    }
    co->fn = fn;
    co->args = args;
    co->status = LITECO_STARTING;
    co->link = (liteco_internal_context_t **) ((((unsigned long) (stack + st_size) - 8) & 0xfffffffffffffff0));
    co->finished_fn = finished_fn;
    co->st_size = st_size;
    co->stack = stack;
    pthread_mutex_init(&co->mutex, NULL);
    liteco_internal_context_make(&co->context, stack, st_size, liteco_callback, co);
    *co->link = NULL;
    co->result = 0;

    return LITECO_SUCCESS;
}

int liteco_coroutine_set_status(liteco_coroutine_t *const co, int status) {
    if (co == NULL) {
        return LITECO_PARAMETER_UNEXCEPTION;
    }
    pthread_mutex_lock(&co->mutex);
    co->status = status;
    pthread_mutex_unlock(&co->mutex);

    return LITECO_SUCCESS;
}

int liteco_resume(liteco_coroutine_t *const co) {
    liteco_internal_context_t this_context;
    if (co == NULL) {
        return LITECO_PARAMETER_UNEXCEPTION;
    }
    pthread_mutex_lock(&co->mutex);
    *co->link = &this_context;
    pthread_mutex_unlock(&co->mutex);

    __CURR_CO__ = co;
    liteco_internal_context_swap(&this_context, &co->context);

    return LITECO_SUCCESS;
}

int liteco_yield() {
    liteco_coroutine_t *co = __CURR_CO__;
    if (__CURR_CO__ == NULL) {
        return LITECO_PARAMETER_UNEXCEPTION;
    }
    __CURR_CO__ = NULL;
    liteco_internal_context_swap(&co->context, *co->link);
    return LITECO_SUCCESS;
}

static int liteco_callback(void *const co_) {
    liteco_coroutine_t *const co = co_;
    if (co_ == NULL) {
        return LITECO_PARAMETER_UNEXCEPTION;
    }
    co->result = co->fn(co, co->args);
    pthread_mutex_lock(&co->mutex);
    co->status = LITECO_TERMINATE;
    pthread_mutex_unlock(&co->mutex);

    return liteco_yield();
}

static inline u_int64_t __now__() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000 * 1000 + tv.tv_usec;
}

int liteco_status_cas(liteco_coroutine_t *const co, int old, int nex) {
    int i;
    int x;
    u_int64_t next_yield;
    if (co == NULL) {
        return LITECO_PARAMETER_UNEXCEPTION;
    }

    for (i = 0; !liteco_internal_atomic_cas(&co->status, old, nex); i++) {
        if (old == LITECO_WAITING && co->status == LITECO_RUNNING) {
            return LITECO_INTERNAL_ERROR;
        }

        if (i == 0) {
            next_yield = __now__() + 5;
        }

        if (__now__() < next_yield) {
            for (x = 0; x < 10 && co->status != old; x++) {
                liteco_internal_p_yield(1);
            }
        }
        else {
            sched_yield();

            next_yield = __now__() + 2;
        }
    }

    return LITECO_SUCCESS;
}

